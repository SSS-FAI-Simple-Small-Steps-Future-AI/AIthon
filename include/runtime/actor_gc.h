#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <chrono>
#include <memory>

namespace aithon::runtime {

// Object header for GC tracking
struct alignas(16) GCObjectHeader {
    uint32_t size;              // Object size in bytes
    uint16_t generation;        // 0 = young, 1 = old
    uint8_t marked : 1;         // Mark bit for GC
    uint8_t pinned : 1;         // Pinned objects not moved
    uint8_t has_refs : 1;       // Contains references to other objects
    uint8_t reserved : 5;
    uint8_t type_id;            // Object type for scanning
    
    void* get_data() {
        return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(this) + sizeof(GCObjectHeader));
    }
};

// Generational Garbage Collector for single actor
class ActorGC {
public:
    struct GCStats {
        uint64_t total_collections;
        uint64_t young_collections;
        uint64_t old_collections;
        uint64_t objects_allocated;
        uint64_t objects_freed;
        uint64_t bytes_allocated;
        uint64_t bytes_freed;
        uint64_t promotions;  // Young → Old
        std::chrono::microseconds total_pause_time;
        std::chrono::microseconds avg_pause_time;
        std::chrono::microseconds max_pause_time;
    };
    
    enum class CollectionReason {
        YOUNG_FULL,
        OLD_FULL,
        EXPLICIT,
        ALLOCATION_FAILURE
    };
    
private:
    // Memory regions
    struct Generation {
        uint8_t* start;
        uint8_t* end;
        uint8_t* alloc_ptr;  // Bump pointer allocation
        size_t size;
        size_t used;
        
        Generation(size_t sz) : size(sz), used(0) {
            start = new uint8_t[sz];
            end = start + sz;
            alloc_ptr = start;
        }
        
        ~Generation() {
            delete[] start;
        }
        
        size_t available() const {
            return size - used;
        }
        
        bool contains(void* ptr) const {
            return ptr >= start && ptr < end;
        }
    };
    
    // Young generation (nursery) - small, collected frequently
    std::unique_ptr<Generation> young_gen_;
    
    // Old generation - larger, collected less frequently
    std::unique_ptr<Generation> old_gen_;
    
    // Root set - stack references, registers, etc.
    std::vector<void**> roots_;
    
    // Write barrier - track old→young references
    std::unordered_set<GCObjectHeader*> remembered_set_;
    
    // Statistics
    GCStats stats_;
    
    // Thresholds
    static constexpr size_t YOUNG_GEN_SIZE = 512 * 1024;      // 512KB
    static constexpr size_t OLD_GEN_SIZE = 8 * 1024 * 1024;   // 8MB
    static constexpr size_t PROMOTION_AGE = 3;                // Survive 3 collections
    static constexpr double YOUNG_THRESHOLD = 0.8;            // Collect at 80% full
    static constexpr double OLD_THRESHOLD = 0.9;              // Collect at 90% full
    
    // Allocation tracking
    std::unordered_map<GCObjectHeader*, uint16_t> allocation_age_;
    
public:
    ActorGC();
    ~ActorGC();
    
    // Allocation
    void* allocate(size_t size, uint8_t type_id = 0, bool has_refs = false);
    void* allocate_old(size_t size, uint8_t type_id = 0, bool has_refs = false);
    
    // Root management
    void add_root(void** root);
    void remove_root(void** root);
    
    // Collection
    void collect_young();  // Minor GC - only young generation
    void collect_full();   // Major GC - both generations
    void collect_if_needed();  // Automatic collection
    
    // Write barrier (for incremental GC)
    void write_barrier(void* old_value, void* new_value);
    
    // Statistics
    const GCStats& statistics() const { return stats_; }
    size_t young_used() const { return young_gen_->used; }
    size_t old_used() const { return old_gen_->used; }
    size_t total_used() const { return young_used() + old_used(); }
    
    // Memory pressure check
    bool is_memory_pressure() const;
    
    // Dump state for debugging
    void dump_state() const;
    
private:
    // Core GC algorithms
    void mark_from_roots();
    void mark_object(GCObjectHeader* obj);
    void sweep_young();
    void sweep_old();
    void evacuate_survivors();  // Copy surviving young objects
    
    // Promotion
    void promote_to_old(GCObjectHeader* obj);
    bool should_promote(GCObjectHeader* obj);
    
    // Allocation helpers
    void* allocate_in_generation(Generation& gen, size_t size, uint8_t type_id, bool has_refs);
    
    // Object scanning
    void scan_object_references(GCObjectHeader* obj, std::function<void(void**)> callback);
    
    // Compaction (optional)
    void compact_old_generation();
    
    // Update statistics
    void record_collection(CollectionReason reason, std::chrono::microseconds duration);
};

// Thread-local GC instance
extern thread_local ActorGC* current_actor_gc;

// Allocation helpers for generated code
extern "C" {
    void* gc_alloc(size_t size);
    void* gc_alloc_array(size_t elem_size, size_t count);
    void gc_add_root(void** root);
    void gc_remove_root(void** root);
    void gc_write_barrier(void* obj, void* field, void* new_value);
    void gc_collect();
}

} // namespace pyvm::runtime