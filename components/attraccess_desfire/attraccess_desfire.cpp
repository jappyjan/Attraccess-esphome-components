#include "attraccess_desfire.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/network/util.h"

namespace esphome
{
    namespace attraccess_desfire
    {

        static const char *const TAG = "attraccess_desfire";

        // LED indicator patterns
        static const int LED_SUCCESS_BLINKS = 3;
        static const int LED_ERROR_BLINKS = 5;
        static const int LED_SUCCESS_ON_TIME = 100;
        static const int LED_SUCCESS_OFF_TIME = 100;
        static const int LED_ERROR_ON_TIME = 50;
        static const int LED_ERROR_OFF_TIME = 50;

        // API communication timeouts
        static const uint32_t API_REQUEST_TIMEOUT = 5000;    // 5 seconds
        static const uint32_t API_RETRY_INTERVAL = 10000;    // 10 seconds
        static const uint32_t CARD_OPERATION_TIMEOUT = 2000; // 2 seconds

        // Main component methods

        void AttracccessDesfireComponent::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up Attraccess DESFire proxy...");

            // Set up pins
            if (this->led_pin_ != nullptr)
            {
                this->led_pin_->setup();
                this->led_pin_->digital_write(false); // LED initially off
            }

            if (this->reset_pin_ != nullptr)
            {
                this->reset_pin_->setup();
                this->reset_pin_->digital_write(true); // Active low reset, so we set it high
            }

            if (this->irq_pin_ != nullptr)
            {
                this->irq_pin_->setup();
            }

            // Set up button pin if available
            if (this->button_pin_ != nullptr)
            {
                this->button_pin_->setup();
                // TODO: Implement proper interrupt handling
                // For now, we'll poll the button in the loop() method
            }

            // Register resource status callback
            if (this->resource_ != nullptr)
            {
                // Build API endpoint from resource API URL
                std::string resource_api_url = this->resource_->get_api_url();
                // Remove trailing slash if present
                if (!resource_api_url.empty() && resource_api_url.back() == '/')
                {
                    resource_api_url.pop_back();
                }
                this->api_endpoint_ = resource_api_url + "/desfire/proxy";
                ESP_LOGI(TAG, "Using API endpoint: %s", this->api_endpoint_.c_str());

                // Get resource ID
                if (this->resource_id_.empty())
                {
                    this->resource_id_ = this->resource_->get_resource_id();
                }

                // Create default device ID if not set
                if (this->device_id_.empty())
                {
                    this->device_id_ = "resource-" + this->resource_id_ + "-desfire-reader";
                    ESP_LOGI(TAG, "Using default device ID: %s", this->device_id_.c_str());
                }

                // Register status callback
                this->resource_->register_status_callback([this](bool in_use)
                                                          {
      this->resource_status_ = in_use;
      ESP_LOGI(TAG, "Resource status changed to: %s", in_use ? "In Use" : "Available"); });
            }
            else
            {
                ESP_LOGE(TAG, "Resource component is required but not set!");
                this->mark_failed();
                return;
            }

            ESP_LOGI(TAG, "DESFire proxy setup complete");
        }

