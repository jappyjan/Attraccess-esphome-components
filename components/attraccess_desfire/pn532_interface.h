#pragma once

#include <Wire.h>
#include <queue>
#include <array>

namespace esphome
{
    namespace attraccess_desfire
    {

        // PN532 command definitions
        enum PN532Command : uint8_t
        {
            PN532_COMMAND_DIAGNOSE = 0x00,
            PN532_COMMAND_GETFIRMWAREVERSION = 0x02,
            PN532_COMMAND_GETGENERALSTATUS = 0x04,
            PN532_COMMAND_READREGISTER = 0x06,
            PN532_COMMAND_WRITEREGISTER = 0x08,
            PN532_COMMAND_SETSERIALBAUDRATE = 0x10,
            PN532_COMMAND_SETPARAMETERS = 0x12,
            PN532_COMMAND_SAMCONFIGURATION = 0x14,
            PN532_COMMAND_POWERDOWN = 0x16,
            PN532_COMMAND_RFCONFIGURATION = 0x32,
            PN532_COMMAND_RFREGULATIONTEST = 0x58,
            PN532_COMMAND_INJUMPFORDEP = 0x56,
            PN532_COMMAND_INJUMPFORPSL = 0x46,
            PN532_COMMAND_INLISTPASSIVETARGET = 0x4A,
            PN532_COMMAND_INATR = 0x50,
            PN532_COMMAND_INPSL = 0x4E,
            PN532_COMMAND_INDATAEXCHANGE = 0x40,
            PN532_COMMAND_INCOMMUNICATETHRU = 0x42,
            PN532_COMMAND_INDESELECT = 0x44,
            PN532_COMMAND_INRELEASE = 0x52,
            PN532_COMMAND_INSELECT = 0x54,
            PN532_COMMAND_INAUTOPOLL = 0x60,
            PN532_COMMAND_TGINITASTARGET = 0x8C,
            PN532_COMMAND_TGSETGENERALBYTES = 0x92,
            PN532_COMMAND_TGGETDATA = 0x86,
            PN532_COMMAND_TGSETDATA = 0x8E,
            PN532_COMMAND_TGSETMETADATA = 0x94,
            PN532_COMMAND_TGGETINITIATORCOMMAND = 0x88,
            PN532_COMMAND_TGRESPONSETOINITIATOR = 0x90,
            PN532_COMMAND_TGGETTARGETSTATUS = 0x8A
        };

        // PN532 Constants
        static const uint8_t PN532_PREAMBLE = 0x00;
        static const uint8_t PN532_STARTCODE1 = 0x00;
        static const uint8_t PN532_STARTCODE2 = 0xFF;
        static const uint8_t PN532_POSTAMBLE = 0x00;
        static const uint8_t PN532_HOSTTOPN532 = 0xD4;
        static const uint8_t PN532_PN532TOHOST = 0xD5;
        static const uint8_t PN532_ACK[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
        static const uint8_t PN532_NACK[6] = {0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};

    } // namespace attraccess_desfire
} // namespace esphome