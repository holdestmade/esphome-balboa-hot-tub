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

    inline constexpr uint8_t ESPHOME_BALBOASPA_MIN_TEMPERATURE_C = 7;
    inline constexpr uint8_t ESPHOME_BALBOASPA_MAX_TEMPERATURE_C = 40;
    inline constexpr uint8_t ESPHOME_BALBOASPA_MIN_TEMPERATURE_F = 60;
    inline constexpr uint8_t ESPHOME_BALBOASPA_MAX_TEMPERATURE_F = 104;

    // update() only runs housekeeping (communication watchdog, periodic filter
    // settings refresh); serial data is consumed in loop().
    inline constexpr uint32_t ESPHOME_BALBOASPA_POLLING_INTERVAL = 1000; // ms

    // Mark the spa as disconnected when no bytes arrive for this long.
    inline constexpr uint32_t COMMUNICATION_TIMEOUT_MS = 10000;

    // How often to re-request the filter settings.
    inline constexpr uint32_t FILTER_SETTINGS_REFRESH_MS = 5 * 60 * 1000;

    inline constexpr const char *STRON = "ON";
    inline constexpr const char *STROFF = "OFF";

    // Maximum valid temperature for sanity-checking decoded readings (°C).
    inline constexpr float ESPHOME_BALBOASPA_MAX_VALID_TEMP_C = 80.0f;

    // Internal sentinel: command value meaning "set temperature pending".
    // Must not collide with any MSG_ID_* or TOGGLE_* code.
    inline constexpr uint8_t SEND_CMD_SET_TEMP = 0xFE;

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
      void loop() override;
      float get_setup_priority() const override;

      SpaConfig get_current_config();
      SpaState *get_current_state();
      SpaFilterSettings *get_current_filter_settings();
      SpaFaultLog *get_current_fault_log();

      void set_temp(float temp);
      void set_time(int hour, int minute);
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
      TEMP_SCALE get_esphome_temp_scale() const { return esphome_temp_scale; }

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
      // Outbound command waiting for a Clear-To-Send slot.
      // `command` is a MSG_ID_* value, a TOGGLE_* item code, or SEND_CMD_SET_TEMP.
      struct PendingCommand
      {
        uint8_t command;
        uint8_t data1;
        uint8_t data2;
      };
      static constexpr size_t COMMAND_QUEUE_CAPACITY = 8;

      CircularBuffer<uint8_t, 100> input_queue;
      CircularBuffer<uint8_t, 100> output_queue;

      PendingCommand command_queue_[COMMAND_QUEUE_CAPACITY];
      size_t command_queue_head_ = 0;
      size_t command_queue_count_ = 0;

      // Last seen CRC per message type, used to skip decoding unchanged messages.
      uint8_t last_status_crc_ = 0x00;
      uint8_t last_config_crc_ = 0x00;
      uint8_t last_filter_crc_ = 0x00;
      uint8_t last_fault_crc_ = 0x00;

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

      TEMP_SCALE spa_temp_scale = TEMP_SCALE::UNDEFINED;
      TEMP_SCALE esphome_temp_scale = TEMP_SCALE::C;
      static float convert_c_to_f(float c);
      static float convert_f_to_c(float f);

      std::vector<std::function<void(SpaState *)>> listeners_;
      std::vector<std::function<void(SpaFilterSettings *)>> filter_listeners_;
      std::vector<std::function<void(SpaFaultLog *)>> fault_log_listeners_;

      enum class RequestStatus : uint8_t
      {
        WANTED = 0,    // should be requested at the next opportunity
        REQUESTED = 1, // request has been sent, waiting for the response
        RECEIVED = 2   // response has been decoded
      };
      RequestStatus config_request_status = RequestStatus::WANTED;
      RequestStatus faultlog_request_status = RequestStatus::WANTED;
      RequestStatus filtersettings_request_status = RequestStatus::WANTED;
      uint32_t filtersettings_received_time = 0; // for the periodic refresh

      SpaConfig spaConfig;
      SpaState spaState;
      SpaFaultLog spaFaultLog;
      SpaFilterSettings spaFilterSettings;

      // Command queue helpers
      bool enqueue_command(uint8_t command, uint8_t data1 = 0, uint8_t data2 = 0);
      bool dequeue_command(PendingCommand &cmd);
      PendingCommand *find_queued_command(uint8_t command);

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

      // CRC byte of the frame currently held in input_queue.
      uint8_t packet_crc() const { return input_queue[input_queue[PROTO_IDX_LENGTH]]; }

      uint8_t crc8(const CircularBuffer<uint8_t, 100> &data, bool ignore_delimiter);
      void ID_request();
      void ID_ack();
      void rs485_send();
      void print_msg(const CircularBuffer<uint8_t, 100> &data);
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
