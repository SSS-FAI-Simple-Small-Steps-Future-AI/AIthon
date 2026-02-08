#include "../../include/runtime/green_threads.h"
#include <iostream>
#include <algorithm>

namespace aithon::runtime {

GreenThreadScheduler* global_green_scheduler = nullptr;

// ============================================================================
// GreenThread Implementation
// ============================================================================

GreenThread::GreenThread(int id, size_t heap_size)
    : thread_id_(id),
      state_(State::CREATED),
      private_heap_(std::make_unique<ActorHeap>(heap_size)),
      stack_pointer_(nullptr),
      instruction_pointer_(nullptr),
      has_crashed_(false),
      supervisor_id_(-1),
      behavior_(nullptr),
      initial_args_(nullptr) {
    
    // Initialize registers
    for (int i = 0; i < 16; i++) {
        registers_[i] = nullptr;
    }
    
    // Initialize GC stats
    gc_stats_.collections_count = 0;
    gc_stats_.objects_freed = 0;
    gc_stats_.bytes_freed = 0;
    gc_stats_.total_gc_time = std::chrono::microseconds(0);
}

GreenThread::~GreenThread() {
    // Heap is automatically cleaned up by unique_ptr
}

void GreenThread::set_behavior(ActorProcess::BehaviorFn fn, void* args) {
    behavior_ = fn;
    initial_args_ = args;
    state_ = State::READY;
}

bool GreenThread::execute_quantum() {
    if (state_ != State::READY && state_ != State::RUNNING) {
        return false;
    }
    
    if (!behavior_) {
        return false;
    }
    
    state_ = State::RUNNING;
    
    try {
        // Create a temporary ActorProcess for behavior execution
        // This allows reuse of existing behavior functions
        ActorProcess temp_actor(thread_id_, 0);  // 0 size since we use our own heap
        temp_actor.set_behavior(behavior_);
        
        // Execute the behavior
        behavior_(&temp_actor, initial_args_);
        
        // Check if we should auto-run GC
        auto_gc_check();
        
        // Back to ready state (or terminated if behavior completed)
        if (state_ == State::RUNNING) {
            state_ = State::READY;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        crash(std::string("Exception: ") + e.what());
        return false;
    } catch (...) {
        crash("Unknown exception");
        return false;
    }
}

bool GreenThread::send_message(const Message& msg) {
    if (has_crashed_ || state_ == State::TERMINATED) {
        return false;
    }
    
    // Copy message to our private heap
    void* local_data = private_heap_->allocate(msg.size);
    if (!local_data) {
        // Try GC and allocate again
        run_gc();
        local_data = private_heap_->allocate(msg.size);
        if (!local_data) {
            return false;  // Out of memory
        }
    }
    
    std::memcpy(local_data, msg.payload, msg.size);
    
    Message local_msg(local_data, msg.size, msg.sender_pid);
    mailbox_.enqueue(std::move(local_msg));
    
    // Wake up if blocked
    if (state_ == State::BLOCKED) {
        state_ = State::READY;
    }
    
    return true;
}

Message* GreenThread::receive_message() {
    auto opt_msg = mailbox_.try_dequeue();
    if (opt_msg.has_value()) {
        Message* msg = static_cast<Message*>(private_heap_->allocate(sizeof(Message)));
        if (msg) {
            new (msg) Message(std::move(opt_msg.value()));
            return msg;
        }
    }
    
    // No messages - block
    if (state_ == State::RUNNING) {
        state_ = State::BLOCKED;
    }
    
    return nullptr;
}

void* GreenThread::allocate(size_t size) {
    void* ptr = private_heap_->allocate(size);
    if (!ptr) {
        // Try GC
        run_gc();
        ptr = private_heap_->allocate(size);
    }
    return ptr;
}

void GreenThread::run_gc() {
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t before = private_heap_->used();
    mark_and_sweep();
    size_t after = private_heap_->used();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    update_gc_stats(duration, before - after);
}

void GreenThread::auto_gc_check() {
    // Run GC if heap is more than 80% full
    double usage = static_cast<double>(private_heap_->used()) / private_heap_->total();
    if (usage > 0.8) {
        run_gc();
    }
}

void GreenThread::mark_and_sweep() {
    private_heap_->collect_garbage();
}

void GreenThread::update_gc_stats(std::chrono::microseconds duration, size_t freed) {
    gc_stats_.collections_count++;
    gc_stats_.bytes_freed += freed;
    gc_stats_.total_gc_time += duration;
    
    // Estimate objects freed (rough approximation)
    gc_stats_.objects_freed += freed / 64;  // Assume avg object size 64 bytes
}

void GreenThread::crash(const std::string& reason) {
    has_crashed_ = true;
    crash_reason_ = reason;
    state_ = State::TERMINATED;
    
    std::cerr << "Green Thread " << thread_id_ << " crashed: " << reason << std::endl;
    
    // Notify supervisor if any
    if (supervisor_id_ != -1 && global_green_scheduler) {
        // TODO: Send crash notification to supervisor
    }
}

void GreenThread::save_context() {
    // Save execution context for preemption
    // In a real implementation, this would save CPU registers
}

void GreenThread::restore_context() {
    // Restore execution context
}

// ============================================================================
// GreenThreadScheduler Implementation
// ============================================================================

GreenThreadScheduler::GreenThreadScheduler(size_t num_workers, SchedulingPolicy policy)
    : policy_(policy) {
    
    if (num_workers == 0) {
        num_workers = std::thread::hardware_concurrency();
        if (num_workers == 0) num_workers = 4;
    }
    
    num_workers_ = num_workers;
    workers_.reserve(num_workers_);
    
    std::cout << "Initializing Green Thread Scheduler\n";
    std::cout << "  Workers: " << num_workers_ << "\n";
    std::cout << "  Policy: " << (policy_ == SchedulingPolicy::WORK_STEALING ? 
                                  "Work Stealing" : "Round Robin") << "\n";
    std::cout << "  Green Threads: Enabled (M:N threading)\n";
    std::cout << "  Garbage Collection: Per-thread GC\n";
    std::cout << "  Memory Isolation: Fully isolated heaps\n";
    std::cout << std::endl;
}

GreenThreadScheduler::~GreenThreadScheduler() {
    stop();
}

void GreenThreadScheduler::start() {
    for (size_t i = 0; i < num_workers_; ++i) {
        auto worker = std::make_unique<WorkerThread>();
        worker->running.store(true);
        worker->threads_executed.store(0);
        worker->context_switches.store(0);
        worker->messages_processed.store(0);
        
        worker->thread = std::thread(&GreenThreadScheduler::worker_loop, this, i);
        workers_.push_back(std::move(worker));
    }
}

void GreenThreadScheduler::stop() {
    // Signal all workers to stop
    for (auto& worker : workers_) {
        worker->running.store(false);
        worker->queue_cv.notify_all();
    }
    
    // Wait for all workers to finish
    for (auto& worker : workers_) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

int GreenThreadScheduler::spawn(ActorProcess::BehaviorFn behavior,
                               void* args,
                               size_t heap_size) {
    int thread_id = next_thread_id_.fetch_add(1);
    
    auto green_thread = std::make_unique<GreenThread>(thread_id, heap_size);
    green_thread->set_behavior(behavior, args);
    
    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        green_threads_[thread_id] = std::move(green_thread);
    }
    
    total_green_threads_created_.fetch_add(1);
    
    // Schedule on a worker
    size_t worker_id = choose_worker();
    GreenThread* thread_ptr = green_threads_[thread_id].get();
    schedule_thread(thread_ptr, worker_id);
    
    return thread_id;
}

void GreenThreadScheduler::terminate(int thread_id) {
    std::lock_guard<std::mutex> lock(threads_mutex_);
    auto it = green_threads_.find(thread_id);
    if (it != green_threads_.end()) {
        it->second->set_state(GreenThread::State::TERMINATED);
    }
}

bool GreenThreadScheduler::send_message(int from_id, int to_id, const Message& msg) {
    GreenThread* to_thread = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        auto it = green_threads_.find(to_id);
        if (it != green_threads_.end()) {
            to_thread = it->second.get();
        }
    }
    
    if (!to_thread || !to_thread->is_alive()) {
        return false;
    }
    
    bool sent = to_thread->send_message(msg);
    if (sent) {
        total_messages_sent_.fetch_add(1);
        
        // Wake up worker if thread was blocked
        for (auto& worker : workers_) {
            worker->queue_cv.notify_one();
        }
    }
    
    return sent;
}

GreenThread* GreenThreadScheduler::get_thread(int thread_id) {
    std::lock_guard<std::mutex> lock(threads_mutex_);
    auto it = green_threads_.find(thread_id);
    if (it != green_threads_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void GreenThreadScheduler::worker_loop(size_t worker_id) {
    WorkerThread& worker = *workers_[worker_id];
    
    while (worker.running.load()) {
        // Get next ready thread
        GreenThread* thread = get_next_ready_thread(worker_id);
        
        if (thread) {
            // Execute one quantum
            thread->execute_quantum();
            
            worker.threads_executed.fetch_add(1);
            worker.context_switches.fetch_add(1);
            
            // Reschedule if still ready
            if (thread->state() == GreenThread::State::READY) {
                schedule_thread(thread, worker_id);
            }
            
        } else {
            // No work - try to steal or wait
            if (policy_ == SchedulingPolicy::WORK_STEALING) {
                if (!try_steal_work(worker_id)) {
                    // Nothing to steal - wait
                    std::unique_lock<std::mutex> lock(worker.queue_mutex);
                    worker.queue_cv.wait_for(lock, std::chrono::milliseconds(10));
                }
            } else {
                std::unique_lock<std::mutex> lock(worker.queue_mutex);
                worker.queue_cv.wait_for(lock, std::chrono::milliseconds(10));
            }
        }
        
        // Move blocked threads back to ready if they have messages
        move_blocked_to_ready(worker_id);
    }
}

void GreenThreadScheduler::schedule_thread(GreenThread* thread, size_t preferred_worker) {
    if (preferred_worker >= workers_.size()) {
        preferred_worker = 0;
    }
    
    WorkerThread& worker = *workers_[preferred_worker];
    
    std::lock_guard<std::mutex> lock(worker.queue_mutex);
    if (thread->state() == GreenThread::State::READY) {
        worker.ready_queue.push_back(thread);
    } else if (thread->state() == GreenThread::State::BLOCKED) {
        worker.blocked_queue.push_back(thread);
    }
    
    worker.queue_cv.notify_one();
}

GreenThread* GreenThreadScheduler::get_next_ready_thread(size_t worker_id) {
    WorkerThread& worker = *workers_[worker_id];
    std::lock_guard<std::mutex> lock(worker.queue_mutex);
    
    if (!worker.ready_queue.empty()) {
        GreenThread* thread = worker.ready_queue.front();
        worker.ready_queue.pop_front();
        return thread;
    }
    
    return nullptr;
}

void GreenThreadScheduler::move_blocked_to_ready(size_t worker_id) {
    WorkerThread& worker = *workers_[worker_id];
    std::lock_guard<std::mutex> lock(worker.queue_mutex);
    
    auto it = worker.blocked_queue.begin();
    while (it != worker.blocked_queue.end()) {
        if ((*it)->has_messages()) {
            (*it)->set_state(GreenThread::State::READY);
            worker.ready_queue.push_back(*it);
            it = worker.blocked_queue.erase(it);
        } else {
            ++it;
        }
    }
}

bool GreenThreadScheduler::try_steal_work(size_t thief_id) {
    // Try to steal from another worker
    for (size_t i = 0; i < workers_.size(); ++i) {
        if (i == thief_id) continue;
        
        WorkerThread& victim = *workers_[i];
        std::lock_guard<std::mutex> lock(victim.queue_mutex);
        
        if (victim.ready_queue.size() > 1) {
            // Steal from back
            GreenThread* stolen = victim.ready_queue.back();
            victim.ready_queue.pop_back();
            
            // Add to our queue
            WorkerThread& thief = *workers_[thief_id];
            std::lock_guard<std::mutex> thief_lock(thief.queue_mutex);
            thief.ready_queue.push_back(stolen);
            
            return true;
        }
    }
    
    return false;
}

size_t GreenThreadScheduler::choose_worker() {
    // Choose worker with smallest ready queue
    size_t min_size = SIZE_MAX;
    size_t chosen = 0;
    
    for (size_t i = 0; i < workers_.size(); ++i) {
        std::lock_guard<std::mutex> lock(workers_[i]->queue_mutex);
        size_t size = workers_[i]->ready_queue.size();
        if (size < min_size) {
            min_size = size;
            chosen = i;
        }
    }
    
    return chosen;
}

void GreenThreadScheduler::dump_statistics() const {
    std::cout << "\n=== Green Thread Scheduler Statistics ===\n";
    std::cout << "Total green threads created: " << total_green_threads_created_.load() << "\n";
    std::cout << "Currently alive threads: " << num_alive_threads() << "\n";
    std::cout << "Total messages sent: " << total_messages_sent_.load() << "\n";
    std::cout << "Total memory used: " << (total_memory_used() / 1024 / 1024) << " MB\n";
    
    std::cout << "\nWorker Statistics:\n";
    for (size_t i = 0; i < workers_.size(); ++i) {
        const auto& worker = *workers_[i];
        std::cout << "  Worker " << i << ":\n";
        std::cout << "    Threads executed: " << worker.threads_executed.load() << "\n";
        std::cout << "    Context switches: " << worker.context_switches.load() << "\n";
        std::cout << "    Messages processed: " << worker.messages_processed.load() << "\n";
    }
    
    std::cout << "=========================================\n\n";
}

size_t GreenThreadScheduler::num_alive_threads() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(threads_mutex_));
    size_t count = 0;
    for (const auto& [id, thread] : green_threads_) {
        if (thread->is_alive()) {
            count++;
        }
    }
    return count;
}

size_t GreenThreadScheduler::total_memory_used() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(threads_mutex_));
    size_t total = 0;
    for (const auto& [id, thread] : green_threads_) {
        total += thread->memory_used();
    }
    return total;
}

// Convenience functions
int spawn_green_thread(ActorProcess::BehaviorFn behavior, void* args) {
    if (!global_green_scheduler) {
        global_green_scheduler = new GreenThreadScheduler();
        global_green_scheduler->start();
    }
    return global_green_scheduler->spawn(behavior, args);
}

bool send_to_thread(int from_id, int to_id, void* data, size_t size) {
    if (!global_green_scheduler) return false;
    
    Message msg(data, size, from_id);
    return global_green_scheduler->send_message(from_id, to_id, msg);
}

} // namespace pyvm::runtime