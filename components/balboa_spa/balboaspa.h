#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"

#include "spa_types.h"
#include "spa_config.h"
#include "spa_state.h"
#include "CircularBuffer.h"
#include "protocol_definitions.h"
#include "bitfield_helpers.h"
#include "message_builder.h"
#include "buffer_utilities.h"
#include <string>
#include <vector>
#include <cstdio>

namespace esphome
{
  namespace balboa_spa
  {

    static const uint8_t ESPHOME_BALBOASPA_MIN_TEMPERATURE_C = 7;
    static const uint8_t ESPHOME_BALBOASPA_MAX_TEMPERATURE_C = 40;
    static const uint8_t ESPHOME_BALBOASPA_MIN_TEMPERATURE_F = 60;
    static const uint8_t ESPHOME_BALBOASPA_MAX_TEMPERATURE_F = 104;

    static const float ESPHOME_BALBOASPA_POLLING_INTERVAL = 50; // frequency to poll uart device

    static constexpr const char *STRON = "ON";
    static constexpr const char *STROFF = "OFF";

    // Maximum valid temperature for sanity-checking decoded readings (°C).
    static constexpr float ESPHOME_BALBOASPA_MAX_VALID_TEMP_C = 80.0f;

    // Internal sentinel: send_command value meaning "set temperature pending".
    static constexpr uint8_t SEND_CMD_SET_TEMP = 0xFE;

    enum TEMP_SCALE : uint8_t
    {
      UNDEFINED = 254,
      F = 0,
      C = 1
    };

    class BalboaSpa : public uart::UARTDevice, public PollingComponent
    {
    public:
      BalboaSpa() : PollingComponent(ESPHOME_BALBOASPA_POLLING_INTERVAL) {}
      void setup() override;
      void update() override;
      float get_setup_priority() const override;

      SpaConfig get_current_config();
      SpaState *get_current_state();
      SpaFilterSettings *get_current_filter_settings();
      SpaFaultLog *get_current_fault_log();

      void set_temp(float temp);
      void set_hour(int hour);
      void set_minute(int minute);
      void set_timescale(bool is_24h);
      void set_filter1_config(uint8_t start_hour, uint8_t start_minute, uint8_t duration_hour, uint8_t duration_minute);
      void set_filter2_config(uint8_t start_hour, uint8_t start_minute, uint8_t duration_hour, uint8_t duration_minute);
      void set_filter1_start_time(uint8_t hour, uint8_t minute);
      void set_filter1_duration(uint8_t hour, uint8_t minute);
      void set_filter2_start_time(uint8_t hour, uint8_t minute);
      void set_filter2_duration(uint8_t hour, uint8_t minute);
      void disable_filter2();
      void toggle_light();
      void toggle_light2();
      void toggle_jet1();
      void toggle_jet2();
      void toggle_jet3();
      void toggle_jet4();
      void toggle_blower();
      void set_highrange(bool high);
      void clear_reminder();

      void set_spa_temp_scale(TEMP_SCALE scale);
      void set_esphome_temp_scale(TEMP_SCALE scale);
      void set_client_id(uint8_t id);

      bool is_communicating();

      void register_listener(const std::function<void(SpaState *)> &func) { this->listeners_.push_back(func); }
      void register_filter_listener(const std::function<void(SpaFilterSettings *)> &func) { this->filter_listeners_.push_back(func); }
      void register_fault_log_listener(const std::function<void(SpaFaultLog *)> &func) { this->fault_log_listeners_.push_back(func); }

      bool get_restmode();
      void toggle_heat();
      void request_config_update();
      void request_filter_settings_update();
      void request_fault_log_update();

    private:
      CircularBuffer<uint8_t, 100> input_queue;
      CircularBuffer<uint8_t, 100> output_queue;
      uint8_t received_byte, loop_index, temp_index;
      uint8_t last_state_crc = 0x00;
      uint8_t send_command = 0x00;
      uint8_t target_temperature = 0x00;
      uint8_t pending_time_hour = 0x00;   // hour for the next SET_TIME command
      uint8_t pending_time_minute = 0x00; // minute for the next SET_TIME command
      uint8_t target_filter1_start_hour = 0x00;
      uint8_t target_filter1_start_minute = 0x00;
      uint8_t target_filter1_duration_hour = 0x00;
      uint8_t target_filter1_duration_minute = 0x00;
      uint8_t target_filter2_start_hour = 0x00;
      uint8_t target_filter2_start_minute = 0x00;
      uint8_t target_filter2_duration_hour = 0x00;
      uint8_t target_filter2_duration_minute = 0x00;
      bool target_filter2_enable = false;
      uint8_t client_id = 0x00;
      uint8_t client_id_override = 0x00;
      bool use_client_id_override = false;
      uint32_t last_received_time = 0; // initialised to millis() in setup()
      uint8_t send_preference_code = 0;
      uint8_t send_preference_data = 0;

      TEMP_SCALE spa_temp_scale = TEMP_SCALE::UNDEFINED;
      TEMP_SCALE esphome_temp_scale = TEMP_SCALE::C;
      float convert_c_to_f(float c);
      float convert_f_to_c(float f);

      std::vector<std::function<void(SpaState *)>> listeners_;
      std::vector<std::function<void(SpaFilterSettings *)>> filter_listeners_;
      std::vector<std::function<void(SpaFaultLog *)>> fault_log_listeners_;

      char config_request_status = 0;         // stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
      char faultlog_request_status = 0;       // stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
      char filtersettings_request_status = 0; // stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
      char faultlog_update_timer = 0;         // temp logic so we only get the fault log once per 5 minutes
      uint16_t filtersettings_update_timer = 0;   // timer for periodic filter settings requests (every 5 minutes)

      SpaConfig spaConfig;
      SpaState spaState;
      SpaFaultLog spaFaultLog;
      SpaFilterSettings spaFilterSettings;

      void read_serial();

      // Message dispatcher helpers (called from read_serial())
      void handle_unregistered();
      void handle_id_request();
      void handle_id_acknowledge();
      void handle_ready_to_send();
      void handle_status_update();
      void handle_config_response();
      void handle_filter_settings_response();
      void handle_fault_log_response();

      uint8_t crc8(CircularBuffer<uint8_t, 100> &data, bool ignore_delimiter);
      void ID_request();
      void ID_ack();
      void rs485_send();
      void print_msg(CircularBuffer<uint8_t, 100> &data);
      void decodeSettings();
      void decodeState();
      void decodeFilterSettings();
      void decodeFault();

      // Populate all target_filter* fields from the last-known spaFilterSettings so
      // that partial-update setters don't accidentally zero out unrelated fields.
      void sync_filter_targets_from_settings();
    };

  } // namespace balboa_spa
} // namespace esphome
