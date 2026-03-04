#pragma once

#include <stdint.h>

// Balboa RS485 Protocol Definitions
//
// Frame format:
//   [0]      SOF (0x7E)
//   [1]      Frame length (number of bytes from [1] to CRC, inclusive)
//   [2]      Destination/source byte (client_id, 0xFF = broadcast, 0xFE = unregistered)
//   [3]      Message subtype (0xBF = from client to spa, 0xAF = from spa to client)
//   [4]      Message type
//   [5..N-2] Data bytes
//   [N-1]    CRC8 (over bytes [1]..[N-2])
//   [N]      EOF (0x7E)
//
// CRC8 algorithm:
//   Polynomial: x^8 + x^2 + x + 1 (0x07)
//   Initial value: 0x02
//   Final XOR: 0x02
//   Example: frame [7E, 07, FE, BF, 01, 02, F1, 73, <CRC>, 7E]
//   CRC is computed over bytes [07, FE, BF, 01, 02, F1, 73] (skip SOF and trailing CRC+EOF)

namespace esphome
{
    namespace balboa_spa
    {

        // Frame delimiter
        static const uint8_t PROTO_SOF_EOF = 0x7E;

        // Frame field indices (absolute byte positions in the frame)
        static const uint8_t PROTO_IDX_LENGTH = 1;
        static const uint8_t PROTO_IDX_DEST = 2;
        static const uint8_t PROTO_IDX_SUBTYPE = 3;
        static const uint8_t PROTO_IDX_MSG_TYPE = 4;
        static const uint8_t PROTO_IDX_DATA_START = 5;

        // Special destination/source address bytes
        static const uint8_t PROTO_ADDR_BROADCAST = 0xFF;
        static const uint8_t PROTO_ADDR_UNREGISTERED = 0xFE;
        static const uint8_t PROTO_SUBTYPE_FROM_CLIENT = 0xBF;
        static const uint8_t PROTO_SUBTYPE_FROM_SPA = 0xAF;

        // Message type codes
        static const uint8_t MSG_ID_NEW_CLIENTS = 0x00;     // FE BF 00 - Any new clients?
        static const uint8_t MSG_ID_NEW_CLIENT_REQ = 0x01;  // FE BF 01 - New client request (ID_request)
        static const uint8_t MSG_ID_CLIENT_ID_OFFER = 0x02; // FE BF 02 - Got new client ID
        static const uint8_t MSG_ID_CLIENT_ACK = 0x03;      // client_id BF 03 - Client acknowledges ID
        static const uint8_t MSG_ID_CLEAR_TO_SEND = 0x06;   // client_id BF 06 - Ready to Send
        static const uint8_t MSG_ID_NOTHING_TO_SEND = 0x07; // client_id BF 07 - Nothing to Send
        static const uint8_t MSG_ID_STATUS_UPDATE = 0x13;   // FF AF 13 - Status Update
        static const uint8_t MSG_ID_SET_TEMP = 0x20;        // Set temperature
        static const uint8_t MSG_ID_SET_TIME = 0x21;        // Set time
        static const uint8_t MSG_ID_READ_CONFIG = 0x22;     // Read configuration (request)
        static const uint8_t MSG_ID_FILTER_CONFIG = 0x23;   // Filter configuration (set or response)
        static const uint8_t MSG_ID_SET_FILTER = 0x23;      // Alias: Set filter configuration
        static const uint8_t MSG_ID_SET_PREFERENCE = 0x27;  // Set spa preference
        static const uint8_t MSG_ID_FAULT_LOG = 0x28;       // Fault log response
        static const uint8_t MSG_ID_CONFIG_RESPONSE = 0x2E; // Configuration response
        static const uint8_t MSG_ID_TOGGLE = 0x11;          // Toggle command

