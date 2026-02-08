#include "../../include/runtime/scheduler.h"
#include <iostream>
#include <algorithm>

namespace aithon::runtime {

// Global scheduler instance
Scheduler* global_scheduler = nullptr;

Scheduler::Worker::Worker() : rng(std::random_device{}()) {}

Scheduler::Scheduler(size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // Fallback
    }
    
    num_workers_ = num_threads;
    workers_.reserve(num_workers_);
    
    std::cout << "Starting scheduler with " << num_workers_ << " workers" << std::endl;
    
    for (size_t i = 0; i < num_workers_; ++i) {
        workers_.push_back(std::make_unique<Worker>());
        workers_[i]->thread = std::thread(&Scheduler::worker_loop, this, i);
    }
    
    global_scheduler = this;
}

Scheduler::~Scheduler() {
    shutdown();
    global_scheduler = nullptr;
}

int Scheduler::spawn(ActorProcess::BehaviorFn behavior, void* initial_args, size_t heap_size) {
    int pid = next_pid_.fetch_add(1, std::memory_order_relaxed);
    
    auto actor = std::make_unique<ActorProcess>(pid, heap_size);
    actor->set_behavior(behavior);
    actor->set_initial_args(initial_args);
    
    // Choose worker with smallest queue
    size_t chosen_worker = choose_worker();
    
    {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        actors_[pid] = std::move(actor);
    }
    
    total_actors_spawned_.fetch_add(1, std::memory_order_relaxed);
    schedule_actor(pid, chosen_worker);
    
    return pid;
}

bool Scheduler::send_message(int from_pid, int to_pid, void* data, size_t size) {
    ActorProcess* to_actor = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        auto it = actors_.find(to_pid);
        if (it == actors_.end() || !it->second->is_alive()) {
            return false;  // Actor doesn't exist or is dead
        }
        to_actor = it->second.get();
    }
    
    Message msg(data, size, from_pid);
    bool sent = to_actor->send(std::move(msg));
    
    if (sent) {
        total_messages_sent_.fetch_add(1, std::memory_order_relaxed);
        
        // If actor was waiting, it's now runnable - wake workers
        if (to_actor->state() == ActorState::RUNNABLE) {
            for (size_t i = 0; i < num_workers_; ++i) {
                workers_[i]->queue_cv.notify_one();
            }
        }
    }
    
    return sent;
}

void Scheduler::kill_actor(int pid) {
    std::lock_guard<std::mutex> lock(actors_mutex_);
    auto it = actors_.find(pid);
    if (it != actors_.end()) {
        it->second->handle_crash("killed");
    }
}