        void AttracccessDesfireComponent::loop()
        {
            // If the component is marked as failed, don't do anything
            if (this->is_failed())
                return;

            // Handle button press (if any)
            if (this->button_pressed_)
            {
                this->process_button_press_();
                this->button_pressed_ = false;
            }

            // State machine for card processing and API communication
            switch (this->state_)
            {
            case CARD_STATE_IDLE:
                // Nothing to do in idle state
                break;

            case CARD_STATE_WAIT_FOR_CARD:
                // Check for a card
                {
                    uint8_t success;
                    uint8_t data_length = 0;

                    this->pn532_packetbuffer_[0] = PN532_COMMAND_INLISTPASSIVETARGET;
                    this->pn532_packetbuffer_[1] = 1; // Maximum number of targets
                    this->pn532_packetbuffer_[2] = 0; // Baud rate (106 kbps type A)

                    if (!this->send_command_(PN532_COMMAND_INLISTPASSIVETARGET, this->pn532_packetbuffer_, 3))
                    {
                        ESP_LOGD(TAG, "Failed to send InListPassiveTarget command");
                        return;
                    }

                    if (!this->read_response_(PN532_COMMAND_INLISTPASSIVETARGET, this->pn532_packetbuffer_, &data_length))
                    {
                        ESP_LOGV(TAG, "No card detected");
                        return;
                    }

                    // Check if a card was found
                    if (this->pn532_packetbuffer_[0] != 1)
                    {
                        // No card found
                        return;
                    }

                    // Card found, extract UID
                    this->uid_length_ = this->pn532_packetbuffer_[5];
                    for (uint8_t i = 0; i < this->uid_length_; i++)
                    {
                        this->uid_buffer_[i] = this->pn532_packetbuffer_[6 + i];
                    }

                    // Format UID as string
                    this->card_uid_ = this->format_uid_(this->uid_buffer_, this->uid_length_);

                    ESP_LOGI(TAG, "Card detected - UID: %s", this->card_uid_.c_str());

                    // Trigger any card read callbacks
                    for (auto *trigger : this->card_read_triggers_)
                    {
                        trigger->trigger(this->card_uid_);
                    }

                    // Notify API about detected card
                    if (this->send_card_detected_(this->card_uid_))
                    {
                        // Update state
                        this->state_ = CARD_STATE_CARD_DETECTED;
                        this->last_card_read_ = millis();
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to notify API about card detection");
                        this->state_ = CARD_STATE_ERROR;
                    }
                }
                break;

            case CARD_STATE_CARD_DETECTED:
            case CARD_STATE_EXECUTING_COMMAND:
                // Check if we have API commands to process
                if (!this->command_queue_.empty() && !this->awaiting_api_response_)
                {
                    ApiCommand cmd = this->command_queue_.front();
                    this->command_queue_.pop();

                    switch (cmd.cmd_type)
                    {
                    case API_CMD_CARD_OPERATION:
                        // Process card operation
                        if (cmd.data.size() >= 2)
                        {
                            CardOpType op_type = static_cast<CardOpType>(cmd.data[0]);
                            uint8_t data_len = cmd.data[1];
                            std::vector<uint8_t> op_data(cmd.data.begin() + 2, cmd.data.begin() + 2 + data_len);

                            if (this->execute_card_operation_(op_type, op_data))
                            {
                                ESP_LOGI(TAG, "Card operation executed successfully");
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Card operation failed");
                                // TODO: Send error response to API
                            }
                        }
                        break;

                    case API_CMD_RESOURCE_ACTIVATE:
                        ESP_LOGI(TAG, "Resource activation command received");
                        // Turn on LED to indicate activation
                        if (this->led_pin_ != nullptr)
                        {
                            this->set_led_state_(true);
                        }
                        break;

                    case API_CMD_RESOURCE_DEACTIVATE:
                        ESP_LOGI(TAG, "Resource deactivation command received");
                        // Turn off LED to indicate deactivation
                        if (this->led_pin_ != nullptr)
                        {
                            this->set_led_state_(false);
                        }
                        break;

                    case API_CMD_AUTHENTICATION_RESULT:
                        if (!cmd.data.empty())
                        {
                            bool success = cmd.data[0] != 0;
                            this->handle_authentication_result_(success);
                        }
                        break;

                    default:
                        ESP_LOGW(TAG, "Unsupported command type: %d", cmd.cmd_type);
                        break;
                    }
                }

                // Check for timeout in card operations
                if ((this->state_ == CARD_STATE_CARD_DETECTED || this->state_ == CARD_STATE_EXECUTING_COMMAND) &&
                    millis() - this->last_card_read_ > this->timeout_)
                {
                    ESP_LOGW(TAG, "Card operation timeout");
                    this->reset_card_state_();
                }
                break;

            case CARD_STATE_AUTHORIZED:
                // Blink success pattern on LED
                if (this->led_pin_ != nullptr)
                {
                    this->blink_led_(LED_SUCCESS_BLINKS, LED_SUCCESS_ON_TIME, LED_SUCCESS_OFF_TIME);
                }

                // Reset for next operation
                this->reset_card_state_();
                break;

            case CARD_STATE_DENIED:
                // Blink error pattern on LED
                if (this->led_pin_ != nullptr)
                {
                    this->blink_led_(LED_ERROR_BLINKS, LED_ERROR_ON_TIME, LED_ERROR_OFF_TIME);
                }

                // Reset for next operation
                this->reset_card_state_();
                break;

            case CARD_STATE_ERROR:
                // Blink error pattern on LED
                if (this->led_pin_ != nullptr)
                {
                    this->blink_led_(LED_ERROR_BLINKS, LED_ERROR_ON_TIME, LED_ERROR_OFF_TIME);
                }

                // Reset for next operation
                this->reset_card_state_();
                break;

            default:
                // Unknown state - reset
                this->reset_card_state_();
                break;
            }

            // Handle API responses if we're waiting for any
            if (this->awaiting_api_response_)
            {
                if (millis() - this->last_api_request_ > API_REQUEST_TIMEOUT)
                {
                    ESP_LOGW(TAG, "API request timed out");
                    this->awaiting_api_response_ = false;

                    // If we were waiting for card, reset to idle
                    if (this->state_ == CARD_STATE_CARD_DETECTED || this->state_ == CARD_STATE_EXECUTING_COMMAND)
                    {
                        this->state_ = CARD_STATE_ERROR;
                    }
                }
                else
                {
                    // Try to handle API response
                    this->handle_api_response_();
                }
            }
        }

