#include "../../include/runtime/actor_process.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>


namespace aithon::runtime {

ActorProcess::ActorProcess(int pid, size_t heap_size)
    : pid_(pid),
      heap_(heap_size),
      state_(ActorState::RUNNABLE),
      reductions_(REDUCTIONS_PER_SLICE),
      supervisor_pid_(-1),
      caller_pid_(-1), exit_reason_(),
      continuation_state_(nullptr),
      behavior_(nullptr),
      initial_args_(nullptr) {
}

ActorProcess::~ActorProcess() {
    // Cleanup will be handled by heap destructor
}

bool ActorProcess::send(Message msg) {
    // Copy message payload to our heap
    void* local_payload = heap_.allocate(msg.size);
    if (!local_payload) {
        // Heap full - trigger GC
        heap_.collect_garbage();
        local_payload = heap_.allocate(msg.size);
        if (!local_payload) {
            return false;  // Still no space
        }
    }
    
    std::memcpy(local_payload, msg.payload, msg.size);
    
    Message local_msg(local_payload, msg.size, msg.sender_pid);
    mailbox_.enqueue(std::move(local_msg));
    
    // Wake up if waiting
    ActorState expected = ActorState::WAITING;
    state_.compare_exchange_strong(expected, ActorState::RUNNABLE,
                                   std::memory_order_release,
                                   std::memory_order_relaxed);
    
    return true;
}

Message* ActorProcess::receive() {
    auto opt_msg = mailbox_.try_dequeue();
    if (opt_msg.has_value()) {
        // Allocate message on heap so it persists
        Message* msg = static_cast<Message*>(heap_.allocate(sizeof(Message)));
        if (msg) {
            new (msg) Message(std::move(opt_msg.value()));
            return msg;
        }
    }
    
    // No messages - suspend
    state_.store(ActorState::WAITING, std::memory_order_release);
    return nullptr;
}

Message* ActorProcess::receive_timeout(uint64_t timeout_ms) {
    uint64_t start = get_monotonic_time();
    
    while (true) {
        auto opt_msg = mailbox_.try_dequeue();
        if (opt_msg.has_value()) {
            Message* msg = static_cast<Message*>(heap_.allocate(sizeof(Message)));
            if (msg) {
                new (msg) Message(std::move(opt_msg.value()));
                return msg;
            }
        }
        
        if (get_monotonic_time() - start > timeout_ms) {
            return nullptr;  // Timeout
        }
        
        std::this_thread::yield();
    }
}

bool ActorProcess::execute_quantum() {
    ActorState expected = ActorState::RUNNABLE;
    if (!state_.compare_exchange_strong(expected, ActorState::RUNNING,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
        return false;  // Not ready to run
    }
    
    reductions_.store(REDUCTIONS_PER_SLICE, std::memory_order_relaxed);
    
    try {
        // Call behavior function (compiled Python code)
        if (behavior_) {
            behavior_(this, initial_args_);
        }
        
        // If we get here, actor yielded voluntarily or completed
        ActorState current = state_.load(std::memory_order_acquire);
        if (current == ActorState::RUNNING) {
            state_.store(ActorState::RUNNABLE, std::memory_order_release);
        }
        return true;
        
    } catch (const std::exception& e) {
        // Actor crashed - isolate the crash
        handle_crash(e.what());
        return false;
    } catch (...) {
        handle_crash("Unknown exception");
        return false;
    }
}

bool ActorProcess::should_yield() {
    int remaining = reductions_.fetch_sub(1, std::memory_order_relaxed);
    return remaining <= 0;
}

void ActorProcess::handle_crash(const std::string& reason) {
    state_.store(ActorState::DEAD, std::memory_order_release);
    exit_reason_.error_msg = reason;
    exit_reason_.crash_time = get_monotonic_time();
    
    std::cerr << "Actor " << pid_ << " crashed: " << reason << std::endl;
    
    // TODO: Notify supervisor
    // TODO: Notify monitors
}

bool ActorProcess::is_alive() const {
    ActorState s = state_.load(std::memory_order_acquire);
    return s != ActorState::DEAD && s != ActorState::EXITING;
}

void ActorProcess::dump_state() const {
    std::cout << "Actor " << pid_ << ":\n";
    std::cout << "  State: ";
    switch (state_.load()) {
        case ActorState::RUNNABLE: std::cout << "RUNNABLE"; break;
        case ActorState::WAITING: std::cout << "WAITING"; break;
        case ActorState::RUNNING: std::cout << "RUNNING"; break;
        case ActorState::SUSPENDED: std::cout << "SUSPENDED"; break;
        case ActorState::EXITING: std::cout << "EXITING"; break;
        case ActorState::DEAD: std::cout << "DEAD"; break;
    }
    std::cout << "\n";
    std::cout << "  Reductions: " << reductions_.load() << "\n";
    std::cout << "  Mailbox empty: " << mailbox_.is_empty() << "\n";
    heap_.dump_stats();
}

uint64_t ActorProcess::get_monotonic_time() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
}

} // namespace pyvm::runtime