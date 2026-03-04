#pragma once

#include <stdint.h>
#include "CircularBuffer.h"
#include "protocol_definitions.h"

// ProtocolMessageBuilder - Helper for constructing Balboa RS485 protocol messages.
//
// Messages are accumulated directly into the caller-supplied output queue.
// Each static factory method appends the payload bytes for a specific message
// type; the caller is responsible for wrapping the payload with length, CRC,
// and SOF/EOF delimiters (done by BalboaSpa::rs485_send()).
//
// Usage example:
//   ProtocolMessageBuilder::build_nothing_to_send(output_queue, client_id);
//   rs485_send();

namespace esphome
{
    namespace balboa_spa
    {

        class ProtocolMessageBuilder
        {
        public:
            // Build a "Nothing to Send" message (client_id BF 07).
            // Sent when the spa issues a Clear-to-Send but the client has nothing to transmit.
            static void build_nothing_to_send(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_NOTHING_TO_SEND);
            }

            // Build a "New Client Request" message (FE BF 01 ...) - announces that a
            // client is present and requests an ID assignment.
            // The trailing bytes 0x02, 0xF1, 0x73 are fixed protocol handshake bytes
            // defined by the Balboa protocol for the client identification request.
            static void build_id_request(CircularBuffer<uint8_t, 100> &queue)
            {
                queue.push(PROTO_ADDR_UNREGISTERED);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_NEW_CLIENT_REQ);
                queue.push(0x02); // Fixed handshake byte (protocol-defined)
                queue.push(0xF1); // Fixed handshake byte (protocol-defined)
                queue.push(0x73); // Fixed handshake byte (protocol-defined)
            }

            // Build an "ID Acknowledge" message (client_id BF 03) - confirms receipt
            // of the assigned client ID.
            static void build_id_ack(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_CLIENT_ACK);
            }

            // Build a "Set Time" message (client_id BF 21 HH MM).
            static void build_set_time(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                       uint8_t hour, uint8_t minute)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_SET_TIME);
                queue.push(hour);
                queue.push(minute);
            }

            // Build a "Set Temperature" message (client_id BF 20 TEMP).
            static void build_set_temp(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                       uint8_t temperature)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_SET_TEMP);
                queue.push(temperature);
            }

            // Build a "Toggle" command message (client_id BF 11 CODE 00).
            static void build_toggle(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                     uint8_t toggle_code)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_TOGGLE);
                queue.push(toggle_code);
                queue.push(0x00);
            }

            // Build a "Set Preference" message (client_id BF 27 PREF_CODE PREF_DATA).
            static void build_set_preference(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                             uint8_t pref_code, uint8_t pref_data)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_SET_PREFERENCE);
                queue.push(pref_code);
                queue.push(pref_data);
            }

            // Build a "Read Config" request message (client_id BF 22 B1 B2 B3).
            static void build_config_request(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                             uint8_t b1, uint8_t b2, uint8_t b3)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_READ_CONFIG);
                queue.push(b1);
                queue.push(b2);
                queue.push(b3);
            }

            // Build a "Set Filter Configuration" message (client_id BF 23 ...).
            static void build_filter_config(CircularBuffer<uint8_t, 100> &queue, uint8_t client_id,
                                            uint8_t f1_start_hour, uint8_t f1_start_min,
                                            uint8_t f1_dur_hour, uint8_t f1_dur_min,
                                            bool f2_enable,
                                            uint8_t f2_start_hour, uint8_t f2_start_min,
                                            uint8_t f2_dur_hour, uint8_t f2_dur_min)
            {
                queue.push(client_id);
                queue.push(PROTO_SUBTYPE_FROM_CLIENT);
                queue.push(MSG_ID_SET_FILTER);
                queue.push(f1_start_hour);
                queue.push(f1_start_min);
                queue.push(f1_dur_hour);
                queue.push(f1_dur_min);
                // Bit 7 of the filter-2 start-hour byte encodes the enable flag
                queue.push(f2_enable ? (f2_start_hour | FILTER_F2_ENABLE_MASK) : 0x00);
                queue.push(f2_start_min);
                queue.push(f2_dur_hour);
                queue.push(f2_dur_min);
            }
        };

    } // namespace balboa_spa
} // namespace esphome