        // Toggle command item codes (used as data byte with MSG_ID_TOGGLE)
        static const uint8_t TOGGLE_CLEAR_REMINDER = 0x03;
        static const uint8_t TOGGLE_JET1 = 0x04;
        static const uint8_t TOGGLE_JET2 = 0x05;
        static const uint8_t TOGGLE_JET3 = 0x06;
        static const uint8_t TOGGLE_JET4 = 0x07;
        static const uint8_t TOGGLE_BLOWER = 0x0C;
        static const uint8_t TOGGLE_LIGHT1 = 0x11;
        static const uint8_t TOGGLE_LIGHT2 = 0x12;
        static const uint8_t TOGGLE_HIGHRANGE = 0x50;
        static const uint8_t TOGGLE_HEAT = 0x51;

        // Read config sub-request data bytes (used with MSG_ID_READ_CONFIG)
        static const uint8_t CONFIG_SUB_SETTINGS_B1 = 0x00; // b1 for settings request
        static const uint8_t CONFIG_SUB_SETTINGS_B2 = 0x00; // b2 for settings request
        static const uint8_t CONFIG_SUB_SETTINGS_B3 = 0x01; // b3 for settings request
        static const uint8_t CONFIG_SUB_FAULT_B1 = 0x20;    // b1 for fault log request
        static const uint8_t CONFIG_SUB_FAULT_B2 = 0xFF;    // b2 for fault log request
        static const uint8_t CONFIG_SUB_FAULT_B3 = 0x00;    // b3 for fault log request
        static const uint8_t CONFIG_SUB_FILTER_B1 = 0x01;   // b1 for filter settings request
        static const uint8_t CONFIG_SUB_FILTER_B2 = 0x00;   // b2 for filter settings request
        static const uint8_t CONFIG_SUB_FILTER_B3 = 0x00;   // b3 for filter settings request

        // Preference codes (used with MSG_ID_SET_PREFERENCE)
        static const uint8_t PREFERENCE_TIMESCALE = 0x02; // 12h/24h clock mode

        // Maximum valid client ID
        static const uint8_t CLIENT_ID_MAX = 0x2F;

        // ----------------------------------------------------------------
        // Status Update (MSG_ID_STATUS_UPDATE = 0x13) byte indices
        // (absolute frame positions; data arguments begin at index 5)
        // ----------------------------------------------------------------

        // Byte 0 of payload (frame index 5): spa operating state
        //   0x14 = A/B temperature sensors active
        static const uint8_t STATUS_IDX_SPA_STATE = 5;

        // Byte 1 of payload (frame index 6): reminder type
        //   0x00=None, 0x04=Clean filter, 0x09=Check sanitizer,
        //   0x0A=Check pH, 0x1E=Fault
        static const uint8_t STATUS_IDX_REMINDER = 6;

        // Byte 2 of payload (frame index 7): current water temperature
        //   0xFF = temperature unknown / not yet measured
        static const uint8_t STATUS_IDX_CURRENT_TEMP = 7;

        // Bytes 3-4 of payload (frame indices 8-9): current clock hour and minute
        static const uint8_t STATUS_IDX_HOUR = 8;
        static const uint8_t STATUS_IDX_MINUTE = 9;

        // Byte 5 of payload (frame index 10): rest/ready mode
        static const uint8_t STATUS_IDX_REST_MODE = 10;

        // Bytes 7-8 of payload (frame indices 12-13): sensor A and B temperatures
        //   Only valid when STATUS_IDX_SPA_STATE == SPA_STATE_AB_TEMPS_ACTIVE
        static const uint8_t STATUS_IDX_TEMP_A = 12;
        static const uint8_t STATUS_IDX_TEMP_B = 13;

        // Byte 10 of payload (frame index 15): heat state and temperature range flags
        static const uint8_t STATUS_IDX_HEAT_RANGE = 15;

        // Byte 11 of payload (frame index 16): jet pump states (2 bits each)
        static const uint8_t STATUS_IDX_JETS = 16;

        // Byte 13 of payload (frame index 18): circulation pump and blower states
        static const uint8_t STATUS_IDX_CIRC_BLOWER = 18;

        // Byte 14 of payload (frame index 19): light states
        static const uint8_t STATUS_IDX_LIGHTS = 19;

        // Byte 19 of payload (frame index 24): cleanup cycle state (lower nibble)
        static const uint8_t STATUS_IDX_CLEANUP = 24;

