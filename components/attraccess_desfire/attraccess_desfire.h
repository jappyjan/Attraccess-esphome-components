#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/attraccess_resource/attraccess_resource.h"
#include "esphome/core/automation.h"
#include "esphome/core/gpio.h"
#include "esphome/components/http_request/http_request.h"

#include "pn532_interface.h"
#include "api_protocol.h"

namespace esphome
{
    namespace attraccess_desfire
    {

        // Forward declarations
        class AttracccessDesfireComponent;
        class CardAuthorizedTrigger;
        class CardDeniedTrigger;
        class CardReadTrigger;

        // Trigger classes
        class CardAuthorizedTrigger : public Trigger<>
        {
        public:
            explicit CardAuthorizedTrigger(AttracccessDesfireComponent *parent) {}
        };

        class CardDeniedTrigger : public Trigger<>
        {
        public:
            explicit CardDeniedTrigger(AttracccessDesfireComponent *parent) {}
        };

        class CardReadTrigger : public Trigger<std::string>
        {
        public:
            explicit CardReadTrigger(AttracccessDesfireComponent *parent) {}
        };

        // Main component class
        class AttracccessDesfireComponent : public Component, public i2c::I2CDevice
        {
        public:
            AttracccessDesfireComponent() = default;

            void setup() override;
            void loop() override;
            void dump_config() override;
            float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

            // Setters
            void set_resource(attraccess_resource::APIResourceStatusComponent *resource) { resource_ = resource; }
            void set_resource_id(const std::string &resource_id) { resource_id_ = resource_id; }
            void set_led_pin(GPIOPin *led_pin) { led_pin_ = led_pin; }
            void set_button_pin(GPIOPin *button_pin) { button_pin_ = button_pin; }
            void set_reset_pin(GPIOPin *reset_pin) { reset_pin_ = reset_pin; }
            void set_irq_pin(GPIOPin *irq_pin) { irq_pin_ = irq_pin; }
            void set_device_id(const std::string &device_id) { device_id_ = device_id; }
            void set_api_key(const std::string &api_key) { api_key_ = api_key; }
            void set_timeout(uint32_t timeout) { timeout_ = timeout; }

            // Triggers
            void add_on_card_authorized_trigger(CardAuthorizedTrigger *trig) { this->card_authorized_triggers_.push_back(trig); }
            void add_on_card_denied_trigger(CardDeniedTrigger *trig) { this->card_denied_triggers_.push_back(trig); }
            void add_on_card_read_trigger(CardReadTrigger *trig) { this->card_read_triggers_.push_back(trig); }

        protected:
// Include helper methods from sub-modules
#include "pn532_interface_methods.h"
#include "api_protocol_methods.h"

            // Button handling
            static void IRAM_ATTR button_isr_(void *arg);
            void process_button_press_();

            // LED control
            void set_led_state_(bool on);
            void blink_led_(int count, int on_time = 100, int off_time = 100);

            // Helper functions
            void reset_card_state_();
            void on_card_authorized_();
            void on_card_denied_();
            std::string format_uid_(const uint8_t *uid, uint8_t length);

            // Component pins
            GPIOPin *led_pin_{nullptr};
            GPIOPin *button_pin_{nullptr};
            GPIOPin *reset_pin_{nullptr};
            GPIOPin *irq_pin_{nullptr};

            // Configuration fields
            attraccess_resource::APIResourceStatusComponent *resource_{nullptr};
            std::string resource_id_;
            std::string api_endpoint_; // Now built from resource API URL
            std::string device_id_;
            std::string api_key_;
            uint32_t timeout_{30000};

            // State variables
            CardState state_{CARD_STATE_IDLE};
            std::string card_uid_;
            volatile bool button_pressed_{false};
            uint32_t last_card_read_{0};
            uint32_t last_button_press_{0};
            uint32_t last_api_request_{0};
            bool resource_status_{false}; // false = available, true = in use
            bool awaiting_api_response_{false};
            uint16_t current_cmd_id_{0};

            // PN532 state
            uint8_t pn532_packetbuffer_[64];
            uint8_t command_buffer_[64];
            uint8_t uid_buffer_[10];
            uint8_t uid_length_{0};

            // API state
            std::queue<ApiCommand> command_queue_;

            // Callback storage
            std::vector<CardAuthorizedTrigger *> card_authorized_triggers_{};
            std::vector<CardDeniedTrigger *> card_denied_triggers_{};
            std::vector<CardReadTrigger *> card_read_triggers_{};
        };

    } // namespace attraccess_desfire
} // namespace esphome