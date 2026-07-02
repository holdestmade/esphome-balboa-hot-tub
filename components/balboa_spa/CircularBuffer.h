#pragma once

#include <stdint.h>
#include <cstddef>
#include "esphome/core/log.h"

namespace esphome
{
    namespace balboa_spa
    {

        inline constexpr const char *CIRCULAR_BUFFER_TAG = "BalboaSpa.CircularBuffer";

        // Fixed-capacity ring buffer backed by a plain array.
        // No heap allocation: suitable for per-byte pushes on embedded targets.
        // Out-of-range accesses are logged and return a default-constructed T
        // instead of invoking undefined behaviour.
        template <typename T, size_t s>
        class CircularBuffer
        {
        private:
            T storage_[s] = {};
            size_t head_ = 0;  // index of the first (oldest) element
            size_t count_ = 0; // number of stored elements

            size_t physical_index(size_t logical_index) const
            {
                return (head_ + logical_index) % s;
            }

            CircularBuffer(const CircularBuffer &) = delete;
            CircularBuffer(CircularBuffer &&) = delete;
            CircularBuffer &operator=(const CircularBuffer &) = delete;
            CircularBuffer &operator=(CircularBuffer &&) = delete;

        public:
            CircularBuffer() = default;

            size_t size() const
            {
                return count_;
            }

            void clear()
            {
                head_ = 0;
                count_ = 0;
            }

            // Append at the back. When full, the oldest element is evicted and
            // false is returned.
            bool push(T val)
            {
                if (count_ >= s)
                {
                    // Buffer full: overwrite the oldest slot, which becomes the back.
                    storage_[head_] = val;
                    head_ = (head_ + 1) % s;
                    return false;
                }
                storage_[physical_index(count_)] = val;
                count_++;
                return true;
            }

            // Prepend at the front. Fails (returns false) when full.
            bool unshift(T val)
            {
                if (count_ >= s)
                {
                    return false;
                }
                head_ = (head_ + s - 1) % s;
                storage_[head_] = val;
                count_++;
                return true;
            }

            // Remove and return the back element.
            T pop()
            {
                if (count_ == 0)
                {
                    ESP_LOGE(CIRCULAR_BUFFER_TAG, "pop() on empty buffer");
                    return T();
                }
                count_--;
                return storage_[physical_index(count_)];
            }

            T first() const
            {
                if (count_ == 0)
                {
                    ESP_LOGE(CIRCULAR_BUFFER_TAG, "first() on empty buffer");
                    return T();
                }
                return storage_[head_];
            }

            T last() const
            {
                if (count_ == 0)
                {
                    ESP_LOGE(CIRCULAR_BUFFER_TAG, "last() on empty buffer");
                    return T();
                }
                return storage_[physical_index(count_ - 1)];
            }

            T operator[](size_t index) const
            {
                if (index >= count_)
                {
                    ESP_LOGE(CIRCULAR_BUFFER_TAG, "INDEX %u out of bounds. size=%u",
                             static_cast<unsigned>(index), static_cast<unsigned>(count_));
                    return T();
                }
                return storage_[physical_index(index)];
            }
        };

    } // ns balboa_spa
} // ns esphome
