#include "runtime/scheduler.h"
#include "runtime/actor_process.h"
#include <iostream>
#include <cstring>

// C API for compiled code to call

extern "C" {

using namespace pyvm::runtime;

// Spawn a new actor
int runtime_spawn_actor(void (*behavior)(void*, void*), void* args) {
    if (!global_scheduler) {
        std::cerr << "Error: Scheduler not initialized" << std::endl;
        return -1;
    }
    
    // Wrap the behavior function
    auto wrapped_behavior = [behavior, args](ActorProcess* actor, void* _args) {
        behavior(actor, args);
    };
    
    // For now, create a simple wrapper
    ActorProcess::BehaviorFn behavior_fn = 
        reinterpret_cast<ActorProcess::BehaviorFn>(behavior);
    
    return global_scheduler->spawn(behavior_fn, args);
}

// Send message to an actor
bool runtime_send_message(int from_pid, int to_pid, void* data, size_t size) {
    if (!global_scheduler) {
        return false;
    }
    
    return global_scheduler->send_message(from_pid, to_pid, data, size);
}

// Receive message (blocking)
void* runtime_receive_message() {
    // This would need to access the current actor's mailbox
    // For now, return nullptr
    return nullptr;
}

// Check if should yield
bool runtime_should_yield() {
    // This would access the current actor's reduction counter
    return false;
}

// Print functions
void runtime_print_int(int64_t value) {
    std::cout << value << std::endl;
}

void runtime_print_float(double value) {
    std::cout << value << std::endl;
}

void runtime_print_string(const char* str) {
    std::cout << str << std::endl;
}

// Initialize the runtime system
void runtime_init(int num_workers) {
    if (!global_scheduler) {
        new Scheduler(num_workers);
    }
}

// Shutdown the runtime
void runtime_shutdown() {
    if (global_scheduler) {
        global_scheduler->shutdown();
        delete global_scheduler;
        global_scheduler = nullptr;
    }
}

// Wait for all actors to complete
void runtime_wait() {
    if (global_scheduler) {
        global_scheduler->wait_for_completion(10000);  // 10 second timeout
    }
}

// Dump statistics
void runtime_dump_stats() {
    if (global_scheduler) {
        global_scheduler->dump_stats();
    }
}

} // extern "C"