#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <cstring>

namespace aithon::runtime {

    class ActorHeap {
    private:
        // Bump allocator for fast allocation
        uint8_t* heap_start_;
        uint8_t* heap_end_;
        uint8_t* allocation_ptr_;

        size_t total_size_;
        size_t used_size_;

        // Object header for GC
        struct alignas(8) ObjectHeader {
            size_t size;
            bool marked;
        };

    public:
        explicit ActorHeap(size_t size);
        ~ActorHeap();

        // Prevent copying
        ActorHeap(const ActorHeap&) = delete;
        ActorHeap& operator=(const ActorHeap&) = delete;

        // Fast bump allocation
        void* allocate(size_t size);

        // Mark-and-sweep GC
        void collect_garbage();

        // Memory info
        size_t used() const { return used_size_; }
        size_t available() const { return total_size_ - used_size_; }
        size_t total() const { return total_size_; }

        // For debugging
        void dump_stats() const;

    private:
        void compact_heap();
    };

} // namespace pyvm::runtime