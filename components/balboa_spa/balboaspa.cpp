#include "balboaspa.h"
#include <cstring>

namespace esphome
{
    namespace balboa_spa
    {

        static const char *TAG = "BalboaSpa.component";
        static const char *CRC_TAG = "BalboaSpa.CRC";

        void BalboaSpa::setup()
        {
            input_queue.clear();
            output_queue.clear();
            filtersettings_update_timer = 0;
            last_received_time = millis(); // avoid spurious timeout on slow boot
        }

        void BalboaSpa::update()
        {
            uint32_t now = millis();
            if (last_received_time + 10000 < now)
            {
                ESP_LOGW(TAG, "No new message since %d Seconds! Mark as dead!", (now - last_received_time) / 1000);
                status_set_error(LOG_STR("No Communication with Balboa Mainboard!"));
                client_id = 0;
            }
            else if (status_has_error())
            {
                status_clear_error();
            }

            // Filter settings periodic update timer (every 5 minutes)
            if (filtersettings_request_status == 2)
            {
                filtersettings_update_timer++;
                if (filtersettings_update_timer >= 6000)
                {                                      // 6000 * 50ms = 5 minutes
                    filtersettings_request_status = 0; // Reset to request again
                    filtersettings_update_timer = 0;
                    ESP_LOGD(TAG, "Spa/debug/filtersettings_request_status: %s", "resetting for periodic update");
                }
            }

            while (available())
            {
                read_serial();
            }
        }

        float BalboaSpa::get_setup_priority() const { return esphome::setup_priority::LATE; }

        SpaConfig BalboaSpa::get_current_config() { return spaConfig; }
        SpaState *BalboaSpa::get_current_state() { return &spaState; }
        SpaFilterSettings *BalboaSpa::get_current_filter_settings() { return &spaFilterSettings; }
        SpaFaultLog *BalboaSpa::get_current_fault_log() { return &spaFaultLog; }

        void BalboaSpa::set_temp(float temp)
        {
            float target_temp = 0.0;

            if (esphome_temp_scale == TEMP_SCALE::C &&
                temp >= ESPHOME_BALBOASPA_MIN_TEMPERATURE_C &&
                temp <= ESPHOME_BALBOASPA_MAX_TEMPERATURE_C)
            {
                target_temp = temp;
            }
            else if (esphome_temp_scale == TEMP_SCALE::F &&
                     temp >= ESPHOME_BALBOASPA_MIN_TEMPERATURE_F &&
                     temp <= ESPHOME_BALBOASPA_MAX_TEMPERATURE_F)
            {
                target_temp = convert_f_to_c(temp);
            }
            else
            {
                ESP_LOGW(TAG, "set_temp(%f): is INVALID! %d", temp, esphome_temp_scale);
                return;
            }

            if (spa_temp_scale == TEMP_SCALE::C)
            {
                target_temperature = target_temp * 2.0f;
            }
            else if (spa_temp_scale == TEMP_SCALE::F)
            {
                target_temperature = convert_c_to_f(target_temp);
            }
            else
            {
                ESP_LOGW(TAG, "set_temp(%f): spa_temp_scale not set. Ignoring %d", temp, spa_temp_scale);
                return;
            }

            send_command = SEND_CMD_SET_TEMP;
        }

        void BalboaSpa::set_highrange(bool high)
        {
            ESP_LOGD(TAG, "highrange=%d to %d requested", spaState.highrange, high);
            if (high != spaState.highrange)
            {
                send_command = TOGGLE_HIGHRANGE;
            }
        }

        bool BalboaSpa::get_restmode()
        {
            return spaState.rest_mode == 1;
        }

        void BalboaSpa::toggle_heat()
        {
            ESP_LOGD("balboa_spa", "Send TOGGLE_HEAT to toggle heat/rest");
            send_command = TOGGLE_HEAT;
        }

        void BalboaSpa::request_config_update()
        {
            ESP_LOGD(TAG, "Requesting spa config update");
            config_request_status = 0; // Reset to request config again
        }

        void BalboaSpa::request_filter_settings_update()
        {
            ESP_LOGD(TAG, "Requesting spa filter settings update");
            filtersettings_request_status = 0; // Reset to request filter settings again
        }

        void BalboaSpa::request_fault_log_update()
        {
            ESP_LOGD(TAG, "Requesting spa fault log update");
            faultlog_request_status = 0; // Reset to request fault log again
        }

        void BalboaSpa::set_hour(int hour)
        {
            if (hour >= 0 && hour <= 23)
            {
                pending_time_hour = static_cast<uint8_t>(hour);
                pending_time_minute = spaState.minutes; // preserve current minute
                send_command = MSG_ID_SET_TIME;
            }
        }

        void BalboaSpa::set_minute(int minute)
        {
            if (minute >= 0 && minute <= 59)
            {
                pending_time_minute = static_cast<uint8_t>(minute);
                pending_time_hour = spaState.hour; // preserve current hour
                send_command = MSG_ID_SET_TIME;
            }
        }

        void BalboaSpa::set_timescale(bool is_24h)
        {
            send_preference_data = is_24h ? 0x01 : 0x00;
            send_command = MSG_ID_SET_PREFERENCE;
            send_preference_code = PREFERENCE_TIMESCALE;
        }

