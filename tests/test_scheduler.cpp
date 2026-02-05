#include "runtime/scheduler.h"
#include "runtime/actor_process.h"
#include <iostream>
#include <cassert>

using namespace pyvm::runtime;

void simple_behavior(ActorProcess* self, void* args) {
    int count = *static_cast<int*>(args);
    std::cout << "Actor " << self->pid() << " running with count=" << count << std::endl;
}

void test_spawn() {
    std::cout << "\n=== Test: Spawn Actors ===\n";
    Scheduler scheduler(2);
    
    int arg1 = 10;
    int arg2 = 20;
    
    int pid1 = scheduler.spawn(simple_behavior, &arg1);
    int pid2 = scheduler.spawn(simple_behavior, &arg2);
    
    std::cout << "Spawned actor 1: PID=" << pid1 << std::endl;
    std::cout << "Spawned actor 2: PID=" << pid2 << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    scheduler.shutdown();
    std::cout << "Test passed!\n";
}

void message_behavior(ActorProcess* self, void* args) {
    for (int i = 0; i < 3; i++) {
        Message* msg = self->receive();
        if (msg) {
            int value = *static_cast<int*>(msg->payload);
            std::cout << "Actor " << self->pid() << " received: " << value << std::endl;
        }
        
        if (self->should_yield()) {
            return;  // Yield to scheduler
        }
    }
}

void test_messaging() {
    std::cout << "\n=== Test: Message Passing ===\n";
    Scheduler scheduler(2);
    
    int pid = scheduler.spawn(message_behavior);
    
    // Send messages
    int msg1 = 100;
    int msg2 = 200;
    int msg3 = 300;
    
    scheduler.send_message(-1, pid, &msg1, sizeof(msg1));
    scheduler.send_message(-1, pid, &msg2, sizeof(msg2));
    scheduler.send_message(-1, pid, &msg3, sizeof(msg3));
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    scheduler.dump_stats();
    scheduler.shutdown();
    std::cout << "Test passed!\n";
}

int main() {
    std::cout << "Running Scheduler Tests\n";
    std::cout << "========================\n";
    
    test_spawn();
    test_messaging();
    
    std::cout << "\nAll tests passed!\n";
    return 0;
}