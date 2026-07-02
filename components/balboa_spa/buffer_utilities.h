#pragma once

#include <stdint.h>
#include <cstddef>
#include "CircularBuffer.h"
#include "esphome/core/log.h"

// BufferUtilities - Safe buffer access helpers for the Balboa component.

namespace esphome
{
    namespace balboa_spa
    {

        inline constexpr const char *BUFFER_UTILS_TAG = "BalboaSpa.Buffer";

        // Return true if `buf` contains at least `min_size` bytes.
        // Logs a warning (with the supplied context) and returns false otherwise.
        template <size_t S>
        inline bool buffer_has_minimum_size(const CircularBuffer<uint8_t, S> &buf,
                                            size_t min_size,
                                            const char *context = "")
        {
            if (buf.size() < min_size)
            {
                ESP_LOGW(BUFFER_UTILS_TAG,
                         "Buffer too small: need>=%u, got=%u, context=%s",
                         static_cast<unsigned>(min_size),
                         static_cast<unsigned>(buf.size()),
                         context);
                return false;
            }
            return true;
        }

    } // namespace balboa_spa
} // namespace esphome