        void BalboaSpa::sync_filter_targets_from_settings()
        {
            target_filter1_start_hour        = spaFilterSettings.filter1_hour;
            target_filter1_start_minute      = spaFilterSettings.filter1_minute;
            target_filter1_duration_hour     = spaFilterSettings.filter1_duration_hour;
            target_filter1_duration_minute   = spaFilterSettings.filter1_duration_minute;
            target_filter2_enable            = spaFilterSettings.filter2_enable;
            target_filter2_start_hour        = spaFilterSettings.filter2_hour;
            target_filter2_start_minute      = spaFilterSettings.filter2_minute;
            target_filter2_duration_hour     = spaFilterSettings.filter2_duration_hour;
            target_filter2_duration_minute   = spaFilterSettings.filter2_duration_minute;
        }

        void BalboaSpa::set_filter1_config(uint8_t start_hour, uint8_t start_minute, uint8_t duration_hour, uint8_t duration_minute)
        {
            if (start_hour < 24 && start_minute < 60 && duration_hour < 24 && duration_minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter1_start_hour    = start_hour;
                target_filter1_start_minute  = start_minute;
                target_filter1_duration_hour = duration_hour;
                target_filter1_duration_minute = duration_minute;
                send_command = MSG_ID_FILTER_CONFIG;
            }
        }

        void BalboaSpa::set_filter2_config(uint8_t start_hour, uint8_t start_minute, uint8_t duration_hour, uint8_t duration_minute)
        {
            if (start_hour < 24 && start_minute < 60 && duration_hour < 24 && duration_minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter2_start_hour    = start_hour;
                target_filter2_start_minute  = start_minute;
                target_filter2_duration_hour = duration_hour;
                target_filter2_duration_minute = duration_minute;
                target_filter2_enable        = true;
                send_command = MSG_ID_FILTER_CONFIG;
            }
        }

        void BalboaSpa::disable_filter2()
        {
            sync_filter_targets_from_settings();
            target_filter2_enable = false;
            send_command = MSG_ID_FILTER_CONFIG;
        }

        void BalboaSpa::set_filter1_start_time(uint8_t hour, uint8_t minute)
        {
            if (hour < 24 && minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter1_start_hour   = hour;
                target_filter1_start_minute = minute;
                send_command = MSG_ID_FILTER_CONFIG;
                ESP_LOGI(TAG, "Filter 1 start time set to %02d:%02d", hour, minute);
            }
        }

        void BalboaSpa::set_filter1_duration(uint8_t hour, uint8_t minute)
        {
            if (hour < 24 && minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter1_duration_hour   = hour;
                target_filter1_duration_minute = minute;
                send_command = MSG_ID_FILTER_CONFIG;
                ESP_LOGI(TAG, "Filter 1 duration set to %02d:%02d", hour, minute);
            }
        }

        void BalboaSpa::set_filter2_start_time(uint8_t hour, uint8_t minute)
        {
            if (hour < 24 && minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter2_start_hour   = hour;
                target_filter2_start_minute = minute;
                target_filter2_enable       = true;
                send_command = MSG_ID_FILTER_CONFIG;
                ESP_LOGI(TAG, "Filter 2 start time set to %02d:%02d", hour, minute);
            }
        }

        void BalboaSpa::set_filter2_duration(uint8_t hour, uint8_t minute)
        {
            if (hour < 24 && minute < 60)
            {
                sync_filter_targets_from_settings();
                target_filter2_duration_hour   = hour;
                target_filter2_duration_minute = minute;
                target_filter2_enable          = true;
                send_command = MSG_ID_FILTER_CONFIG;
                ESP_LOGI(TAG, "Filter 2 duration set to %02d:%02d", hour, minute);
            }
        }

        void BalboaSpa::toggle_light()
        {
            send_command = TOGGLE_LIGHT1;
        }

        void BalboaSpa::toggle_light2()
        {
            send_command = TOGGLE_LIGHT2;
        }

        void BalboaSpa::toggle_jet1()
        {
            send_command = TOGGLE_JET1;
        }

        void BalboaSpa::toggle_jet2()
        {
            send_command = TOGGLE_JET2;
        }

        void BalboaSpa::toggle_jet3()
        {
            send_command = TOGGLE_JET3;
        }

        void BalboaSpa::toggle_jet4()
        {
            send_command = TOGGLE_JET4;
        }

        void BalboaSpa::toggle_blower()
        {
            send_command = TOGGLE_BLOWER;
        }

        void BalboaSpa::clear_reminder()
        {
            send_command = TOGGLE_CLEAR_REMINDER;
            ESP_LOGI(TAG, "Clearing spa reminder");
        }

