#pragma once

// Lock-Free Mailbox Queue

#include <atomic>
#include <memory>
#include <optional>

namespace aithon::runtime {

// MPSC (Multi-Producer Single-Consumer) lock-free queue
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::optional<T> data;
        std::atomic<Node*> next;
        
        Node() : next(nullptr) {}
        explicit Node(T value) : data(std::move(value)), next(nullptr) {}
    };
    
    std::atomic<Node*> head_;  // Consumer end
    std::atomic<Node*> tail_;  // Producer end
    
public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }
    
    ~LockFreeQueue() {
        while (Node* node = head_.load(std::memory_order_relaxed)) {
            head_.store(node->next.load(std::memory_order_relaxed));
            delete node;
        }
    }
    
    // Prevent copying
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    // Enqueue - can be called by multiple threads
    void enqueue(T value) {
        Node* new_node = new Node(std::move(value));
        Node* prev_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
        prev_tail->next.store(new_node, std::memory_order_release);
    }
    
    // Try dequeue - should only be called by owning actor
    std::optional<T> try_dequeue() {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next.load(std::memory_order_acquire);
        
        if (next == nullptr) {
            return std::nullopt;  // Queue is empty
        }
        
        // Extract data before moving head
        std::optional<T> result = std::move(next->data);
        
        // Move head forward
        head_.store(next, std::memory_order_release);
        
        // Delete old head
        delete head;
        
        return result;
    }
    
    bool is_empty() const {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }
};

} // namespace aithon::runtime