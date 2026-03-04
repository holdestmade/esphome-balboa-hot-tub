#pragma once

#include <stdint.h>
#include "protocol_definitions.h"

// Bit-field extraction helpers for the Balboa RS485 protocol.
//
// These inline functions provide descriptive, testable wrappers around
// the bit-masking operations used throughout the protocol parser.

namespace esphome
{
    namespace balboa_spa
    {

        // Read a single bit from a byte value.
        // Returns 1 if the bit at position `bit` is set, 0 otherwise.
        inline uint8_t read_bit(uint8_t value, uint8_t bit)
        {
            return (value >> bit) & 0x01;
        }

        // ----------------------------------------------------------------
        // Jet pump state extraction (STATUS_IDX_JETS byte)
        // Each jet occupies 2 bits: 0=OFF, 1=LOW, 2=HIGH
        // ----------------------------------------------------------------

        // Extract jet 1 state from the jets status byte (bits 0-1).
        inline uint8_t extract_jet1_state(uint8_t jets_byte)
        {
            return jets_byte & STATUS_JET1_MASK;
        }

        // Extract jet 2 state from the jets status byte (bits 2-3).
        inline uint8_t extract_jet2_state(uint8_t jets_byte)
        {
            return (jets_byte & STATUS_JET2_MASK) >> STATUS_JET2_SHIFT;
        }

        // Extract jet 3 state from the jets status byte (bits 4-5).
        inline uint8_t extract_jet3_state(uint8_t jets_byte)
        {
            return (jets_byte & STATUS_JET3_MASK) >> STATUS_JET3_SHIFT;
        }

        // Extract jet 4 state from the jets status byte (bits 6-7).
        inline uint8_t extract_jet4_state(uint8_t jets_byte)
        {
            return (jets_byte & STATUS_JET4_MASK) >> STATUS_JET4_SHIFT;
        }

        // Generic: extract the state of a jet given a 0-based jet index (0-3).
        // jet_index 0 -> bits 0-1, jet_index 1 -> bits 2-3, etc.
        inline uint8_t extract_jet_state(uint8_t jets_byte, uint8_t jet_index)
        {
            return (jets_byte >> (jet_index * 2)) & 0x03;
        }

        // ----------------------------------------------------------------
        // Light state extraction (STATUS_IDX_LIGHTS byte)
        // ----------------------------------------------------------------

        // Extract light 1 state: returns 1 if light1 is ON (bits 0-1 == 0x03), 0 otherwise.
        inline uint8_t extract_light1_state(uint8_t lights_byte)
        {
            return (lights_byte & STATUS_LIGHT1_ON_VALUE) == STATUS_LIGHT1_ON_VALUE ? 1 : 0;
        }

        // Extract light 2 state from bits 2-3 of the lights byte.
        inline uint8_t extract_light2_state(uint8_t lights_byte)
        {
            return (lights_byte & STATUS_LIGHT2_MASK) >> STATUS_LIGHT2_SHIFT;
        }

        // ----------------------------------------------------------------
        // Pump configuration extraction (CONFIG_IDX_PUMPS_* bytes)
        // Each pump occupies 2 bits: 0=absent, 1=1-speed, 2=2-speed
        // ----------------------------------------------------------------

        // Extract pump state for a given pump_index (0-3) from a config byte.
        inline uint8_t extract_pump_state(uint8_t config_byte, uint8_t pump_index)
        {
            return (config_byte >> (pump_index * 2)) & 0x03;
        }

        // ----------------------------------------------------------------
        // Cleanup cycle extraction (STATUS_IDX_CLEANUP lower nibble)
        // ----------------------------------------------------------------

        // Returns true when the cleanup cycle is active (lower nibble == 0x0C).
        inline bool is_cleanup_cycle_active(uint8_t cleanup_byte)
        {
            return (cleanup_byte & CLEANUP_NIBBLE_MASK) == CLEANUP_ON_VALUE;
        }

        // Decode cleanup cycle state to 0 (off), 1 (on), or 254 (unknown).
        inline uint8_t decode_cleanup_cycle(uint8_t cleanup_byte)
        {
            uint8_t nibble = cleanup_byte & CLEANUP_NIBBLE_MASK;
            if (nibble == CLEANUP_ON_VALUE)
                return 1;
            if (nibble == CLEANUP_OFF_V1 || nibble == CLEANUP_OFF_V2 || nibble == CLEANUP_OFF_V3)
                return 0;
            return 254; // Unknown state
        }

    } // namespace balboa_spa
} // namespace esphome