        void BalboaSpa::read_serial()
        {
            if (!read_byte(&received_byte))
            {
                return;
            }

            // Drop bytes until a SOF marker is seen
            if (input_queue.first() != PROTO_SOF_EOF && received_byte != PROTO_SOF_EOF)
            {
                input_queue.clear();
                return;
            }

            // Drop a duplicate SOF marker (double PROTO_SOF_EOF at start)
            if (input_queue.size() >= 2 && input_queue[1] == PROTO_SOF_EOF)
            {
                input_queue.pop();
                return;
            }

            input_queue.push(received_byte);

            // Check whether we have received a complete packet.
            // A complete frame satisfies:  received byte is EOF &&
            //   total size >= length_field + 2  (SOF + data[1..length] + EOF)
            if (received_byte == PROTO_SOF_EOF &&
                input_queue.size() > 2 &&
                input_queue.size() >= static_cast<size_t>(input_queue[PROTO_IDX_LENGTH]) + 2)
            {
                if (input_queue.size() - 2 < input_queue[PROTO_IDX_LENGTH])
                {
                    ESP_LOGD(TAG, "packet_size: %d, recv_size: %d",
                             input_queue[PROTO_IDX_LENGTH], input_queue.size());
                    ESP_LOGD(TAG, "%s", "Packet incomplete!");
                    input_queue.clear();
                    return;
                }

                // Verify CRC
                auto calculated_crc = this->crc8(input_queue, true);
                auto packet_crc = input_queue[input_queue[PROTO_IDX_LENGTH]];
                if (calculated_crc != packet_crc)
                {
                    ESP_LOGD(CRC_TAG, "CRC %d != Packet crc %d end=0x%X",
                             calculated_crc, packet_crc,
                             input_queue[input_queue[PROTO_IDX_LENGTH] + 1]);
                    input_queue.clear();
                    return;
                }

                // Dispatch based on registration state and message type
                if (client_id == 0)
                {
                    handle_unregistered();
                }
                else if (input_queue[PROTO_IDX_DEST] == client_id &&
                         input_queue[PROTO_IDX_MSG_TYPE] == MSG_ID_CLEAR_TO_SEND)
                {
                    handle_ready_to_send();
                }
                else if (input_queue[PROTO_IDX_DEST] == client_id &&
                         input_queue[PROTO_IDX_MSG_TYPE] == MSG_ID_CONFIG_RESPONSE)
                {
                    if (last_state_crc != input_queue[input_queue[PROTO_IDX_LENGTH]])
                        handle_config_response();
                }
                else if (input_queue[PROTO_IDX_DEST] == client_id &&
                         input_queue[PROTO_IDX_MSG_TYPE] == MSG_ID_FAULT_LOG)
                {
                    if (last_state_crc != input_queue[input_queue[PROTO_IDX_LENGTH]])
                        handle_fault_log_response();
                }
                else if (input_queue[PROTO_IDX_DEST] == PROTO_ADDR_BROADCAST &&
                         input_queue[PROTO_IDX_MSG_TYPE] == MSG_ID_STATUS_UPDATE)
                {
                    if (last_state_crc != input_queue[input_queue[PROTO_IDX_LENGTH]])
                        handle_status_update();
                }
                else if (input_queue[PROTO_IDX_DEST] == client_id &&
                         input_queue[PROTO_IDX_MSG_TYPE] == MSG_ID_FILTER_CONFIG)
                {
                    if (last_state_crc != input_queue[input_queue[PROTO_IDX_LENGTH]])
                        handle_filter_settings_response();
                }

                input_queue.clear();
            }
            last_received_time = millis();
        }

        // ---------------------------------------------------------------------------
        // Message handler implementations
        // ---------------------------------------------------------------------------

        void BalboaSpa::handle_unregistered()
        {
            ESP_LOGD(TAG, "Spa/node/id: %s", "Unregistered");
            print_msg(input_queue);

            uint8_t msg_type = input_queue[PROTO_IDX_MSG_TYPE];

            if (input_queue[PROTO_IDX_DEST] == PROTO_ADDR_UNREGISTERED &&
                msg_type == MSG_ID_CLIENT_ID_OFFER)
            {
                handle_id_acknowledge();
            }
            else if (input_queue[PROTO_IDX_DEST] == PROTO_ADDR_UNREGISTERED &&
                     msg_type == MSG_ID_NEW_CLIENTS)
            {
                handle_id_request();
            }
        }

        void BalboaSpa::handle_id_request()
        {
            ESP_LOGD(TAG, "Spa/node/id: %s", "Requesting ID");
            ID_request();
        }

        void BalboaSpa::handle_id_acknowledge()
        {
            if (!buffer_has_minimum_size(input_queue, ID_NEGO_IDX_CLIENT_ID + 1, "ID ack"))
                return;

            if (use_client_id_override)
            {
                client_id = client_id_override;
                ESP_LOGD(TAG, "Spa/node/id: Using override ID: %d, acknowledging", client_id);
            }
            else
            {
                client_id = input_queue[ID_NEGO_IDX_CLIENT_ID];
                if (client_id > CLIENT_ID_MAX)
                    client_id = CLIENT_ID_MAX;
                ESP_LOGD(TAG, "Spa/node/id: Got ID: %d, acknowledging", client_id);
            }
            ID_ack();
            ESP_LOGD(TAG, "Spa/node/id: %d", client_id);
        }

