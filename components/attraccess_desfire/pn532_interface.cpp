#include "attraccess_desfire.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome
{
    namespace attraccess_desfire
    {

        static const char *const TAG = "attraccess_desfire.pn532";

        bool AttracccessDesfireComponent::init_pn532_()
        {
            // Reset PN532
            if (this->reset_pin_ != nullptr)
            {
                ESP_LOGD(TAG, "Resetting PN532...");
                this->reset_pin_->digital_write(false);
                delay(400);
                this->reset_pin_->digital_write(true);
                delay(100);
            }

            // Get firmware version to verify PN532 is working
            uint8_t response_length = 0;
            this->pn532_packetbuffer_[0] = PN532_COMMAND_GETFIRMWAREVERSION;

            if (!this->send_command_(PN532_COMMAND_GETFIRMWAREVERSION, nullptr, 0))
            {
                ESP_LOGE(TAG, "Failed to send firmware version command");
                return false;
            }

            if (!this->read_response_(PN532_COMMAND_GETFIRMWAREVERSION, this->pn532_packetbuffer_, &response_length))
            {
                ESP_LOGE(TAG, "Failed to read firmware version");
                return false;
            }

            uint32_t firmware_version = this->pn532_packetbuffer_[0];
            firmware_version <<= 8;
            firmware_version |= this->pn532_packetbuffer_[1];
            firmware_version <<= 8;
            firmware_version |= this->pn532_packetbuffer_[2];
            firmware_version <<= 8;
            firmware_version |= this->pn532_packetbuffer_[3];

            ESP_LOGI(TAG, "PN532 Firmware version: %d.%d", this->pn532_packetbuffer_[1], this->pn532_packetbuffer_[2]);

            // Configure SAM (Secure Access Module)
            this->pn532_packetbuffer_[0] = PN532_COMMAND_SAMCONFIGURATION;
            this->pn532_packetbuffer_[1] = 0x01; // Normal mode
            this->pn532_packetbuffer_[2] = 0x14; // Timeout 50ms * 20 = 1 second
            this->pn532_packetbuffer_[3] = 0x01; // Use IRQ pin

            if (!this->send_command_(PN532_COMMAND_SAMCONFIGURATION, this->pn532_packetbuffer_, 4))
            {
                ESP_LOGE(TAG, "Failed to send SAM configuration command");
                return false;
            }

            if (!this->read_response_(PN532_COMMAND_SAMCONFIGURATION, this->pn532_packetbuffer_, &response_length))
            {
                ESP_LOGE(TAG, "Failed to configure SAM");
                return false;
            }

            // Configure RF field
            this->pn532_packetbuffer_[0] = PN532_COMMAND_RFCONFIGURATION;
            this->pn532_packetbuffer_[1] = 0x01; // CfgItem = 1 (RF Field)
            this->pn532_packetbuffer_[2] = 0x01; // RF On

            if (!this->send_command_(PN532_COMMAND_RFCONFIGURATION, this->pn532_packetbuffer_, 3))
            {
                ESP_LOGE(TAG, "Failed to send RF configuration command");
                return false;
            }

            if (!this->read_response_(PN532_COMMAND_RFCONFIGURATION, this->pn532_packetbuffer_, &response_length))
            {
                ESP_LOGE(TAG, "Failed to configure RF field");
                return false;
            }

            return true;
        }

        bool AttracccessDesfireComponent::send_command_(uint8_t command, const uint8_t *data, uint8_t data_len)
        {
            // Check for sane parameters
            if (data_len > 64 - 8)
            {
                ESP_LOGE(TAG, "Command too long: %d bytes", data_len);
                return false;
            }

            // Prepare command buffer
            uint8_t checksum;
            uint8_t cmd_len = data_len + 1; // Add command byte
            uint8_t packet_buffer[64];      // Packet buffer

            packet_buffer[0] = PN532_PREAMBLE;
            packet_buffer[1] = PN532_STARTCODE1;
            packet_buffer[2] = PN532_STARTCODE2;
            packet_buffer[3] = cmd_len;
            packet_buffer[4] = ~cmd_len + 1; // Checksum of length
            packet_buffer[5] = PN532_HOSTTOPN532;
            packet_buffer[6] = command;

            // Copy data to packet buffer
            for (uint8_t i = 0; i < data_len; i++)
            {
                packet_buffer[7 + i] = data[i];
            }

            // Calculate checksum
            checksum = PN532_HOSTTOPN532 + command;
            for (uint8_t i = 0; i < data_len; i++)
            {
                checksum += data[i];
            }

            packet_buffer[7 + data_len] = ~checksum + 1; // Data checksum
            packet_buffer[8 + data_len] = PN532_POSTAMBLE;

            // Send the command
            ESP_LOGV(TAG, "Sending command...");

            // Send all bytes
            this->write(packet_buffer, 9 + data_len);

            // Wait for ACK
            if (!this->wait_ready_(1000))
            {
                ESP_LOGE(TAG, "No ACK received after send");
                return false;
            }

            // Read ACK
            uint8_t ack_buffer[6];
            this->read(ack_buffer, 6);

            // Check if ACK is valid
            if (memcmp(ack_buffer, PN532_ACK, 6) != 0)
            {
                ESP_LOGE(TAG, "Invalid ACK received");
                return false;
            }

            return true;
        }

        bool AttracccessDesfireComponent::read_response_(uint8_t command, uint8_t *response, uint8_t *response_len, uint16_t timeout)
        {
            // Wait for response
            if (!this->wait_ready_(timeout))
            {
                ESP_LOGW(TAG, "No response received within timeout");
                return false;
            }

            // Read response header
            uint8_t header[7];
            this->read(header, 7);

            // Check header validity
            if (header[0] != PN532_PREAMBLE ||
                header[1] != PN532_STARTCODE1 ||
                header[2] != PN532_STARTCODE2)
            {
                ESP_LOGE(TAG, "Invalid response header");
                return false;
            }

            // Length check
            uint8_t length = header[3];
            if (header[4] != (uint8_t)(~length + 1))
            {
                ESP_LOGE(TAG, "Invalid response length checksum");
                return false;
            }

            // Check command response
            if (header[5] != PN532_PN532TOHOST || header[6] != (command + 1))
            {
                ESP_LOGE(TAG, "Invalid command response");
                return false;
            }

            // Read response data
            if (length > 0)
            {
                this->read(response, length - 1); // -1 because we already read command byte
                *response_len = length - 1;
            }
            else
            {
                *response_len = 0;
            }

            // Read checksum and postamble
            uint8_t checksum;
            uint8_t postamble;
            this->read(&checksum, 1);
            this->read(&postamble, 1);

            // Verify checksum
            uint8_t calculated_checksum = PN532_PN532TOHOST + (command + 1);
            for (uint8_t i = 0; i < *response_len; i++)
            {
                calculated_checksum += response[i];
            }
            calculated_checksum = ~calculated_checksum + 1;

            if (checksum != calculated_checksum)
            {
                ESP_LOGE(TAG, "Invalid response checksum");
                return false;
            }

            return true;
        }

        bool AttracccessDesfireComponent::wait_ready_(uint16_t timeout)
        {
            uint16_t timer = 0;

            // If we're using an IRQ pin, wait for it to go low
            if (this->irq_pin_ != nullptr)
            {
                while (this->irq_pin_->digital_read())
                {
                    if (timer >= timeout)
                    {
                        return false;
                    }
                    delay(1);
                    timer++;
                }
                return true;
            }

            // Otherwise, poll the I2C status
            uint8_t status[1];
            while (timer < timeout)
            {
                // Read status byte
                this->read(status, 1);

                // Check if ready
                if (status[0] & 0x01)
                {
                    return true;
                }

                delay(10);
                timer += 10;
            }

            return false;
        }

        bool AttracccessDesfireComponent::transceive_data_(const uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len)
        {
            // Prepare InDataExchange command
            uint8_t data_len = 0;

            this->pn532_packetbuffer_[0] = PN532_COMMAND_INDATAEXCHANGE;
            this->pn532_packetbuffer_[1] = 1; // Card number (fixed to 1 for now)

            // Copy data to packet buffer
            for (uint8_t i = 0; i < tx_len; i++)
            {
                this->pn532_packetbuffer_[2 + i] = tx_data[i];
            }

            if (!this->send_command_(PN532_COMMAND_INDATAEXCHANGE, this->pn532_packetbuffer_, tx_len + 2))
            {
                ESP_LOGE(TAG, "Failed to send data to card");
                return false;
            }

            if (!this->read_response_(PN532_COMMAND_INDATAEXCHANGE, this->pn532_packetbuffer_, &data_len))
            {
                ESP_LOGE(TAG, "Failed to read response from card");
                return false;
            }

            // Check status code (0x00 = success)
            if (this->pn532_packetbuffer_[0] != 0x00)
            {
                ESP_LOGE(TAG, "Data exchange failed with status 0x%02X", this->pn532_packetbuffer_[0]);
                return false;
            }

            // Copy response data
            if (data_len > 1)
            {
                memcpy(rx_data, this->pn532_packetbuffer_ + 1, data_len - 1);
                *rx_len = data_len - 1;
            }
            else
            {
                *rx_len = 0;
            }

            return true;
        }

    } // namespace attraccess_desfire
} // namespace esphome