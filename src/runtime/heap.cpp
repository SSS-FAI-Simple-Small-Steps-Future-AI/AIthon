#include "runtime/heap.h"
#include <iostream>
#include <cstdlib>

namespace pyvm::runtime {

ActorHeap::ActorHeap(size_t size) : total_size_(size), used_size_(0) {
    heap_start_ = static_cast<uint8_t*>(std::aligned_alloc(8, size));
    if (!heap_start_) {
        throw std::bad_alloc();
    }
    heap_end_ = heap_start_ + size;
    allocation_ptr_ = heap_start_;
}

ActorHeap::~ActorHeap() {
    std::free(heap_start_);
}

void* ActorHeap::allocate(size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;
    size_t total = sizeof(ObjectHeader) + size;
    
    if (allocation_ptr_ + total > heap_end_) {
        // Try GC first
        collect_garbage();
        
        if (allocation_ptr_ + total > heap_end_) {
            return nullptr;  // Out of memory even after GC
        }
    }
    
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(allocation_ptr_);
    header->size = size;
    header->marked = false;
    
    void* result = allocation_ptr_ + sizeof(ObjectHeader);
    allocation_ptr_ += total;
    used_size_ += total;
    
    return result;
}

void ActorHeap::collect_garbage() {
    // Simple mark-and-compact GC
    // In a real implementation, this would:
    // 1. Mark all reachable objects from roots
    // 2. Compact heap by moving live objects
    // 3. Update all pointers
    
    // For now, just compact without proper marking
    compact_heap();
}

void ActorHeap::compact_heap() {
    uint8_t* scan = heap_start_;
    uint8_t* new_allocation_ptr = heap_start_;
    size_t new_used_size = 0;
    
    while (scan < allocation_ptr_) {
        ObjectHeader* header = reinterpret_cast<ObjectHeader*>(scan);
        size_t obj_size = sizeof(ObjectHeader) + header->size;
        
        if (header->marked) {
            // Keep this object - move it if necessary
            if (scan != new_allocation_ptr) {
                std::memmove(new_allocation_ptr, scan, obj_size);
            }
            new_allocation_ptr += obj_size;
            new_used_size += obj_size;
            
            // Reset mark for next GC
            reinterpret_cast<ObjectHeader*>(new_allocation_ptr - obj_size)->marked = false;
        }
        
        scan += obj_size;
    }
    
    allocation_ptr_ = new_allocation_ptr;
    used_size_ = new_used_size;
}

void ActorHeap::dump_stats() const {
    std::cout << "Heap Stats:\n";
    std::cout << "  Total: " << total_size_ << " bytes\n";
    std::cout << "  Used: " << used_size_ << " bytes\n";
    std::cout << "  Available: " << (total_size_ - used_size_) << " bytes\n";
    std::cout << "  Usage: " << (100.0 * used_size_ / total_size_) << "%\n";
}

} // namespace pyvm::runtime