        void BalboaSpa::handle_ready_to_send()
        {
            if (send_command == MSG_ID_SET_TIME)
            {
                ProtocolMessageBuilder::build_set_time(output_queue, client_id,
                                                       pending_time_hour, pending_time_minute);
            }
            else if (send_command == SEND_CMD_SET_TEMP)
            {
                ProtocolMessageBuilder::build_set_temp(output_queue, client_id, target_temperature);
            }
            else if (send_command == 0x00)
            {
                // No user command pending — use the slot for housekeeping requests
                if (config_request_status == 0)
                {
                    ProtocolMessageBuilder::build_config_request(output_queue, client_id,
                                                                 CONFIG_SUB_SETTINGS_B1,
                                                                 CONFIG_SUB_SETTINGS_B2,
                                                                 CONFIG_SUB_SETTINGS_B3);
                    ESP_LOGD(TAG, "Spa/config/status: %s", "Getting config");
                    config_request_status = 1;
                }
                else if (faultlog_request_status == 0)
                {
                    ProtocolMessageBuilder::build_config_request(output_queue, client_id,
                                                                 CONFIG_SUB_FAULT_B1,
                                                                 CONFIG_SUB_FAULT_B2,
                                                                 CONFIG_SUB_FAULT_B3);
                    faultlog_request_status = 1;
                    ESP_LOGD(TAG, "Spa/debug/faultlog_request_status: %s",
                             "requesting fault log");
                }
                else if (filtersettings_request_status == 0 && faultlog_request_status == 2)
                {
                    // Only request filter settings after the fault log has been received
                    ProtocolMessageBuilder::build_config_request(output_queue, client_id,
                                                                 CONFIG_SUB_FILTER_B1,
                                                                 CONFIG_SUB_FILTER_B2,
                                                                 CONFIG_SUB_FILTER_B3);
                    ESP_LOGD(TAG, "Spa/debug/filtersettings_request_status: %s",
                             "requesting filter settings");
                    filtersettings_request_status = 1;
                }
                else
                {
                    ProtocolMessageBuilder::build_nothing_to_send(output_queue, client_id);
                }
            }
            else if (send_command == MSG_ID_SET_PREFERENCE)
            {
                ProtocolMessageBuilder::build_set_preference(output_queue, client_id,
                                                             send_preference_code,
                                                             send_preference_data);
            }
            else if (send_command == MSG_ID_FILTER_CONFIG)
            {
                ProtocolMessageBuilder::build_filter_config(output_queue, client_id,
                                                            target_filter1_start_hour,
                                                            target_filter1_start_minute,
                                                            target_filter1_duration_hour,
                                                            target_filter1_duration_minute,
                                                            target_filter2_enable,
                                                            target_filter2_start_hour,
                                                            target_filter2_start_minute,
                                                            target_filter2_duration_hour,
                                                            target_filter2_duration_minute);
            }
            else
            {
                // Generic toggle command
                ProtocolMessageBuilder::build_toggle(output_queue, client_id, send_command);
            }

            rs485_send();
            send_command = 0x00;
        }

        void BalboaSpa::handle_status_update()
        {
            if (!buffer_has_minimum_size(input_queue, STATUS_IDX_TARGET_TEMP + 1, "status update"))
                return;
            decodeState();
        }

        void BalboaSpa::handle_config_response()
        {
            if (!buffer_has_minimum_size(input_queue, CONFIG_IDX_AUX_MISTER + 1, "config response"))
                return;
            decodeSettings();
        }

        void BalboaSpa::handle_filter_settings_response()
        {
            if (!buffer_has_minimum_size(input_queue, FILTER_IDX_F2_DUR_MIN + 1, "filter settings"))
                return;
            ESP_LOGD(TAG, "Spa/debug/filtersettings_request_status: %s", "decoding filter settings");
            decodeFilterSettings();
        }

        void BalboaSpa::handle_fault_log_response()
        {
            if (!buffer_has_minimum_size(input_queue, FAULT_IDX_MINUTES + 1, "fault log"))
                return;
            decodeFault();
        }

        // CRC8 algorithm used by the Balboa RS485 protocol.
        // Polynomial: x^8 + x^2 + x + 1 (0x07)
        // Initial value: 0x02  Final XOR: 0x02
        // When ignore_delimiter is true, the leading SOF byte (index 0) and the
        // trailing two bytes (CRC + EOF) are excluded from the calculation.
        //
        // Example: frame [7E, 07, FE, BF, 01, 02, F1, 73, <CRC>, 7E]
        //   crc8 with ignore_delimiter=true operates on [07, FE, BF, 01, 02, F1, 73]
        //   (skips SOF at index 0 and the trailing CRC+EOF bytes)
        uint8_t BalboaSpa::crc8(CircularBuffer<uint8_t, 100> &data, bool ignore_delimiter)
        {
            unsigned long crc_value;
            int bit_index;
            uint8_t data_length = ignore_delimiter ? data.size() - 2 : data.size();

            crc_value = CRC8_INIT;
            for (size_t byte_index = ignore_delimiter; byte_index < data_length; byte_index++)
            {
                crc_value ^= data[byte_index];
                for (bit_index = 0; bit_index < 8; bit_index++)
                {
                    if ((crc_value & 0x80) != 0)
                    {
                        crc_value <<= 1;
                        crc_value ^= CRC8_POLYNOMIAL;
                    }
                    else
                    {
                        crc_value <<= 1;
                    }
                }
            }
            return crc_value ^ CRC8_FINAL_XOR;
        }

        void BalboaSpa::ID_request()
        {
            ProtocolMessageBuilder::build_id_request(output_queue);
            rs485_send();
        }

        void BalboaSpa::ID_ack()
        {
            ProtocolMessageBuilder::build_id_ack(output_queue, client_id);
            rs485_send();
        }

        void BalboaSpa::rs485_send()
        {
            // Add telegram length
            output_queue.unshift(output_queue.size() + 2);

            // Add CRC
            output_queue.push(crc8(output_queue, false));

            // Wrap telegram in SOF/EOF
            output_queue.unshift(PROTO_SOF_EOF);
            output_queue.push(PROTO_SOF_EOF);

            for (loop_index = 0; loop_index < output_queue.size(); loop_index++)
            {
                write(output_queue[loop_index]);
            }

            flush();

            output_queue.clear();
        }

