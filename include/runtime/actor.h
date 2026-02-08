// runtime/actor.h
#pragma once
#include <queue>
#include <functional>
#include <memory>
#include <atomic>

namespace aithon::runtime {

    class Message {
    public:
        void* data;
        size_t size;
        int sender_pid;

        Message(void* d, size_t s, int pid)
            : data(d), size(s), sender_pid(pid) {}
    };

    class Actor {
    private:
        int pid;
        std::queue<Message> mailbox;
        std::mutex mailbox_mutex;
        std::function<void(Message&)> behavior;
        std::atomic<bool> is_alive{true};

    public:
        Actor(int id) : pid(id) {}

        void send(Message msg) {
            std::lock_guard<std::mutex> lock(mailbox_mutex);
            mailbox.push(std::move(msg));
        }

        void process_messages() {
            while (is_alive) {
                Message msg = receive();
                if (behavior) {
                    behavior(msg);
                }
            }
        }

        Message receive() {
            std::unique_lock<std::mutex> lock(mailbox_mutex);
            // Wait for message (condition variable in real impl)
            while (mailbox.empty() && is_alive) {
                lock.unlock();
                std::this_thread::yield();
                lock.lock();
            }

            if (!mailbox.empty()) {
                Message msg = std::move(mailbox.front());
                mailbox.pop();
                return msg;
            }

            return Message(nullptr, 0, -1); // Dead actor
        }

        void set_behavior(std::function<void(Message&)> fn) {
            behavior = fn;
        }

        void kill() { is_alive = false; }
        int get_pid() const { return pid; }
    };

} // namespace pyvm::runtime