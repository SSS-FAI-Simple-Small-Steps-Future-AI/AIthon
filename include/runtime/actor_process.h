#pragma once

// Actor Process with Isolated Memory


#include "heap.h"
#include "lockfree_queue.h"
#include "message.h"
#include <atomic>
#include <functional>
#include <vector>
#include <string>

namespace aithon::runtime {

// Reduction budget per scheduling quantum
constexpr int REDUCTIONS_PER_SLICE = 2000;

enum class ActorState {
    RUNNABLE,      // Ready to run
    WAITING,       // Waiting for message
    RUNNING,       // Currently executing
    SUSPENDED,     // Suspended (e.g., await)
    EXITING,       // Graceful shutdown
    DEAD           // Crashed or terminated
};

class ActorProcess {
public:
    // Behavior function type (compiled from Python async def)
    using BehaviorFn = void (*)(ActorProcess*, void*);
    
private:
    // Process ID
    int pid_;
    
    // Isolated heap for this actor only
    ActorHeap heap_;
    
    // Mailbox - lock-free MPSC queue
    LockFreeQueue<Message> mailbox_;
    
    // Current state
    std::atomic<ActorState> state_;
    
    // Reduction counter (for preemptive scheduling)
    std::atomic<int> reductions_;
    
    // Linked supervisors for crash propagation
    int supervisor_pid_;
    std::vector<int> monitored_by_;
    int caller_pid_;  // For returning results
    
    // Exception info if crashed
    struct ExitReason {
        std::string error_msg;
        std::string stack_trace;
        uint64_t crash_time;
    } exit_reason_;
    
    // Continuation - where to resume execution
    void* continuation_state_;
    
    // Behavior function (compiled from Python async def)
    BehaviorFn behavior_;
    
    // Initial arguments
    void* initial_args_;
    
public:
    ActorProcess(int pid, size_t heap_size = 1024 * 1024);
    ~ActorProcess();
    
    // Prevent copying
    ActorProcess(const ActorProcess&) = delete;
    ActorProcess& operator=(const ActorProcess&) = delete;
    
    // Send message to this actor
    bool send(Message msg);
    
    // Receive message (returns nullptr if no message available)
    Message* receive();
    
    // Non-blocking receive with timeout
    Message* receive_timeout(uint64_t timeout_ms);
    
    // Execute one scheduling quantum
    bool execute_quantum();
    
    // Called by compiled code to check if we should yield
    bool should_yield();
    
    // Crash handling
    void handle_crash(const std::string& reason);
    
    // Getters
    int pid() const { return pid_; }
    ActorState state() const { return state_.load(); }
    bool is_alive() const;
    
    void set_behavior(BehaviorFn fn) { behavior_ = fn; }
    void set_supervisor(int pid) { supervisor_pid_ = pid; }
    void add_monitor(int pid) { monitored_by_.push_back(pid); }
    void set_caller(int pid) { caller_pid_ = pid; }
    void set_initial_args(void* args) { initial_args_ = args; }
    
    ActorHeap& heap() { return heap_; }
    
    // For debugging
    void dump_state() const;
    
private:
    static uint64_t get_monotonic_time();
};

} // namespace pyvm::runtime