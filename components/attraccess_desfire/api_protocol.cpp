#include "attraccess_desfire.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"
#include <HTTPClient.h>

namespace esphome
{
    namespace attraccess_desfire
    {

        static const char *const TAG = "attraccess_desfire.api";

        bool AttracccessDesfireComponent::send_api_request_(ApiCommandType cmd_type, const std::vector<uint8_t> &data)
        {
            if (this->api_endpoint_.empty())
            {
                ESP_LOGE(TAG, "API endpoint not configured");
                return false;
            }

            // Ensure we have network connectivity
            if (!network::is_connected())
            {
                ESP_LOGE(TAG, "Network is not connected");
                return false;
            }

            // Build binary packet
            uint16_t cmd_id = this->get_next_cmd_id_();
            std::vector<uint8_t> packet = this->build_binary_packet_(cmd_type, cmd_id, data);

            // Send HTTP request
            ESP_LOGI(TAG, "Sending API request to %s (cmd_type: %d, cmd_id: %d, data_len: %d)",
                     this->api_endpoint_.c_str(), cmd_type, cmd_id, data.size());

            HTTPClient http;
            http.begin(String(this->api_endpoint_.c_str()));
            http.addHeader("Content-Type", "application/octet-stream");

            if (!this->device_id_.empty())
            {
                http.addHeader("X-Device-ID", String(this->device_id_.c_str()));
            }

            if (!this->api_key_.empty())
            {
                http.addHeader("X-API-Key", String(this->api_key_.c_str()));
            }

            int http_code = http.POST(packet.data(), packet.size());

            if (http_code > 0)
            {
                ESP_LOGI(TAG, "API response code: %d", http_code);

                if (http_code == HTTP_CODE_OK)
                {
                    // Mark that we're awaiting an API response
                    this->awaiting_api_response_ = true;
                    this->last_api_request_ = millis();

                    // Parse API response if available
                    String payload = http.getString();
                    if (payload.length() > 0)
                    {
                        ESP_LOGD(TAG, "API response received with %d bytes", payload.length());

                        // Convert String to vector<uint8_t>
                        std::vector<uint8_t> response_bytes;
                        response_bytes.reserve(payload.length());
                        for (size_t i = 0; i < payload.length(); i++)
                        {
                            response_bytes.push_back(payload[i]);
                        }

                        // Process response data if needed
                        ApiCommand cmd;
                        if (this->parse_binary_packet_(response_bytes, cmd))
                        {
                            this->command_queue_.push(cmd);
                        }
                    }

                    http.end();
                    return true;
                }
                else
                {
                    ESP_LOGE(TAG, "API request failed with code: %d", http_code);
                }
            }
            else
            {
                ESP_LOGE(TAG, "API request failed: %s", http.errorToString(http_code).c_str());
            }

            http.end();
            return false;
        }

        bool AttracccessDesfireComponent::handle_api_response_()
        {
            // This would normally check for received responses via some communication channel
            // For simplicity, we'll just clear the flag if we've processed all commands
            if (this->command_queue_.empty())
            {
                this->awaiting_api_response_ = false;
            }
            return true;
        }

        bool AttracccessDesfireComponent::send_button_event_()
        {
            // Create button event data
            std::vector<uint8_t> data;

            // Include resource ID if available
            if (!this->resource_id_.empty())
            {
                data.insert(data.end(), this->resource_id_.begin(), this->resource_id_.end());
            }

            return this->send_api_request_(API_CMD_BUTTON_EVENT, data);
        }

        bool AttracccessDesfireComponent::send_card_detected_(const std::string &uid)
        {
            // Create card detection data
            std::vector<uint8_t> data;

            // Add UID
            data.insert(data.end(), uid.begin(), uid.end());

            return this->send_api_request_(API_CMD_CARD_DETECTED, data);
        }