        // Byte 20 of payload (frame index 25): target/set temperature
        static const uint8_t STATUS_IDX_TARGET_TEMP = 25;

        // Status update bit positions in STATUS_IDX_HEAT_RANGE byte
        static const uint8_t STATUS_HEAT_BIT = 4;      // Heating active
        static const uint8_t STATUS_HIGHRANGE_BIT = 2;  // High temperature range active

        // Status update bit positions in STATUS_IDX_CIRC_BLOWER byte
        static const uint8_t STATUS_CIRC_BIT = 1;   // Circulation pump active
        static const uint8_t STATUS_BLOWER_BIT = 2; // Blower active

        // Jet pump bit masks and shifts within STATUS_IDX_JETS byte
        // Each jet occupies 2 bits: 0=OFF, 1=LOW, 2=HIGH
        static const uint8_t STATUS_JET1_MASK = 0x03;  // bits 0-1
        static const uint8_t STATUS_JET2_MASK = 0x0C;  // bits 2-3
        static const uint8_t STATUS_JET2_SHIFT = 2;
        static const uint8_t STATUS_JET3_MASK = 0x30;  // bits 4-5
        static const uint8_t STATUS_JET3_SHIFT = 4;
        static const uint8_t STATUS_JET4_MASK = 0xC0;  // bits 6-7
        static const uint8_t STATUS_JET4_SHIFT = 6;

        // Light state values within STATUS_IDX_LIGHTS byte
        static const uint8_t STATUS_LIGHT1_ON_VALUE = 0x03; // bits 0-1 == 0x03 means light1 ON
        static const uint8_t STATUS_LIGHT2_MASK = 0x0C;     // bits 2-3 for light2
        static const uint8_t STATUS_LIGHT2_SHIFT = 2;

        // Cleanup cycle constants (applied to lower nibble of STATUS_IDX_CLEANUP)
        static const uint8_t CLEANUP_NIBBLE_MASK = 0x0F;
        static const uint8_t CLEANUP_ON_VALUE = 0x0C;  // Cleanup cycle active
        static const uint8_t CLEANUP_OFF_V1 = 0x04;    // Cleanup cycle inactive (variant 1)
        static const uint8_t CLEANUP_OFF_V2 = 0x02;    // Cleanup cycle inactive (variant 2)
        static const uint8_t CLEANUP_OFF_V3 = 0x00;    // Cleanup cycle inactive (variant 3)

        // Spa operating state values (STATUS_IDX_SPA_STATE)
        static const uint8_t SPA_STATE_AB_TEMPS_ACTIVE = 0x14; // A/B temperature mode

        // ----------------------------------------------------------------
        // Config Response (MSG_ID_CONFIG_RESPONSE = 0x2E) byte indices
        //
        // Note: These are absolute frame byte positions (same as STATUS_IDX_* above).
        // In the config response frame the 'subtype' field at position [3] is
        // repurposed by the spa firmware to carry temperature scale / clock mode,
        // so CONFIG_IDX_TEMP_CLOCK = 3 is intentional and correct.
        // ----------------------------------------------------------------

        static const uint8_t CONFIG_IDX_TEMP_CLOCK = 3;    // Temperature scale (bit 0) + clock mode (bit 1)
        static const uint8_t CONFIG_IDX_PUMPS_1_4 = 5;     // Pump 1-4 configuration (2 bits each)
        static const uint8_t CONFIG_IDX_PUMPS_5_6 = 6;     // Pump 5-6 configuration (2 bits each)
        static const uint8_t CONFIG_IDX_LIGHTS = 7;        // Light 1/2 configuration
        static const uint8_t CONFIG_IDX_CIRC_BLOWER = 8;   // Circulation pump + blower
        static const uint8_t CONFIG_IDX_AUX_MISTER = 9;    // Auxiliary outputs + mister

        // Bit masks in CONFIG_IDX_TEMP_CLOCK
        static const uint8_t CONFIG_TEMP_SCALE_MASK = 0x01; // 0=Fahrenheit, 1=Celsius
        static const uint8_t CONFIG_CLOCK_MODE_SHIFT = 1;   // 0=12h, 1=24h

