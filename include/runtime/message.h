#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace aithon::runtime {

    struct Message {
        void* payload;
        size_t size;
        int sender_pid;
        uint64_t timestamp;

        Message() : payload(nullptr), size(0), sender_pid(-1), timestamp(0) {}

        Message(void* data, size_t sz, int from)
            : payload(data), size(sz), sender_pid(from) {
            auto now = std::chrono::steady_clock::now();
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count();
        }

        Message(const Message& other) = delete;
        Message& operator=(const Message& other) = delete;

        Message(Message&& other) noexcept
            : payload(other.payload),
              size(other.size),
              sender_pid(other.sender_pid),
              timestamp(other.timestamp) {
            other.payload = nullptr;
            other.size = 0;
        }

        Message& operator=(Message&& other) noexcept {
            if (this != &other) {
                payload = other.payload;
                size = other.size;
                sender_pid = other.sender_pid;
                timestamp = other.timestamp;

                other.payload = nullptr;
                other.size = 0;
            }
            return *this;
        }

        ~Message() = default;
    };

} // namespace aithon::runtime