        void BalboaSpa::print_msg(CircularBuffer<uint8_t, 100> &data)
        {
            // Pre-allocated buffer: each byte formats as "XX " (3 chars), plus null terminator.
            // CircularBuffer capacity is 100 bytes, so 100 * 3 + 1 = 301 chars maximum.
            static constexpr size_t BUF_SIZE = 100 * 3 + 1;
            char buf[BUF_SIZE];
            size_t pos = 0;
            for (size_t i = 0; i < data.size() && pos + 3 < BUF_SIZE; i++)
            {
                int written = snprintf(buf + pos, BUF_SIZE - pos, "%02X ", data[i]);
                if (written > 0)
                    pos += written;
            }
            ESP_LOGD(TAG, "MSG: %s", buf);
            yield();
        }

        void BalboaSpa::decodeSettings()
        {
            ESP_LOGD(TAG, "Spa/config/status: Got config");
            spaConfig.pump1 = input_queue[CONFIG_IDX_PUMPS_1_4] & 0x03;
            spaConfig.pump2 = (input_queue[CONFIG_IDX_PUMPS_1_4] & 0x0C) >> 2;
            spaConfig.pump3 = (input_queue[CONFIG_IDX_PUMPS_1_4] & 0x30) >> 4;
            spaConfig.pump4 = (input_queue[CONFIG_IDX_PUMPS_1_4] & 0xC0) >> 6;
            spaConfig.pump5 = (input_queue[CONFIG_IDX_PUMPS_5_6] & 0x03);
            spaConfig.pump6 = (input_queue[CONFIG_IDX_PUMPS_5_6] & 0xC0) >> 6;
            spaConfig.light1 = (input_queue[CONFIG_IDX_LIGHTS] & 0x03);
            spaConfig.light2 = (input_queue[CONFIG_IDX_LIGHTS] >> 2) & 0x03;
            spaConfig.circ = ((input_queue[CONFIG_IDX_CIRC_BLOWER] & CONFIG_CIRC_MASK) != 0);
            spaConfig.blower = ((input_queue[CONFIG_IDX_CIRC_BLOWER] & CONFIG_BLOWER_MASK) != 0);
            spaConfig.mister = ((input_queue[CONFIG_IDX_AUX_MISTER] & CONFIG_MISTER_MASK) != 0);
            spaConfig.aux1 = ((input_queue[CONFIG_IDX_AUX_MISTER] & CONFIG_AUX1_MASK) != 0);
            spaConfig.aux2 = ((input_queue[CONFIG_IDX_AUX_MISTER] & CONFIG_AUX2_MASK) != 0);
            spaConfig.temperature_scale = input_queue[CONFIG_IDX_TEMP_CLOCK] & CONFIG_TEMP_SCALE_MASK;
            spaConfig.clock_mode = (input_queue[CONFIG_IDX_TEMP_CLOCK] >> CONFIG_CLOCK_MODE_SHIFT) & 0x1;
            ESP_LOGD(TAG, "Spa/config/pumps1: %d", spaConfig.pump1);
            ESP_LOGD(TAG, "Spa/config/pumps2: %d", spaConfig.pump2);
            ESP_LOGD(TAG, "Spa/config/pumps3: %d", spaConfig.pump3);
            ESP_LOGD(TAG, "Spa/config/pumps4: %d", spaConfig.pump4);
            ESP_LOGD(TAG, "Spa/config/pumps5: %d", spaConfig.pump5);
            ESP_LOGD(TAG, "Spa/config/pumps6: %d", spaConfig.pump6);
            ESP_LOGD(TAG, "Spa/config/light1: %d", spaConfig.light1);
            ESP_LOGD(TAG, "Spa/config/light2: %d", spaConfig.light2);
            ESP_LOGD(TAG, "Spa/config/circ: %d", spaConfig.circ);
            ESP_LOGD(TAG, "Spa/config/blower: %d", spaConfig.blower);
            ESP_LOGD(TAG, "Spa/config/mister: %d", spaConfig.mister);
            ESP_LOGD(TAG, "Spa/config/aux1: %d", spaConfig.aux1);
            ESP_LOGD(TAG, "Spa/config/aux2: %d", spaConfig.aux2);
            ESP_LOGD(TAG, "Spa/config/temperature_scale: %d", spaConfig.temperature_scale);
            ESP_LOGD(TAG, "Spa/config/clock_mode: %d", spaConfig.clock_mode);
            config_request_status = 2;

            if (spa_temp_scale == TEMP_SCALE::UNDEFINED)
            {
                spa_temp_scale = static_cast<TEMP_SCALE>(spaConfig.temperature_scale);
            }
        }