        void AttracccessDesfireComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Attraccess DESFire Proxy:");

            if (this->is_failed())
            {
                ESP_LOGCONFIG(TAG, "  FAILED to initialize NFC reader");
                return;
            }

            ESP_LOGCONFIG(TAG, "  Connected to resource: %s", this->resource_id_.c_str());
            ESP_LOGCONFIG(TAG, "  API Endpoint: %s", this->api_endpoint_.c_str());
            ESP_LOGCONFIG(TAG, "  Device ID: %s", this->device_id_.c_str());

            if (!this->api_key_.empty())
            {
                ESP_LOGCONFIG(TAG, "  API Key: [REDACTED]");
            }

            ESP_LOGCONFIG(TAG, "  Timeout: %dms", this->timeout_);

            LOG_PIN("  LED Pin: ", this->led_pin_);
            LOG_PIN("  Button Pin: ", this->button_pin_);
            LOG_PIN("  Reset Pin: ", this->reset_pin_);
            LOG_PIN("  IRQ Pin: ", this->irq_pin_);
        }

        void AttracccessDesfireComponent::process_button_press_()
        {
            // Record time of button press
            this->last_button_press_ = millis();
            ESP_LOGI(TAG, "Button pressed - notifying API");

            // Turn on LED to indicate button press
            if (this->led_pin_ != nullptr)
            {
                this->led_pin_->digital_write(true);
            }

            // Send button event to API
            if (this->send_button_event_())
            {
                // Enter wait-for-card state
                this->state_ = CARD_STATE_WAIT_FOR_CARD;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to send button event to API");

                // Blink error
                if (this->led_pin_ != nullptr)
                {
                    this->blink_led_(2, 100, 100);
                    this->led_pin_->digital_write(false);
                }
            }
        }

        void AttracccessDesfireComponent::set_led_state_(bool on)
        {
            if (this->led_pin_ != nullptr)
            {
                this->led_pin_->digital_write(on);
            }
        }

        void AttracccessDesfireComponent::blink_led_(int count, int on_time, int off_time)
        {
            for (int i = 0; i < count; i++)
            {
                this->set_led_state_(true);
                delay(on_time);
                this->set_led_state_(false);

                // Don't delay after the last blink
                if (i < count - 1)
                {
                    delay(off_time);
                }
            }
        }

        void AttracccessDesfireComponent::reset_card_state_()
        {
            this->state_ = CARD_STATE_IDLE;
            this->set_led_state_(false);
        }

        void AttracccessDesfireComponent::on_card_authorized_()
        {
            // Call any registered triggers
            for (auto *trigger : this->card_authorized_triggers_)
            {
                trigger->trigger();
            }
        }

        void AttracccessDesfireComponent::on_card_denied_()
        {
            // Call any registered triggers
            for (auto *trigger : this->card_denied_triggers_)
            {
                trigger->trigger();
            }
        }

        std::string AttracccessDesfireComponent::format_uid_(const uint8_t *uid, uint8_t length)
        {
            char uid_str[32] = {0};

            for (uint8_t i = 0; i < length; i++)
            {
                sprintf(&uid_str[i * 2], "%02X", uid[i]);
            }

            return std::string(uid_str);
        }

    } // namespace attraccess_desfire
} // namespace esphome
