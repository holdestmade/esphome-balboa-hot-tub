#pragma once

#include <stdint.h>
#include <cstddef>
#include "CircularBuffer.h"
#include "esphome/core/log.h"

// BufferUtilities - Safe buffer access helpers for the Balboa component.
//
// The CircularBuffer's operator[] already logs an error and returns a default
// value when an index is out of bounds.  These helpers add an explicit check
// and a caller-supplied context string so that the source of a bad access is
// immediately apparent in the log output.

namespace esphome
{
    namespace balboa_spa
    {

        static const char *BUFFER_UTILS_TAG = "BalboaSpa.Buffer";

        // Return the byte at `index` in `buf`.
        // Logs a warning and returns 0 if the index is out of bounds.
        inline uint8_t safe_buffer_read(CircularBuffer<uint8_t, 100> &buf,
                                        size_t index,
                                        const char *context = "")
        {
            if (index >= buf.size())
            {
                ESP_LOGW(BUFFER_UTILS_TAG,
                         "Read out of bounds: index=%u, size=%u, context=%s",
                         static_cast<unsigned>(index),
                         static_cast<unsigned>(buf.size()),
                         context);
                return 0;
            }
            return buf[index];
        }

        // Return true if `buf` contains at least `min_size` bytes.
        // Logs a warning (with the supplied context) and returns false otherwise.
        inline bool buffer_has_minimum_size(CircularBuffer<uint8_t, 100> &buf,
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

        // Return true (and log a warning) when a push to `buf` would exceed its
        // capacity.  The CircularBuffer will still accept the value by evicting the
        // oldest entry; this function lets the caller detect and log the condition
        // before that happens.
        template <size_t S>
        inline bool buffer_would_overflow(CircularBuffer<uint8_t, S> &buf,
                                          const char *context = "")
        {
            if (buf.size() >= S)
            {
                ESP_LOGW(BUFFER_UTILS_TAG,
                         "Buffer overflow: capacity=%u, context=%s",
                         static_cast<unsigned>(S),
                         context);
                return true;
            }
            return false;
        }

    } // namespace balboa_spa
} // namespace esphome
