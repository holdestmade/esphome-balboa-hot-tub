#include "stdint.h"
#include <string>

#ifndef SPA_TYPES_H
#define SPA_TYPES_H

namespace esphome
{
    namespace balboa_spa
    {
        struct SpaFaultLog
        {
            uint8_t total_entries : 5;
            uint8_t current_entry : 5;
            uint8_t fault_code : 6;
            std::string fault_message;
            uint8_t days_ago;
            uint8_t hour : 5;
            uint8_t minutes : 6;
        };

        struct SpaFilterSettings
        {
            uint8_t filter1_hour : 5;
            uint8_t filter1_minute : 6;
            uint8_t filter1_duration_hour : 5;
            uint8_t filter1_duration_minute : 6;
            uint8_t filter2_enable : 1;
            uint8_t filter2_hour : 5;
            uint8_t filter2_minute : 6;
            uint8_t filter2_duration_hour : 5;
            uint8_t filter2_duration_minute : 6;
        };

        // Jet/pump toggle state: 0=OFF, 1=LOW/ON, 2=HIGH_SPEED, 254=UNKNOWN
        enum ToggleStateMaybe
        {
            OFF = 0,
            ON,
            HIGH_SPEED,
            DONT_KNOW
        };

        // Heat state values reported in the status update message.
        enum HeatState : uint8_t
        {
            HEAT_STATE_IDLE = 0,   // Heater is idle (not heating)
            HEAT_STATE_HEATING = 1 // Heater is actively heating
        };

        // Reminder type values reported in the status update message.
        // See STATUS_IDX_REMINDER in protocol_definitions.h.
        enum ReminderType : uint8_t
        {
            REMINDER_TYPE_NONE = 0x00,
            REMINDER_TYPE_CLEAN_FILTER_ALT = 0x03, // observed on some firmwares
            REMINDER_TYPE_CLEAN_FILTER = 0x04,
            REMINDER_TYPE_CHECK_SANITIZER = 0x09,
            REMINDER_TYPE_CHECK_PH = 0x0A,
            REMINDER_TYPE_FAULT = 0x1E
        };

        // Map a reminder code to a human-readable name.
        // Returns nullptr for unknown codes so callers can format them as needed.
        // NOTE: This list is incomplete. If you encounter an unknown code, please
        // open a GitHub issue with the code value and the reminder message shown
        // on your spa control panel so we can expand this mapping.
        inline const char *reminder_type_to_string(uint8_t code)
        {
            switch (code)
            {
            case REMINDER_TYPE_NONE:
                return "None";
            case REMINDER_TYPE_CLEAN_FILTER_ALT:
            case REMINDER_TYPE_CLEAN_FILTER:
                return "Clean Filter";
            case REMINDER_TYPE_CHECK_SANITIZER:
                return "Check Sanitizer";
            case REMINDER_TYPE_CHECK_PH:
                return "Check pH";
            case REMINDER_TYPE_FAULT:
                return "Fault";
            default:
                return nullptr;
            }
        }

        // Cleanup cycle state values (decoded from STATUS_IDX_CLEANUP lower nibble).
        enum CleanupCycleState : uint8_t
        {
            CLEANUP_CYCLE_OFF = 0,
            CLEANUP_CYCLE_ON = 1,
            CLEANUP_CYCLE_UNKNOWN = 254
        };

    } // namespace balboa_spa
} // namespace esphome

#endif