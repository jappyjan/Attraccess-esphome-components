#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/helpers.h"
#include <WiFiClient.h>
#include <string>
#include <queue>

namespace esphome
{
    namespace attraccess_resource
    {

        class APIResourceStatusSensor;
        class APIResourceAvailabilitySensor;
        class APIResourceInUseSensor;

        // Callback type for resource status change notifications
        using ResourceStatusCallback = std::function<void(bool)>;

        class APIResourceStatusComponent : public Component
        {
        public:
            APIResourceStatusComponent() {}

            void setup() override;
            void loop() override;
            void dump_config() override;
            float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

            void set_api_url(const std::string &api_url) { this->api_url_ = api_url; }
            void set_resource_id(const std::string &resource_id) { this->resource_id_ = resource_id; }
            void set_refresh_interval(uint32_t refresh_interval) { this->refresh_interval_ = refresh_interval; }
            void set_username(const std::string &username) { this->username_ = username; }
            void set_password(const std::string &password) { this->password_ = password; }

            void set_status_text_sensor(text_sensor::TextSensor *status_text_sensor) { this->status_text_sensor_ = status_text_sensor; }
            void set_in_use_sensor(binary_sensor::BinarySensor *in_use_sensor) { this->in_use_sensor_ = in_use_sensor; }
            void set_availability_sensor(binary_sensor::BinarySensor *availability_sensor)
            {
                this->availability_sensor_ = availability_sensor;
            }

            void register_status_callback(ResourceStatusCallback callback) { this->callbacks_.push_back(callback); }

            // Add getters
            std::string get_api_url() const { return this->api_url_; }
            std::string get_resource_id() const { return this->resource_id_; }

            // Add status check methods
            bool is_api_available() const { return this->connected_; }
            bool is_active() const { return !this->last_in_use_; }

        protected:
            void connect_sse_();
            void disconnect_sse_();
            void process_sse_line_(const std::string &line);
            void handle_api_response_(const std::string &response);
            void check_connection_();
            void debug_network_connectivity_();

            std::string api_url_;
            std::string resource_id_;
            uint32_t refresh_interval_; // Used as a keepalive/reconnect interval
            std::string username_;
            std::string password_;

            text_sensor::TextSensor *status_text_sensor_{nullptr};
            binary_sensor::BinarySensor *in_use_sensor_{nullptr};
            binary_sensor::BinarySensor *availability_sensor_{nullptr};

            uint32_t last_connect_attempt_{0};
            uint32_t last_data_received_{0};
            bool last_in_use_{false};
            bool connected_{false};

            // SSE client data
            WiFiClient *client_{nullptr};
            std::string buffer_;

            // Callbacks for status changes
            std::vector<ResourceStatusCallback> callbacks_{};
        };

        class APIResourceStatusSensor : public text_sensor::TextSensor, public Component
        {
        public:
            void set_parent(APIResourceStatusComponent *parent) { this->parent_ = parent; }
            void setup() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

        protected:
            APIResourceStatusComponent *parent_;
        };

        class APIResourceInUseSensor : public binary_sensor::BinarySensor, public Component
        {
        public:
            void set_parent(APIResourceStatusComponent *parent) { this->parent_ = parent; }
            void setup() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

        protected:
            APIResourceStatusComponent *parent_;
        };

        class APIResourceAvailabilitySensor : public binary_sensor::BinarySensor, public Component
        {
        public:
            void set_parent(APIResourceStatusComponent *parent) { this->parent_ = parent; }
            void setup() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

        protected:
            APIResourceStatusComponent *parent_;
        };

    } // namespace attraccess_resource
} // namespace esphome