        void BalboaSpa::decodeState()
        {
            float temp_read = 0.0f;

            // Decode target (set) temperature from STATUS_IDX_TARGET_TEMP
            if (spa_temp_scale == TEMP_SCALE::C)
            {
                temp_read = input_queue[STATUS_IDX_TARGET_TEMP] / 2.0f;
            }
            else if (spa_temp_scale == TEMP_SCALE::F)
            {
                temp_read = convert_f_to_c(input_queue[STATUS_IDX_TARGET_TEMP]);
            }

            if (esphome_temp_scale == TEMP_SCALE::C &&
                temp_read >= ESPHOME_BALBOASPA_MIN_TEMPERATURE_C &&
                temp_read <= ESPHOME_BALBOASPA_MAX_TEMPERATURE_C)
            {
                spaState.target_temp = temp_read;
                ESP_LOGD(TAG, "Spa/temperature/target: %.2f C", temp_read);
            }
            else if (esphome_temp_scale == TEMP_SCALE::F &&
                     temp_read >= ESPHOME_BALBOASPA_MIN_TEMPERATURE_F &&
                     temp_read <= ESPHOME_BALBOASPA_MAX_TEMPERATURE_F)
            {
                spaState.target_temp = convert_c_to_f(temp_read);
                ESP_LOGD(TAG, "Spa/temperature/target: %.2f F", temp_read);
            }
            else
            {
                ESP_LOGW(TAG, "Spa/temperature/target INVALID %.2f %.2f %d %d",
                         input_queue[STATUS_IDX_TARGET_TEMP], temp_read,
                         spaConfig.temperature_scale, esphome_temp_scale);
            }

            // Decode actual current water temperature from STATUS_IDX_CURRENT_TEMP
            if (input_queue[STATUS_IDX_CURRENT_TEMP] != TEMP_UNKNOWN)
            {
                if (spa_temp_scale == TEMP_SCALE::C)
                {
                    temp_read = input_queue[STATUS_IDX_CURRENT_TEMP] / 2.0f;
                }
                else if (spa_temp_scale == TEMP_SCALE::F)
                {
                    temp_read = convert_f_to_c(input_queue[STATUS_IDX_CURRENT_TEMP]);
                }

                if (temp_read > ESPHOME_BALBOASPA_MAX_VALID_TEMP_C)
                {
                    // Temperature approaching boiling — definitely invalid
                    ESP_LOGW(TAG, "Spa/temperature/current INVALID %.2f %.2f %d",
                             input_queue[STATUS_IDX_CURRENT_TEMP], temp_read,
                             spaConfig.temperature_scale);
                }
                else if (esphome_temp_scale == TEMP_SCALE::C)
                {
                    spaState.current_temp = temp_read;
                    ESP_LOGD(TAG, "Spa/temperature/current: %.2f C", temp_read);
                }
                else if (esphome_temp_scale == TEMP_SCALE::F)
                {
                    spaState.current_temp = convert_c_to_f(temp_read);
                    ESP_LOGD(TAG, "Spa/temperature/current: %.2f F", temp_read);
                }
                else
                {
                    ESP_LOGW(TAG, "Spa/temperature/current INVALID %.2f %.2f %d %d",
                             input_queue[STATUS_IDX_CURRENT_TEMP], temp_read,
                             spaConfig.temperature_scale, esphome_temp_scale);
                }
            }

            // Decode clock from STATUS_IDX_HOUR and STATUS_IDX_MINUTE
            {
                uint8_t new_hour   = input_queue[STATUS_IDX_HOUR];
                uint8_t new_minute = input_queue[STATUS_IDX_MINUTE];
                if (new_hour != spaState.hour || new_minute != spaState.minutes)
                {
                    spaState.hour    = new_hour;
                    spaState.minutes = new_minute;
                }
            }

            spaState.rest_mode = input_queue[STATUS_IDX_REST_MODE];

            // Decode heat state and temperature range from STATUS_IDX_HEAT_RANGE
            spaState.heat_state = read_bit(input_queue[STATUS_IDX_HEAT_RANGE], STATUS_HEAT_BIT);

            double spa_component_state = read_bit(input_queue[STATUS_IDX_HEAT_RANGE], STATUS_HIGHRANGE_BIT);
            if (spa_component_state != spaState.highrange)
            {
                ESP_LOGD(TAG, "Spa/highrange/state: %.0f", spa_component_state);
                spaState.highrange = spa_component_state;
            }

            // Decode jet states from STATUS_IDX_JETS (2 bits each)
            spa_component_state = extract_jet1_state(input_queue[STATUS_IDX_JETS]);
            if (spa_component_state != spaState.jet1)
            {
                ESP_LOGD(TAG, "Spa/jet_1/state: %.0f", spa_component_state);
                spaState.jet1 = spa_component_state;
            }

            spa_component_state = extract_jet2_state(input_queue[STATUS_IDX_JETS]);
            if (spa_component_state != spaState.jet2)
            {
                ESP_LOGD(TAG, "Spa/jet_2/state: %.0f", spa_component_state);
                spaState.jet2 = spa_component_state;
            }

            spa_component_state = extract_jet3_state(input_queue[STATUS_IDX_JETS]);
            if (spa_component_state != spaState.jet3)
            {
                ESP_LOGD(TAG, "Spa/jet_3/state: %.0f", spa_component_state);
                spaState.jet3 = spa_component_state;
            }

            spa_component_state = extract_jet4_state(input_queue[STATUS_IDX_JETS]);
            if (spa_component_state != spaState.jet4)
            {
                ESP_LOGD(TAG, "Spa/jet_4/state: %.0f", spa_component_state);
                spaState.jet4 = spa_component_state;
            }

            // Decode circulation and blower states from STATUS_IDX_CIRC_BLOWER
            spa_component_state = read_bit(input_queue[STATUS_IDX_CIRC_BLOWER], STATUS_CIRC_BIT);
            if (spa_component_state != spaState.circulation)
            {
                ESP_LOGD(TAG, "Spa/circ/state: %.0f", spa_component_state);
                spaState.circulation = spa_component_state;
            }

            spa_component_state = read_bit(input_queue[STATUS_IDX_CIRC_BLOWER], STATUS_BLOWER_BIT);
            if (spa_component_state != spaState.blower)
            {
                ESP_LOGD(TAG, "Spa/blower/state: %.0f", spa_component_state);
                spaState.blower = spa_component_state;
            }

            // Decode light states from STATUS_IDX_LIGHTS
            spa_component_state = extract_light1_state(input_queue[STATUS_IDX_LIGHTS]);
            if (spa_component_state != spaState.light)
            {
                ESP_LOGD(TAG, "Spa/light/state: %.0f", spa_component_state);
                spaState.light = spa_component_state;
            }

            spa_component_state = extract_light2_state(input_queue[STATUS_IDX_LIGHTS]);
            if (spa_component_state != spaState.light2)
            {
                ESP_LOGD(TAG, "Spa/light2/state: %.0f", spa_component_state);
                spaState.light2 = spa_component_state;
            }

            // Decode cleanup cycle state from lower nibble of STATUS_IDX_CLEANUP
            uint8_t cleanup_cycle_value = decode_cleanup_cycle(input_queue[STATUS_IDX_CLEANUP]);
            if (cleanup_cycle_value != spaState.cleanup_cycle)
            {
                ESP_LOGD(TAG, "Spa/cleanup_cycle/state: %d (raw=0x%02X)",
                         cleanup_cycle_value, input_queue[STATUS_IDX_CLEANUP] & CLEANUP_NIBBLE_MASK);
                spaState.cleanup_cycle = cleanup_cycle_value;
            }

            // Decode reminder type from STATUS_IDX_REMINDER
            uint8_t reminder_value = input_queue[STATUS_IDX_REMINDER];
            if (reminder_value != spaState.reminder)
            {
                ESP_LOGD(TAG, "Spa/reminder/state: 0x%02X", reminder_value);
                spaState.reminder = reminder_value;
            }

            // Decode sensor A and B temperatures.
            // These are only valid when STATUS_IDX_SPA_STATE == SPA_STATE_AB_TEMPS_ACTIVE.
            auto convert_status_temp = [this](uint8_t raw_temp) -> float
            {
                if (raw_temp == TEMP_UNKNOWN)
                    return NAN;

                float temp_c = NAN;
                if (spa_temp_scale == TEMP_SCALE::C)
                    temp_c = raw_temp / 2.0f;
                else if (spa_temp_scale == TEMP_SCALE::F)
                    temp_c = convert_f_to_c(raw_temp);

                if (std::isnan(temp_c))
                    return NAN;

                if (esphome_temp_scale == TEMP_SCALE::F)
                    return convert_c_to_f(temp_c);

                return temp_c;
            };

            const bool ab_temperatures_active =
                input_queue[STATUS_IDX_SPA_STATE] == SPA_STATE_AB_TEMPS_ACTIVE;

            float temperature_a = NAN;
            float temperature_b = NAN;
            if (ab_temperatures_active)
            {
                temperature_a = convert_status_temp(input_queue[STATUS_IDX_TEMP_A]);
                temperature_b = convert_status_temp(input_queue[STATUS_IDX_TEMP_B]);
            }

            if (temperature_a != spaState.temperature_a)
            {
                ESP_LOGD(TAG, "Spa/temperature_a/state: %.2f", temperature_a);
                spaState.temperature_a = temperature_a;
            }

            if (temperature_b != spaState.temperature_b)
            {
                ESP_LOGD(TAG, "Spa/temperature_b/state: %.2f", temperature_b);
                spaState.temperature_b = temperature_b;
            }

            last_state_crc = input_queue[input_queue[PROTO_IDX_LENGTH]];

            // Notify all state listeners now that spaState is fully updated
            for (const auto &listener : this->listeners_)
            {
                listener(&spaState);
            }
        }

