#include "runtime/actor_gc.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace pyvm::runtime {

thread_local ActorGC* current_actor_gc = nullptr;

ActorGC::ActorGC()
    : young_gen_(std::make_unique<Generation>(YOUNG_GEN_SIZE)),
      old_gen_(std::make_unique<Generation>(OLD_GEN_SIZE)) {
    
    // Initialize statistics
    stats_ = {};
}

ActorGC::~ActorGC() {
    // Cleanup handled by unique_ptr
}

void* ActorGC::allocate(size_t size, uint8_t type_id, bool has_refs) {
    // Try young generation first (fast path)
    void* ptr = allocate_in_generation(*young_gen_, size, type_id, has_refs);
    
    if (!ptr) {
        // Young gen full - collect young generation
        collect_young();
        
        // Try again
        ptr = allocate_in_generation(*young_gen_, size, type_id, has_refs);
        
        if (!ptr) {
            // Still can't allocate - try old generation directly
            ptr = allocate_old(size, type_id, has_refs);
        }
    }
    
    if (ptr) {
        stats_.objects_allocated++;
        stats_.bytes_allocated += size;
    }
    
    return ptr;
}

void* ActorGC::allocate_old(size_t size, uint8_t type_id, bool has_refs) {
    void* ptr = allocate_in_generation(*old_gen_, size, type_id, has_refs);
    
    if (!ptr) {
        // Old gen full - do major collection
        collect_full();
        
        ptr = allocate_in_generation(*old_gen_, size, type_id, has_refs);
    }
    
    return ptr;
}

void* ActorGC::allocate_in_generation(Generation& gen, size_t size, 
                                      uint8_t type_id, bool has_refs) {
    // Align size
    size_t aligned_size = (size + 15) & ~15;
    size_t total_size = sizeof(GCObjectHeader) + aligned_size;
    
    // Check space
    if (gen.alloc_ptr + total_size > gen.end) {
        return nullptr;
    }
    
    // Allocate
    GCObjectHeader* header = reinterpret_cast<GCObjectHeader*>(gen.alloc_ptr);
    header->size = aligned_size;
    header->generation = (&gen == young_gen_.get()) ? 0 : 1;
    header->marked = false;
    header->pinned = false;
    header->has_refs = has_refs;
    header->type_id = type_id;
    
    gen.alloc_ptr += total_size;
    gen.used += total_size;
    
    // Track allocation age for young objects
    if (header->generation == 0) {
        allocation_age_[header] = 0;
    }
    
    return header->get_data();
}

void ActorGC::add_root(void** root) {
    roots_.push_back(root);
}

void ActorGC::remove_root(void** root) {
    roots_.erase(std::remove(roots_.begin(), roots_.end(), root), roots_.end());
}

void ActorGC::collect_young() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Minor GC - only collect young generation
    // This is fast and happens frequently
    
    // 1. Mark phase - mark from roots
    mark_from_roots();
    
    // 2. Check remembered set (old→young references)
    for (auto* old_obj : remembered_set_) {
        scan_object_references(old_obj, [this](void** ref) {
            void* obj = *ref;
            if (obj && young_gen_->contains(obj)) {
                auto* header = reinterpret_cast<GCObjectHeader*>(
                    static_cast<uint8_t*>(obj) - sizeof(GCObjectHeader)
                );
                mark_object(header);
            }
        });
    }
    
    // 3. Evacuate survivors (copy to old gen if aged enough)
    evacuate_survivors();
    
    // 4. Reset young generation
    young_gen_->alloc_ptr = young_gen_->start;
    young_gen_->used = 0;
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    record_collection(CollectionReason::YOUNG_FULL, duration);
    stats_.young_collections++;
}

void ActorGC::collect_full() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Major GC - collect both generations
    // This is slower but happens less frequently
    
    // 1. Mark phase - mark all reachable objects
    mark_from_roots();
    
    // 2. Sweep both generations
    sweep_young();
    sweep_old();
    
    // 3. Optional: compact old generation to reduce fragmentation
    if (old_gen_->used > old_gen_->size * 0.7) {
        compact_old_generation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    record_collection(CollectionReason::OLD_FULL, duration);
    stats_.old_collections++;
}

void ActorGC::collect_if_needed() {
    // Automatic collection based on memory pressure
    
    double young_usage = static_cast<double>(young_gen_->used) / young_gen_->size;
    double old_usage = static_cast<double>(old_gen_->used) / old_gen_->size;
    
    if (young_usage > YOUNG_THRESHOLD) {
        collect_young();
    }
    
    if (old_usage > OLD_THRESHOLD) {
        collect_full();
    }
}