ActorProcess* Scheduler::get_actor(int pid) {
    std::lock_guard<std::mutex> lock(actors_mutex_);
    auto it = actors_.find(pid);
    if (it != actors_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Scheduler::shutdown() {
    system_running_.store(false, std::memory_order_release);
    
    // Wake all workers
    for (auto& worker : workers_) {
        worker->running.store(false, std::memory_order_release);
        worker->queue_cv.notify_all();
    }
    
    // Join all workers
    for (auto& worker : workers_) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
    
    std::cout << "Scheduler shutdown complete" << std::endl;
}

void Scheduler::wait_for_completion(uint64_t timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    
    while (system_running_.load(std::memory_order_acquire)) {
        size_t alive = num_alive_actors();
        if (alive == 0) {
            break;
        }
        
        if (timeout_ms > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start
            ).count();
            if (elapsed >= timeout_ms) {
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

size_t Scheduler::num_actors() const {
    std::lock_guard<std::mutex> lock(
        const_cast<std::mutex&>(actors_mutex_)
    );
    return actors_.size();
}

size_t Scheduler::num_alive_actors() const {
    std::lock_guard<std::mutex> lock(
        const_cast<std::mutex&>(actors_mutex_)
    );
    size_t count = 0;
    for (const auto& [pid, actor] : actors_) {
        if (actor->is_alive()) {
            count++;
        }
    }
    return count;
}

void Scheduler::dump_stats() const {
    std::cout << "\n=== Scheduler Statistics ===\n";
    std::cout << "Total actors spawned: " << total_actors_spawned_.load() << "\n";
    std::cout << "Current actors: " << num_actors() << "\n";
    std::cout << "Alive actors: " << num_alive_actors() << "\n";
    std::cout << "Total messages sent: " << total_messages_sent_.load() << "\n";
    std::cout << "Total reductions: " << total_reductions_.load() << "\n";
    std::cout << "Workers: " << num_workers_ << "\n";
    
    for (size_t i = 0; i < num_workers_; ++i) {
        std::cout << "  Worker " << i << " queue size: " 
                  << workers_[i]->queue_size.load() << "\n";
    }
    std::cout << "===========================\n\n";
}

void Scheduler::worker_loop(size_t worker_id) {
    Worker& worker = *workers_[worker_id];
    
    while (worker.running.load(std::memory_order_acquire)) {
        ActorProcess* actor = get_next_actor(worker_id);
        
        if (actor) {
            // Execute one quantum
            bool should_reschedule = actor->execute_quantum();
            
            if (should_reschedule && actor->is_alive()) {
                // Put back in queue if still runnable
                if (actor->state() == ActorState::RUNNABLE) {
                    schedule_actor(actor->pid(), worker_id);
                }
            }
            
            total_reductions_.fetch_add(REDUCTIONS_PER_SLICE, std::memory_order_relaxed);
            
            // Check for work stealing
            if (should_steal_work(worker_id)) {
                steal_work(worker_id);
            }
            
        } else {
            // No work - wait or steal
            std::unique_lock<std::mutex> lock(worker.queue_mutex);
            worker.queue_cv.wait_for(
                lock,
                std::chrono::milliseconds(10),
                [&worker] { 
                    return worker.queue_size.load(std::memory_order_relaxed) > 0 || 
                           !worker.running.load(std::memory_order_acquire);
                }
            );
        }
    }
}

ActorProcess* Scheduler::get_next_actor(size_t worker_id) {
    Worker& worker = *workers_[worker_id];
    std::lock_guard<std::mutex> lock(worker.queue_mutex);
    
    if (worker.run_queue.empty()) {
        return nullptr;
    }
    
    ActorProcess* actor = worker.run_queue.front();
    worker.run_queue.pop_front();
    worker.queue_size.fetch_sub(1, std::memory_order_relaxed);
    
    return actor;
}

void Scheduler::schedule_actor(int pid, size_t worker_id) {
    ActorProcess* actor = nullptr;
    {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        auto it = actors_.find(pid);
        if (it != actors_.end()) {
            actor = it->second.get();
        }
    }
    
    if (!actor) return;
    
    Worker& worker = *workers_[worker_id];
    {
        std::lock_guard<std::mutex> lock(worker.queue_mutex);
        worker.run_queue.push_back(actor);
        worker.queue_size.fetch_add(1, std::memory_order_relaxed);
    }
    worker.queue_cv.notify_one();
}

size_t Scheduler::choose_worker() {
    size_t min_size = SIZE_MAX;
    size_t chosen = 0;
    
    for (size_t i = 0; i < num_workers_; ++i) {
        size_t size = workers_[i]->queue_size.load(std::memory_order_relaxed);
        if (size < min_size) {
            min_size = size;
            chosen = i;
        }
    }
    
    return chosen;
}

bool Scheduler::should_steal_work(size_t worker_id) {
    Worker& worker = *workers_[worker_id];
    
    // Steal if our queue is empty or very small
    if (worker.queue_size.load(std::memory_order_relaxed) < 2) {
        // Check if others have lots of work
        for (size_t i = 0; i < num_workers_; ++i) {
            if (i != worker_id && 
                workers_[i]->queue_size.load(std::memory_order_relaxed) > STEAL_THRESHOLD) {
                return true;
            }
        }
    }
    
    return false;
}

void Scheduler::steal_work(size_t thief_id) {
    Worker& thief = *workers_[thief_id];
    
    // Random work stealing
    std::uniform_int_distribution<size_t> dist(0, num_workers_ - 1);
    size_t victim_id = dist(thief.rng);
    
    if (victim_id == thief_id) return;
    
    Worker& victim = *workers_[victim_id];
    
    // Try to steal half of victim's queue
    std::lock_guard<std::mutex> victim_lock(victim.queue_mutex);
    size_t steal_count = victim.run_queue.size() / 2;
    
    if (steal_count > 0) {
        std::lock_guard<std::mutex> thief_lock(thief.queue_mutex);
        
        for (size_t i = 0; i < steal_count; ++i) {
            ActorProcess* actor = victim.run_queue.back();
            victim.run_queue.pop_back();
            thief.run_queue.push_back(actor);
        }
        
        victim.queue_size.fetch_sub(steal_count, std::memory_order_relaxed);
        thief.queue_size.fetch_add(steal_count, std::memory_order_relaxed);
    }
}

} // namespace pyvm::runtime