        bool AttracccessDesfireComponent::execute_card_operation_(CardOpType op_type, const std::vector<uint8_t> &data)
        {
            uint8_t rx_data[64];
            uint8_t rx_len = 0;

            // Execute the appropriate operation
            switch (op_type)
            {
            case CARD_OP_TRANSCEIVE:
                // Direct transceive to card
                if (!this->transceive_data_(data.data(), data.size(), rx_data, &rx_len))
                {
                    ESP_LOGE(TAG, "Card transceive failed");
                    return false;
                }

                // Send response back to API
                {
                    std::vector<uint8_t> response_data(rx_data, rx_data + rx_len);
                    if (!this->send_api_request_(API_CMD_CARD_OPERATION, response_data))
                    {
                        ESP_LOGE(TAG, "Failed to send card response to API");
                        return false;
                    }
                }
                break;

            default:
                ESP_LOGE(TAG, "Unsupported card operation type: %d", op_type);
                return false;
            }

            return true;
        }

        void AttracccessDesfireComponent::handle_authentication_result_(bool success)
        {
            if (success)
            {
                ESP_LOGI(TAG, "Authentication successful");
                this->state_ = CARD_STATE_AUTHORIZED;
                this->on_card_authorized_();
            }
            else
            {
                ESP_LOGI(TAG, "Authentication failed");
                this->state_ = CARD_STATE_DENIED;
                this->on_card_denied_();
            }
        }

        std::vector<uint8_t> AttracccessDesfireComponent::build_binary_packet_(ApiCommandType cmd_type, uint16_t cmd_id, const std::vector<uint8_t> &data)
        {
            std::vector<uint8_t> packet;

            // Magic (2 bytes)
            packet.push_back(BINARY_PACKET_MAGIC >> 8);
            packet.push_back(BINARY_PACKET_MAGIC & 0xFF);

            // Version (1 byte)
            packet.push_back(BINARY_PACKET_VERSION);

            // Command ID (2 bytes)
            packet.push_back(cmd_id >> 8);
            packet.push_back(cmd_id & 0xFF);

            // Command Type (1 byte)
            packet.push_back(cmd_type);

            // Data length (2 bytes)
            packet.push_back(data.size() >> 8);
            packet.push_back(data.size() & 0xFF);

            // Data
            packet.insert(packet.end(), data.begin(), data.end());

            // Nonce (4 bytes) - use milliseconds as crude nonce
            uint32_t nonce = millis();
            packet.push_back((nonce >> 24) & 0xFF);
            packet.push_back((nonce >> 16) & 0xFF);
            packet.push_back((nonce >> 8) & 0xFF);
            packet.push_back(nonce & 0xFF);

            // TODO: Add HMAC if needed

            return packet;
        }

        bool AttracccessDesfireComponent::parse_binary_packet_(const std::vector<uint8_t> &packet, ApiCommand &command)
        {
            // Minimum packet size: magic(2) + version(1) + cmd_id(2) + cmd_type(1) + data_len(2) + nonce(4) = 12 bytes
            if (packet.size() < 12)
            {
                ESP_LOGE(TAG, "Packet too small: %d bytes", packet.size());
                return false;
            }

            // Check magic
            uint16_t magic = (packet[0] << 8) | packet[1];
            if (magic != BINARY_PACKET_MAGIC)
            {
                ESP_LOGE(TAG, "Invalid packet magic: %04X", magic);
                return false;
            }

            // Check version
            if (packet[2] != BINARY_PACKET_VERSION)
            {
                ESP_LOGE(TAG, "Unsupported packet version: %d", packet[2]);
                return false;
            }

            // Extract command ID
            command.cmd_id = (packet[3] << 8) | packet[4];

            // Extract command type
            command.cmd_type = packet[5];

            // Extract data length
            uint16_t data_len = (packet[6] << 8) | packet[7];

            // Check if packet is large enough to contain the claimed data length
            if (packet.size() < 12 + data_len)
            {
                ESP_LOGE(TAG, "Packet too small for claimed data length");
                return false;
            }

            // Extract data
            command.data.assign(packet.begin() + 8, packet.begin() + 8 + data_len);

            // TODO: Verify HMAC if needed

            return true;
        }

        uint16_t AttracccessDesfireComponent::get_next_cmd_id_()
        {
            return ++this->current_cmd_id_;
        }

    } // namespace attraccess_desfire
} // namespace esphome