        // Bit masks in CONFIG_IDX_CIRC_BLOWER
        static const uint8_t CONFIG_CIRC_MASK = 0x80;
        static const uint8_t CONFIG_BLOWER_MASK = 0x03;

        // Bit masks in CONFIG_IDX_AUX_MISTER
        static const uint8_t CONFIG_MISTER_MASK = 0x30;
        static const uint8_t CONFIG_AUX1_MASK = 0x01;
        static const uint8_t CONFIG_AUX2_MASK = 0x02;

        // ----------------------------------------------------------------
        // Filter Settings (MSG_ID_FILTER_CONFIG = 0x23) byte indices
        // ----------------------------------------------------------------

        static const uint8_t FILTER_IDX_F1_HOUR = 5;
        static const uint8_t FILTER_IDX_F1_MINUTE = 6;
        static const uint8_t FILTER_IDX_F1_DUR_HOUR = 7;
        static const uint8_t FILTER_IDX_F1_DUR_MIN = 8;

        // Byte 9: bit 7 = filter 2 enable, bits 0-6 = filter 2 start hour
        static const uint8_t FILTER_IDX_F2_ENABLE_HOUR = 9;
        static const uint8_t FILTER_IDX_F2_MINUTE = 10;
        static const uint8_t FILTER_IDX_F2_DUR_HOUR = 11;
        static const uint8_t FILTER_IDX_F2_DUR_MIN = 12;

        static const uint8_t FILTER_F2_ENABLE_BIT = 7;    // Bit position of F2 enable flag
        static const uint8_t FILTER_F2_ENABLE_MASK = 0x80; // Mask for F2 enable flag

        // ----------------------------------------------------------------
        // Fault Log (MSG_ID_FAULT_LOG = 0x28) byte indices
        // ----------------------------------------------------------------

        static const uint8_t FAULT_IDX_TOTAL_ENTRIES = 5;
        static const uint8_t FAULT_IDX_CURRENT_ENTRY = 6;
        static const uint8_t FAULT_IDX_FAULT_CODE = 7;
        static const uint8_t FAULT_IDX_DAYS_AGO = 8;
        static const uint8_t FAULT_IDX_HOUR = 9;
        static const uint8_t FAULT_IDX_MINUTES = 10;

        // ----------------------------------------------------------------
        // ID negotiation byte indices (within FE BF xx messages)
        // ----------------------------------------------------------------

        static const uint8_t ID_NEGO_IDX_TYPE = 4;      // Message subtype
        static const uint8_t ID_NEGO_IDX_CLIENT_ID = 5; // Proposed client ID in FE BF 02

        // ----------------------------------------------------------------
        // Reminder type values (STATUS_IDX_REMINDER)
        // ----------------------------------------------------------------

        static const uint8_t REMINDER_NONE = 0x00;
        static const uint8_t REMINDER_CLEAN_FILTER = 0x04;
        static const uint8_t REMINDER_CHECK_SANITIZER = 0x09;
        static const uint8_t REMINDER_CHECK_PH = 0x0A;
        static const uint8_t REMINDER_FAULT = 0x1E;

        // ----------------------------------------------------------------
        // Miscellaneous sentinel values
        // ----------------------------------------------------------------

        // Temperature byte value indicating "not available / unknown"
        static const uint8_t TEMP_UNKNOWN = 0xFF;

        // CRC8 algorithm parameters
        static const uint8_t CRC8_POLYNOMIAL = 0x07;  // x^8 + x^2 + x + 1
        static const uint8_t CRC8_INIT = 0x02;
        static const uint8_t CRC8_FINAL_XOR = 0x02;
        // CRC8 example: for frame [7E, 07, FE, BF, 01, 02, F1, 73, <CRC>, 7E]
        //   crc8(ignore_delimiter=true) operates on bytes [07, FE, BF, 01, 02, F1, 73]
        //   (skips SOF at index 0 and trailing CRC+EOF bytes)

    } // namespace balboa_spa
} // namespace esphome