void ActorGC::mark_from_roots() {
    // Mark all objects reachable from roots
    for (void** root : roots_) {
        if (*root) {
            void* obj = *root;
            GCObjectHeader* header = nullptr;
            
            // Determine which generation
            if (young_gen_->contains(obj)) {
                header = reinterpret_cast<GCObjectHeader*>(
                    static_cast<uint8_t*>(obj) - sizeof(GCObjectHeader)
                );
            } else if (old_gen_->contains(obj)) {
                header = reinterpret_cast<GCObjectHeader*>(
                    static_cast<uint8_t*>(obj) - sizeof(GCObjectHeader)
                );
            }
            
            if (header) {
                mark_object(header);
            }
        }
    }
}

void ActorGC::mark_object(GCObjectHeader* obj) {
    if (!obj || obj->marked) {
        return;  // Already marked
    }
    
    obj->marked = true;
    
    // Recursively mark referenced objects
    if (obj->has_refs) {
        scan_object_references(obj, [this](void** ref) {
            void* child = *ref;
            if (child) {
                GCObjectHeader* child_header = nullptr;
                
                if (young_gen_->contains(child)) {
                    child_header = reinterpret_cast<GCObjectHeader*>(
                        static_cast<uint8_t*>(child) - sizeof(GCObjectHeader)
                    );
                } else if (old_gen_->contains(child)) {
                    child_header = reinterpret_cast<GCObjectHeader*>(
                        static_cast<uint8_t*>(child) - sizeof(GCObjectHeader)
                    );
                }
                
                if (child_header) {
                    mark_object(child_header);
                }
            }
        });
    }
}

void ActorGC::sweep_young() {
    uint8_t* scan = young_gen_->start;
    
    while (scan < young_gen_->alloc_ptr) {
        GCObjectHeader* header = reinterpret_cast<GCObjectHeader*>(scan);
        size_t obj_size = sizeof(GCObjectHeader) + header->size;
        
        if (!header->marked) {
            // Object is garbage
            stats_.objects_freed++;
            stats_.bytes_freed += header->size;
        }
        
        // Reset mark bit for next collection
        header->marked = false;
        
        scan += obj_size;
    }
}

void ActorGC::sweep_old() {
    uint8_t* scan = old_gen_->start;
    uint8_t* new_alloc_ptr = old_gen_->start;
    size_t new_used = 0;
    
    while (scan < old_gen_->alloc_ptr) {
        GCObjectHeader* header = reinterpret_cast<GCObjectHeader*>(scan);
        size_t obj_size = sizeof(GCObjectHeader) + header->size;
        
        if (header->marked) {
            // Keep this object - compact it if necessary
            if (scan != new_alloc_ptr) {
                std::memmove(new_alloc_ptr, scan, obj_size);
            }
            new_alloc_ptr += obj_size;
            new_used += obj_size;
            
            // Reset mark bit
            header->marked = false;
        } else {
            // Object is garbage
            stats_.objects_freed++;
            stats_.bytes_freed += header->size;
        }
        
        scan += obj_size;
    }
    
    old_gen_->alloc_ptr = new_alloc_ptr;
    old_gen_->used = new_used;
}

void ActorGC::evacuate_survivors() {
    // Scan young generation and promote aged survivors to old generation
    uint8_t* scan = young_gen_->start;
    
    while (scan < young_gen_->alloc_ptr) {
        GCObjectHeader* header = reinterpret_cast<GCObjectHeader*>(scan);
        size_t obj_size = sizeof(GCObjectHeader) + header->size;
        
        if (header->marked && should_promote(header)) {
            promote_to_old(header);
        }
        
        scan += obj_size;
    }
}

void ActorGC::promote_to_old(GCObjectHeader* obj) {
    // Copy object from young to old generation
    size_t total_size = sizeof(GCObjectHeader) + obj->size;
    
    void* new_location = allocate_in_generation(*old_gen_, obj->size, 
                                                obj->type_id, obj->has_refs);
    
    if (new_location) {
        // Copy data
        std::memcpy(new_location, obj->get_data(), obj->size);
        
        // Update references to point to new location
        // (This requires cooperation with mutator or read barrier)
        
        stats_.promotions++;
    }
}

