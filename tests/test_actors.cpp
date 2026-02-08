#include "runtime/actor_process.h"
#include <iostream>
#include <cassert>

using namespace aithon::runtime;

void test_heap() {
    std::cout << "\n=== Test: Actor Heap ===\n";
    
    ActorHeap heap(1024 * 1024);  // 1MB
    
    void* ptr1 = heap.allocate(100);
    assert(ptr1 != nullptr);
    std::cout << "Allocated 100 bytes\n";
    
    void* ptr2 = heap.allocate(200);
    assert(ptr2 != nullptr);
    std::cout << "Allocated 200 bytes\n";
    
    heap.dump_stats();
    
    std::cout << "Test passed!\n";
}

void test_mailbox() {
    std::cout << "\n=== Test: Actor Mailbox ===\n";
    
    ActorProcess actor(1);
    
    int msg1 = 42;
    int msg2 = 100;
    
    Message m1(&msg1, sizeof(msg1), 0);
    Message m2(&msg2, sizeof(msg2), 0);
    
    actor.send(std::move(m1));
    actor.send(std::move(m2));
    
    Message* received1 = actor.receive();
    assert(received1 != nullptr);
    int val1 = *static_cast<int*>(received1->payload);
    std::cout << "Received: " << val1 << std::endl;
    assert(val1 == 42);
    
    Message* received2 = actor.receive();
    assert(received2 != nullptr);
    int val2 = *static_cast<int*>(received2->payload);
    std::cout << "Received: " << val2 << std::endl;
    assert(val2 == 100);
    
    std::cout << "Test passed!\n";
}

void test_actor_lifecycle() {
    std::cout << "\n=== Test: Actor Lifecycle ===\n";
    
    ActorProcess actor(1);
    
    std::cout << "Actor state: ";
    switch (actor.state()) {
        case ActorState::RUNNABLE: std::cout << "RUNNABLE\n"; break;
        case ActorState::WAITING: std::cout << "WAITING\n"; break;
        case ActorState::RUNNING: std::cout << "RUNNING\n"; break;
        case ActorState::DEAD: std::cout << "DEAD\n"; break;
        default: std::cout << "UNKNOWN\n";
    }
    
    assert(actor.is_alive());
    
    actor.handle_crash("Test crash");
    assert(!actor.is_alive());
    assert(actor.state() == ActorState::DEAD);
    
    std::cout << "Test passed!\n";
}

int main() {
    std::cout << "Running Actor Tests\n";
    std::cout << "===================\n";
    
    test_heap();
    test_mailbox();
    test_actor_lifecycle();
    
    std::cout << "\nAll tests passed!\n";
    return 0;
}