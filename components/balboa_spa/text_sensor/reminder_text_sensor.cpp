#include "reminder_text_sensor.h"

namespace esphome
{
    namespace balboa_spa
    {
        static const char *TAG = "balboa_spa.text_sensor.reminder";

        void ReminderTextSensor::set_parent(BalboaSpa *parent)
        {
            parent->register_listener(
                [this](SpaState *spaState)
                {
                    this->update(spaState);
                });
        }

        void ReminderTextSensor::update(SpaState *spaState)
        {
            // Check if the reminder has changed
            if (spaState->reminder != last_reminder_)
            {
                // Reminder code names come from the shared mapping in spa_types.h.
                std::string reminder_message;
                const char *known_name = reminder_type_to_string(spaState->reminder);
                if (known_name != nullptr)
                {
                    reminder_message = known_name;
                }
                else
                {
                    // Format unknown reminder code in hex
                    char hex_str[8];
                    snprintf(hex_str, sizeof(hex_str), "0x%02X", spaState->reminder);
                    reminder_message = std::string("Unknown (") + hex_str + ")";
                }


                ESP_LOGD(TAG, "Reminder update: %s (0x%02X)", 
                         reminder_message.c_str(), spaState->reminder);
                this->publish_state(reminder_message);
                last_reminder_ = spaState->reminder;
            }
        }

    } // namespace balboa_spa
} // namespace esphome