bool ActorGC::should_promote(GCObjectHeader* obj) {
    auto it = allocation_age_.find(obj);
    if (it != allocation_age_.end()) {
        it->second++;
        return it->second >= PROMOTION_AGE;
    }
    return false;
}

void ActorGC::scan_object_references(GCObjectHeader* obj, 
                                     std::function<void(void**)> callback) {
    // Scan object for references based on type
    // This is simplified - real implementation would use type-specific scanners
    
    if (!obj->has_refs) {
        return;
    }
    
    // For now, assume objects are arrays of pointers
    void* data = obj->get_data();
    size_t num_refs = obj->size / sizeof(void*);
    
    void** refs = static_cast<void**>(data);
    for (size_t i = 0; i < num_refs; i++) {
        callback(&refs[i]);
    }
}

void ActorGC::write_barrier(void* old_value, void* new_value) {
    // Track old→young references for generational GC
    if (!new_value) return;
    
    // If new_value is in young gen and container is in old gen
    if (young_gen_->contains(new_value)) {
        // Find containing object
        // Add to remembered set
        // (Simplified - real implementation would track precisely)
    }
}

void ActorGC::compact_old_generation() {
    // Compact old generation to eliminate fragmentation
    // This is an optional optimization
    
    uint8_t* scan = old_gen_->start;
    uint8_t* compact_ptr = old_gen_->start;
    
    while (scan < old_gen_->alloc_ptr) {
        GCObjectHeader* header = reinterpret_cast<GCObjectHeader*>(scan);
        size_t obj_size = sizeof(GCObjectHeader) + header->size;
        
        if (header->marked) {
            if (scan != compact_ptr) {
                std::memmove(compact_ptr, scan, obj_size);
            }
            compact_ptr += obj_size;
        }
        
        scan += obj_size;
    }
    
    old_gen_->alloc_ptr = compact_ptr;
    old_gen_->used = compact_ptr - old_gen_->start;
}

bool ActorGC::is_memory_pressure() const {
    double young_usage = static_cast<double>(young_gen_->used) / young_gen_->size;
    double old_usage = static_cast<double>(old_gen_->used) / old_gen_->size;
    
    return young_usage > 0.7 || old_usage > 0.8;
}

void ActorGC::record_collection(CollectionReason reason, 
                                std::chrono::microseconds duration) {
    stats_.total_collections++;
    stats_.total_pause_time += duration;
    
    if (duration > stats_.max_pause_time) {
        stats_.max_pause_time = duration;
    }
    
    stats_.avg_pause_time = stats_.total_pause_time / stats_.total_collections;
}

void ActorGC::dump_state() const {
    std::cout << "Actor GC State:\n";
    std::cout << "  Young Gen: " << young_gen_->used << " / " 
              << young_gen_->size << " bytes\n";
    std::cout << "  Old Gen: " << old_gen_->used << " / " 
              << old_gen_->size << " bytes\n";
    std::cout << "  Total Collections: " << stats_.total_collections << "\n";
    std::cout << "  Young Collections: " << stats_.young_collections << "\n";
    std::cout << "  Old Collections: " << stats_.old_collections << "\n";
    std::cout << "  Objects Allocated: " << stats_.objects_allocated << "\n";
    std::cout << "  Objects Freed: " << stats_.objects_freed << "\n";
    std::cout << "  Promotions: " << stats_.promotions << "\n";
    std::cout << "  Avg Pause: " << stats_.avg_pause_time.count() << " μs\n";
    std::cout << "  Max Pause: " << stats_.max_pause_time.count() << " μs\n";
}

// C API for generated code
extern "C" {

void* gc_alloc(size_t size) {
    if (current_actor_gc) {
        return current_actor_gc->allocate(size);
    }
    return nullptr;
}

void* gc_alloc_array(size_t elem_size, size_t count) {
    if (current_actor_gc) {
        return current_actor_gc->allocate(elem_size * count, 0, true);
    }
    return nullptr;
}

void gc_add_root(void** root) {
    if (current_actor_gc) {
        current_actor_gc->add_root(root);
    }
}

void gc_remove_root(void** root) {
    if (current_actor_gc) {
        current_actor_gc->remove_root(root);
    }
}

void gc_write_barrier(void* obj, void* field, void* new_value) {
    if (current_actor_gc) {
        void* old_value = *static_cast<void**>(field);
        current_actor_gc->write_barrier(old_value, new_value);
    }
}

void gc_collect() {
    if (current_actor_gc) {
        current_actor_gc->collect_if_needed();
    }
}

} // extern "C"

} // namespace pyvm::runtime