        void BalboaSpa::decodeFilterSettings()
        {
            spaFilterSettings.filter1_hour = input_queue[FILTER_IDX_F1_HOUR];
            spaFilterSettings.filter1_minute = input_queue[FILTER_IDX_F1_MINUTE];
            spaFilterSettings.filter1_duration_hour = input_queue[FILTER_IDX_F1_DUR_HOUR];
            spaFilterSettings.filter1_duration_minute = input_queue[FILTER_IDX_F1_DUR_MIN];
            spaFilterSettings.filter2_enable = read_bit(input_queue[FILTER_IDX_F2_ENABLE_HOUR], FILTER_F2_ENABLE_BIT);
            // Clear the enable flag bit to get the raw hour value
            spaFilterSettings.filter2_hour = input_queue[FILTER_IDX_F2_ENABLE_HOUR] & ~FILTER_F2_ENABLE_MASK;
            spaFilterSettings.filter2_minute = input_queue[FILTER_IDX_F2_MINUTE];
            spaFilterSettings.filter2_duration_hour = input_queue[FILTER_IDX_F2_DUR_HOUR];
            spaFilterSettings.filter2_duration_minute = input_queue[FILTER_IDX_F2_DUR_MIN];

            // Filter 1 time conversion — use a fixed format string with %02d (standard, portable)
            static const char *format_string = R"({"start":"%02d:%02d","duration":"%02d:%02d"})";
            {
                int len = std::snprintf(nullptr, 0, format_string,
                                        spaFilterSettings.filter1_hour, spaFilterSettings.filter1_minute,
                                        spaFilterSettings.filter1_duration_hour, spaFilterSettings.filter1_duration_minute);
                std::string filter_payload(len + 1, '\0');
                std::snprintf(&filter_payload[0], len + 1, format_string,
                              spaFilterSettings.filter1_hour, spaFilterSettings.filter1_minute,
                              spaFilterSettings.filter1_duration_hour, spaFilterSettings.filter1_duration_minute);
                ESP_LOGD(TAG, "Spa/filter1/state: %s", filter_payload.c_str());
            }

            // Filter 2 time conversion
            ESP_LOGD(TAG, "Spa/filter2_enabled/state: %s", spaFilterSettings.filter2_enable == 1 ? STRON : STROFF);
            {
                int len = std::snprintf(nullptr, 0, format_string,
                                        spaFilterSettings.filter2_hour, spaFilterSettings.filter2_minute,
                                        spaFilterSettings.filter2_duration_hour, spaFilterSettings.filter2_duration_minute);
                std::string filter_payload(len + 1, '\0');
                std::snprintf(&filter_payload[0], len + 1, format_string,
                              spaFilterSettings.filter2_hour, spaFilterSettings.filter2_minute,
                              spaFilterSettings.filter2_duration_hour, spaFilterSettings.filter2_duration_minute);
                ESP_LOGD(TAG, "Spa/filter2/state: %s", filter_payload.c_str());
            }

            filtersettings_request_status = 2;
            filtersettings_update_timer = 0; // Reset timer after successful decode

            // Notify filter listeners about filter settings update
            for (const auto &filter_listener : this->filter_listeners_)
            {
                filter_listener(&spaFilterSettings);
            }

            // Update CRC state to prevent reprocessing the same message
            last_state_crc = input_queue[input_queue[PROTO_IDX_LENGTH]];
        }

