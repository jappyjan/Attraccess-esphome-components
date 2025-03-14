#pragma once

#include <vector>
#include <string>
#include <queue>

namespace esphome
{
    namespace attraccess_desfire
    {

        // API communication command types
        enum ApiCommandType : uint8_t
        {
            API_CMD_DEVICE_AUTH = 0x01,
            API_CMD_BUTTON_EVENT = 0x02,
            API_CMD_CARD_DETECTED = 0x03,
            API_CMD_CARD_OPERATION = 0x10,
            API_CMD_RESOURCE_ACTIVATE = 0x20,
            API_CMD_RESOURCE_DEACTIVATE = 0x21,
            API_CMD_STATUS_UPDATE = 0x30,
            API_CMD_AUTHENTICATION_RESULT = 0x40
        };

        // Card operation types
        enum CardOpType : uint8_t
        {
            CARD_OP_TRANSCEIVE = 0x01,
            CARD_OP_SELECT_APPLICATION = 0x02,
            CARD_OP_AUTHENTICATE = 0x03,
            CARD_OP_READ_DATA = 0x04,
            CARD_OP_WRITE_DATA = 0x05
        };

        // Component state machine states
        enum CardState : uint8_t
        {
            CARD_STATE_IDLE = 0,
            CARD_STATE_WAIT_FOR_CARD = 1,
            CARD_STATE_CARD_DETECTED = 2,
            CARD_STATE_EXECUTING_COMMAND = 3,
            CARD_STATE_AUTHORIZED = 4,
            CARD_STATE_DENIED = 5,
            CARD_STATE_ERROR = 6
        };

        // Binary packet constants
        static const uint16_t BINARY_PACKET_MAGIC = 0xBA71; // "BApi"
        static const uint8_t BINARY_PACKET_VERSION = 0x01;

        // Structure for API command
        struct ApiCommand
        {
            uint16_t cmd_id;
            uint8_t cmd_type;
            std::vector<uint8_t> data;
        };

        // Structure for API response
        struct ApiResponse
        {
            uint16_t cmd_id;
            uint8_t status;
            std::vector<uint8_t> data;
        };

    } // namespace attraccess_desfire
} // namespace esphome