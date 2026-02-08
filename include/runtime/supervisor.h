#pragma once

#include "actor_process.h"
#include "scheduler.h"
#include <map>
#include <vector>
#include <functional>
#include <chrono>

namespace aithon::runtime {

// Restart strategy for supervision
enum class RestartStrategy {
    ONE_FOR_ONE,      // Restart only the failed child
    ONE_FOR_ALL,      // Restart all children
    REST_FOR_ONE,     // Restart failed child and all started after it
    SIMPLE_ONE_FOR_ONE // For dynamic children
};

// Child specification
struct ChildSpec {
    std::string id;
    ActorProcess::BehaviorFn start_func;
    void* start_args;
    RestartStrategy restart;
    int max_restarts;
    std::chrono::seconds max_time;
    bool permanent;  // Always restart
    bool temporary;  // Never restart
    bool transient;  // Restart only if abnormal termination
};

// Child state tracking
struct ChildState {
    int pid;
    ChildSpec spec;
    int restart_count;
    std::chrono::time_point<std::chrono::steady_clock> last_restart;
    bool is_alive;
};

// Supervisor actor
class Supervisor {
private:
    int supervisor_pid_;
    RestartStrategy strategy_;
    int max_restarts_;
    std::chrono::seconds max_time_;

    // Children tracking
    std::map<std::string, ChildState> children_;
    std::vector<std::string> child_order_;  // For REST_FOR_ONE strategy

    // Restart intensity tracking
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> restart_times_;

    // Scheduler reference
    Scheduler* scheduler_;

public:
    Supervisor(Scheduler* sched, int pid, RestartStrategy strategy,
              int max_restarts = 5,
              std::chrono::seconds max_time = std::chrono::seconds(60))
        : scheduler_(sched),
          supervisor_pid_(pid),
          strategy_(strategy),
          max_restarts_(max_restarts),
          max_time_(max_time) {}

    // Add child to supervision tree
    bool add_child(const ChildSpec& spec);

    // Remove child from supervision tree
    bool remove_child(const std::string& id);

    // Start a child
    bool start_child(const std::string& id);

    // Stop a child
    bool stop_child(const std::string& id);

    // Restart a child
    bool restart_child(const std::string& id);

    // Handle child failure
    void handle_child_exit(int child_pid, const std::string& reason);

    // Get all children
    std::vector<std::string> get_children() const;

    // Get child state
    const ChildState* get_child_state(const std::string& id) const;

    // Check if restart intensity is exceeded
    bool restart_intensity_exceeded();

    // Terminate all children
    void terminate_all_children();

private:
    // Restart strategies implementation
    void restart_one_for_one(const std::string& failed_id);
    void restart_one_for_all();
    void restart_rest_for_one(const std::string& failed_id);

    // Helper to actually restart a child
    bool do_restart_child(const std::string& id);

    // Check if should restart based on child spec
    bool should_restart(const ChildState& child, const std::string& reason);

    // Record restart
    void record_restart();

    // Clean old restart records
    void cleanup_restart_records();
};

// Supervisor tree builder
class SupervisorTreeBuilder {
public:
    SupervisorTreeBuilder() = default;

    // Create supervisor with strategy
    int create_supervisor(Scheduler* sched,
                         RestartStrategy strategy,
                         int max_restarts = 5,
                         std::chrono::seconds max_time = std::chrono::seconds(60));

    // Add child to supervisor
    bool add_child_to_supervisor(int supervisor_pid,
                                 const ChildSpec& spec);

    // Create nested supervisor
    int create_child_supervisor(int parent_pid,
                               const std::string& id,
                               RestartStrategy strategy);

private:
    std::map<int, std::unique_ptr<Supervisor>> supervisors_;
};

// Global supervisor tree
extern SupervisorTreeBuilder* global_supervisor_tree;

// Convenience functions
int spawn_supervised(Scheduler* sched,
                    int supervisor_pid,
                    const std::string& id,
                    ActorProcess::BehaviorFn behavior,
                    void* args = nullptr);

void link_actor_to_supervisor(int supervisor_pid, int actor_pid);

} // namespace pyvm::runtime