        void BalboaSpa::decodeFault()
        {
            // Lookup table mapping fault codes to human-readable messages.
            // Easier to extend than a switch/case — just add a row.
            struct FaultEntry { uint8_t code; const char *message; };
            static const FaultEntry FAULT_TABLE[] = {
                {15, "Sensors are out of sync"},
                {16, "The water flow is low"},
                {17, "The water flow has failed"},
                {18, "The settings have been reset"},
                {19, "Priming Mode"},
                {20, "The clock has failed"},
                {21, "The settings have been reset"},
                {22, "Program memory failure"},
                {26, "Sensors are out of sync -- Call for service"},
                {27, "The heater is dry"},
                {28, "The heater may be dry"},
                {29, "The water is too hot"},
                {30, "The heater is too hot"},
                {31, "Sensor A Fault"},
                {32, "Sensor B Fault"},
                {34, "A pump may be stuck on"},
                {35, "Hot fault"},
                {36, "The GFCI test failed"},
                {37, "Standby Mode (Hold Mode)"},
            };

            spaFaultLog.total_entries = input_queue[FAULT_IDX_TOTAL_ENTRIES];
            spaFaultLog.current_entry = input_queue[FAULT_IDX_CURRENT_ENTRY];
            spaFaultLog.fault_code    = input_queue[FAULT_IDX_FAULT_CODE];

            spaFaultLog.fault_message = "Unknown error";
            for (const auto &entry : FAULT_TABLE)
            {
                if (entry.code == spaFaultLog.fault_code)
                {
                    spaFaultLog.fault_message = entry.message;
                    break;
                }
            }

            spaFaultLog.days_ago = input_queue[FAULT_IDX_DAYS_AGO];
            spaFaultLog.hour     = input_queue[FAULT_IDX_HOUR];
            spaFaultLog.minutes  = input_queue[FAULT_IDX_MINUTES];
            ESP_LOGD(TAG, "Spa/fault/Entries: %d", spaFaultLog.total_entries);
            ESP_LOGD(TAG, "Spa/fault/Entry: %d",   spaFaultLog.current_entry);
            ESP_LOGD(TAG, "Spa/fault/Code: %d",    spaFaultLog.fault_code);
            ESP_LOGD(TAG, "Spa/fault/Message: %s", spaFaultLog.fault_message.c_str());
            ESP_LOGD(TAG, "Spa/fault/DaysAgo: %d", spaFaultLog.days_ago);
            ESP_LOGD(TAG, "Spa/fault/Hours: %d",   spaFaultLog.hour);
            ESP_LOGD(TAG, "Spa/fault/Minutes: %d", spaFaultLog.minutes);
            faultlog_request_status = 2;

            // Notify fault log listeners
            for (const auto &listener : this->fault_log_listeners_)
            {
                listener(&spaFaultLog);
            }

            // Update CRC state to prevent reprocessing the same message
            last_state_crc = input_queue[input_queue[PROTO_IDX_LENGTH]];
        }

        bool BalboaSpa::is_communicating()
        {
            return client_id != 0;
        }

        void BalboaSpa::set_spa_temp_scale(TEMP_SCALE scale)
        {
            spa_temp_scale = scale;
        }

        void BalboaSpa::set_esphome_temp_scale(TEMP_SCALE scale)
        {
            esphome_temp_scale = scale;
        }

        void BalboaSpa::set_client_id(uint8_t id)
        {
            if (id >= 1 && id <= CLIENT_ID_MAX)
            {
                client_id_override = id;
                use_client_id_override = true;
                ESP_LOGD(TAG, "Client ID override set to %d", id);
            }
            else
            {
                ESP_LOGW(TAG, "Invalid client ID override %d, must be between 1 and %d", id, CLIENT_ID_MAX);
            }
        }

        float BalboaSpa::convert_c_to_f(float c)
        {
            return (c * 9.0 / 5.0) + 32.0;
        }

        float BalboaSpa::convert_f_to_c(float f)
        {
            return (f - 32.0) * 5.0 / 9.0;
        }
    } // namespace balboa_spa
} // namespace esphome
