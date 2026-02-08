#pragma once

#include "actor_process.h"
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>

namespace aithon::runtime {

// Green thread (lightweight actor) with independent memory and GC
class GreenThread {
public:
    enum class State {
        CREATED,
        READY,
        RUNNING,
        BLOCKED,
        TERMINATED
    };
    
private:
    int thread_id_;
    State state_;
    
    // Independent memory space
    std::unique_ptr<ActorHeap> private_heap_;
    
    // Private execution context
    void* stack_pointer_;
    void* instruction_pointer_;
    void* registers_[16];  // Saved register state
    
    // Garbage collector for this thread
    struct GCStats {
        size_t collections_count;
        size_t objects_freed;
        size_t bytes_freed;
        std::chrono::microseconds total_gc_time;
    };
    GCStats gc_stats_;
    
    // Crash isolation
    bool has_crashed_;
    std::string crash_reason_;
    
    // Message passing (actor model)
    LockFreeQueue<Message> mailbox_;
    
    // Parent supervisor for crash reporting
    int supervisor_id_;
    
    // Execution behavior
    ActorProcess::BehaviorFn behavior_;
    void* initial_args_;
    
public:
    explicit GreenThread(int id, size_t heap_size = 2 * 1024 * 1024);  // 2MB default
    ~GreenThread();
    
    // Prevent copying
    GreenThread(const GreenThread&) = delete;
    GreenThread& operator=(const GreenThread&) = delete;
    
    // State management
    State state() const { return state_; }
    void set_state(State new_state) { state_ = new_state; }
    
    int id() const { return thread_id_; }
    bool is_alive() const { return !has_crashed_ && state_ != State::TERMINATED; }
    
    // Execution
    void set_behavior(ActorProcess::BehaviorFn fn, void* args = nullptr);
    bool execute_quantum();  // Execute one time slice
    
    // Message passing
    bool send_message(const Message& msg);
    Message* receive_message();
    bool has_messages() const { return !mailbox_.is_empty(); }
    
    // Garbage collection
    void run_gc();  // Explicit GC
    void auto_gc_check();  // Automatic GC based on heap pressure
    const GCStats& gc_statistics() const { return gc_stats_; }
    
    // Memory management
    void* allocate(size_t size);
    size_t memory_used() const { return private_heap_->used(); }
    size_t memory_available() const { return private_heap_->available(); }
    
    // Crash handling
    void crash(const std::string& reason);
    bool has_crashed() const { return has_crashed_; }
    const std::string& crash_reason() const { return crash_reason_; }
    
    // Supervision
    void set_supervisor(int supervisor_id) { supervisor_id_ = supervisor_id; }
    int supervisor() const { return supervisor_id_; }
    
    // Context switching
    void save_context();
    void restore_context();
    
private:
    // GC implementation
    void mark_and_sweep();
    void update_gc_stats(std::chrono::microseconds duration, size_t freed);
};

// Green thread scheduler with M:N threading
class GreenThreadScheduler {
private:
    struct WorkerThread {
        std::thread thread;
        std::deque<GreenThread*> ready_queue;
        std::deque<GreenThread*> blocked_queue;
        std::mutex queue_mutex;
        std::condition_variable queue_cv;
        std::atomic<bool> running;
        
        // Per-worker statistics
        std::atomic<uint64_t> threads_executed;
        std::atomic<uint64_t> context_switches;
        std::atomic<uint64_t> messages_processed;
    };
    
    // M:N threading - M green threads on N OS threads
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    size_t num_workers_;
    
    // All green threads
    std::unordered_map<int, std::unique_ptr<GreenThread>> green_threads_;
    std::mutex threads_mutex_;
    std::atomic<int> next_thread_id_{0};
    
    // Scheduling policy
    enum class SchedulingPolicy {
        ROUND_ROBIN,
        WORK_STEALING,
        PRIORITY_BASED
    };
    SchedulingPolicy policy_;
    
    // GC coordinator
    std::atomic<bool> gc_pause_requested_{false};
    
    // Statistics
    std::atomic<uint64_t> total_green_threads_created_{0};
    std::atomic<uint64_t> total_messages_sent_{0};
    std::atomic<uint64_t> total_gc_collections_{0};
    
public:
    explicit GreenThreadScheduler(size_t num_workers = 0, 
                                 SchedulingPolicy policy = SchedulingPolicy::WORK_STEALING);
    ~GreenThreadScheduler();
    
    // Green thread lifecycle
    int spawn(ActorProcess::BehaviorFn behavior, 
             void* args = nullptr,
             size_t heap_size = 2 * 1024 * 1024);
    
    void terminate(int thread_id);
    
    // Message passing between green threads
    bool send_message(int from_id, int to_id, const Message& msg);
    
    // Get green thread (for debugging)
    GreenThread* get_thread(int thread_id);
    
    // Scheduler control
    void start();
    void stop();
    void pause_for_gc();  // Pause all threads for global GC
    void resume_after_gc();
    
    // Statistics
    void dump_statistics() const;
    size_t num_alive_threads() const;
    size_t total_memory_used() const;
    
private:
    // Worker thread main loop
    void worker_loop(size_t worker_id);
    
    // Scheduling
    void schedule_thread(GreenThread* thread, size_t preferred_worker);
    GreenThread* get_next_ready_thread(size_t worker_id);
    void move_blocked_to_ready(size_t worker_id);
    
    // Work stealing
    bool try_steal_work(size_t thief_id);
    
    // Choose which worker to assign thread to
    size_t choose_worker();
};

// Global green thread scheduler
extern GreenThreadScheduler* global_green_scheduler;

// Convenience functions
int spawn_green_thread(ActorProcess::BehaviorFn behavior, void* args = nullptr);
bool send_to_thread(int from_id, int to_id, void* data, size_t size);
void* thread_allocate(size_t size);  // Allocate from current thread's heap

} // namespace pyvm::runtime