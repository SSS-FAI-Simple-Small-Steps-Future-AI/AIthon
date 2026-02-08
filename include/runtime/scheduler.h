#pragma once

#include "actor_process.h"
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <random>
#include <unordered_map>
#include <memory>

namespace aithon::runtime {

class Scheduler {
private:
    // Worker thread
    struct Worker {
        std::thread thread;
        std::deque<ActorProcess*> run_queue;
        std::mutex queue_mutex;
        std::condition_variable queue_cv;
        std::atomic<size_t> queue_size{0};
        std::mt19937 rng;
        std::atomic<bool> running{true};
        
        Worker();
    };
    
    std::vector<std::unique_ptr<Worker>> workers_;
    size_t num_workers_;
    
    // Global actor registry
    std::unordered_map<int, std::unique_ptr<ActorProcess>> actors_;
    std::mutex actors_mutex_;
    std::atomic<int> next_pid_{0};
    
    // System running flag
    std::atomic<bool> system_running_{true};
    
    // Statistics
    std::atomic<uint64_t> total_messages_sent_{0};
    std::atomic<uint64_t> total_reductions_{0};
    std::atomic<uint64_t> total_actors_spawned_{0};
    
    // Migration threshold
    static constexpr size_t MIGRATION_THRESHOLD = 100;
    static constexpr size_t STEAL_THRESHOLD = 10;
    
public:
    explicit Scheduler(size_t num_threads = 0);
    ~Scheduler();
    
    // Prevent copying
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    
    // Spawn new actor
    int spawn(ActorProcess::BehaviorFn behavior, 
              void* initial_args = nullptr,
              size_t heap_size = 1024 * 1024);
    
    // Send message from one actor to another
    bool send_message(int from_pid, int to_pid, void* data, size_t size);
    
    // Kill an actor
    void kill_actor(int pid);
    
    // Get actor by PID (for debugging)
    ActorProcess* get_actor(int pid);
    
    // Shutdown scheduler
    void shutdown();
    
    // Wait for all actors to complete
    void wait_for_completion(uint64_t timeout_ms = 0);
    
    // Statistics
    size_t num_actors() const;
    size_t num_alive_actors() const;
    uint64_t total_messages() const { return total_messages_sent_.load(); }
    uint64_t total_reductions() const { return total_reductions_.load(); }
    
    // Dump statistics
    void dump_stats() const;
    
private:
    // Worker thread main loop
    void worker_loop(size_t worker_id);
    
    // Get next actor from worker's queue
    ActorProcess* get_next_actor(size_t worker_id);
    
    // Schedule actor on specific worker
    void schedule_actor(int pid, size_t worker_id);
    
    // Choose best worker for new actor
    size_t choose_worker();
    
    // Work stealing
    bool should_steal_work(size_t worker_id);
    void steal_work(size_t thief_id);
};

// Global scheduler instance
extern Scheduler* global_scheduler;

} // namespace pyvm::runtime