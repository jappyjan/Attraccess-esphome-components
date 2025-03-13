#include "attraccess_resource.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include <ArduinoJson.h>
#include <WiFiClient.h>

namespace esphome
{
    namespace attraccess_resource
    {

        static const char *TAG = "attraccess_resource";
        static const uint32_t CONNECTION_TIMEOUT = 15000; // 15 seconds
        static const uint32_t KEEPALIVE_TIMEOUT = 45000;  // 45 seconds
        static const char *STATUS_IN_USE = "In Use";
        static const char *STATUS_AVAILABLE = "Available";

        void APIResourceStatusComponent::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up API Resource Status (SSE)...");

            this->client_ = new WiFiClient();

            // Initial connection
            this->connect_sse_();

            // Set initial availability state
            if (this->availability_sensor_ != nullptr)
            {
                this->availability_sensor_->publish_state(false); // Will be set to true when connected
            }
        }

        void APIResourceStatusComponent::loop()
        {
            // Check connection state
            this->check_connection_();

            if (!this->connected_)
            {
                // Try to reconnect if we haven't received data for a while
                const uint32_t now = millis();
                if (now - this->last_connect_attempt_ >= this->refresh_interval_)
                {
                    ESP_LOGW(TAG, "SSE connection lost or not established, reconnecting...");
                    this->connect_sse_();
                    this->last_connect_attempt_ = now;
                }
                return;
            }

            // Check for timeout (no data received for a while)
            if (millis() - this->last_data_received_ > KEEPALIVE_TIMEOUT)
            {
                ESP_LOGW(TAG, "SSE connection timed out, reconnecting...");
                this->disconnect_sse_();
                return;
            }

            // Process incoming data
            if (this->client_->available())
            {
                char c = this->client_->read();
                this->last_data_received_ = millis();

                if (c == '\n')
                {
                    // Process complete line
                    if (!this->buffer_.empty())
                    {
                        this->process_sse_line_(this->buffer_);
                        this->buffer_.clear();
                    }
                }
                else if (c != '\r')
                {
                    // Add to buffer (ignore carriage returns)
                    this->buffer_ += c;
                }
            }
        }

        void APIResourceStatusComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "API Resource Status (SSE):");
            ESP_LOGCONFIG(TAG, "  API URL: %s", this->api_url_.c_str());
            ESP_LOGCONFIG(TAG, "  Resource ID: %s", this->resource_id_.c_str());
            ESP_LOGCONFIG(TAG, "  Reconnect Interval: %u ms", this->refresh_interval_);
            ESP_LOGCONFIG(TAG, "  Monitoring: Device Usage Status (In Use/Available)");
            // Only log authentication if it's being used
            if (!this->username_.empty())
            {
                ESP_LOGCONFIG(TAG, "  Authentication: Enabled (not typically needed for public resources)");
            }
        }

        void APIResourceStatusComponent::connect_sse_()
        {
            if (this->client_ == nullptr)
            {
                this->client_ = new WiFiClient();
            }

            if (this->client_->connected())
            {
                this->client_->stop();
            }

            // Construct the URL with the resource ID
            std::string full_url = this->api_url_;

            // Check if the URL already contains "/api"
            if (full_url.find("/api") == std::string::npos)
            {
                // Append "/api" if it's not already there
                if (full_url.back() != '/')
                {
                    full_url += '/';
                }
                full_url += "api";
            }

            // Ensure the URL ends with a slash before appending the resource path
            if (full_url.back() != '/')
            {
                full_url += '/';
            }
            full_url += "resources/" + this->resource_id_ + "/events";

            // Parse URL
            ESP_LOGD(TAG, "Connecting to SSE endpoint: %s", full_url.c_str());

            // Extract host and path from URL
            // Example: http://example.com/api/resources/12345/events
            std::string protocol, host, path;

            // Find protocol separator
            size_t protocol_end = full_url.find("://");
            if (protocol_end != std::string::npos)
            {
                protocol = full_url.substr(0, protocol_end);
                size_t host_start = protocol_end + 3;

                // Find path start
                size_t path_start = full_url.find("/", host_start);
                if (path_start != std::string::npos)
                {
                    host = full_url.substr(host_start, path_start - host_start);
                    path = full_url.substr(path_start);
                }
                else
                {
                    host = full_url.substr(host_start);
                    path = "/";
                }
            }
            else
            {
                ESP_LOGE(TAG, "Invalid URL format: %s", full_url.c_str());
                return;
            }

            // Determine port based on protocol
            int port = 80;
            if (protocol == "https")
            {
                port = 443;
                ESP_LOGE(TAG, "HTTPS not supported for SSE connections. Please use HTTP.");
                return;
            }

            // Connect to server
            if (!this->client_->connect(host.c_str(), port))
            {
                ESP_LOGE(TAG, "Failed to connect to %s:%d", host.c_str(), port);
                if (this->availability_sensor_ != nullptr)
                {
                    this->availability_sensor_->publish_state(false);
                }
                return;
            }

            // Build HTTP request for SSE
            String request = "GET " + String(path.c_str()) + " HTTP/1.1\r\n" +
                             "Host: " + String(host.c_str()) + "\r\n" +
                             "Cache-Control: no-cache\r\n" +
                             "Accept: text/event-stream\r\n";

            // Add authentication only if provided (should be rare for public resources)
            if (!this->username_.empty() && !this->password_.empty())
            {
                ESP_LOGD(TAG, "Using basic authentication (unusual for public resources)");
                std::string auth_string = this->username_ + ":" + this->password_;
                // Placeholder for base64 encoding - in real component, use proper base64 encoding
                request += "Authorization: Basic " + String(auth_string.c_str()) + "\r\n";
            }

            request += "Connection: keep-alive\r\n" +
                       "\r\n";

            this->client_->print(request);
            this->last_connect_attempt_ = millis();
            this->last_data_received_ = millis(); // Reset timeout counter
            this->buffer_.clear();

            // Connection state will be updated after we receive a valid response
            ESP_LOGD(TAG, "SSE connection request sent");
        }

        void APIResourceStatusComponent::disconnect_sse_()
        {
            if (this->client_ != nullptr && this->client_->connected())
            {
                this->client_->stop();
            }
            this->connected_ = false;

            if (this->availability_sensor_ != nullptr)
            {
                this->availability_sensor_->publish_state(false);
            }

            ESP_LOGD(TAG, "SSE connection closed");
        }

        void APIResourceStatusComponent::check_connection_()
        {
            if (this->client_ == nullptr || !this->client_->connected())
            {
                if (this->connected_)
                {
                    ESP_LOGW(TAG, "SSE connection lost");
                    this->connected_ = false;

                    if (this->availability_sensor_ != nullptr)
                    {
                        this->availability_sensor_->publish_state(false);
                    }
                }
            }
        }

        void APIResourceStatusComponent::process_sse_line_(const std::string &line)
        {
            ESP_LOGV(TAG, "SSE line: %s", line.c_str());

            // Skip comments
            if (line.empty() || line[0] == ':')
            {
                return;
            }

            // Check for HTTP header to confirm connection
            if (!this->connected_ && line.find("HTTP/1.1") != std::string::npos)
            {
                if (line.find("200") != std::string::npos)
                {
                    ESP_LOGI(TAG, "SSE connection established");
                    // We'll set connected to true after we've processed all headers
                }
                else
                {
                    ESP_LOGE(TAG, "SSE connection failed: %s", line.c_str());
                }
                return;
            }

            // If we get an empty line after HTTP headers, mark as connected
            if (!this->connected_ && line.empty())
            {
                this->connected_ = true;
                if (this->availability_sensor_ != nullptr)
                {
                    this->availability_sensor_->publish_state(true);
                }
                return;
            }

            // Process SSE events
            if (line.find("event:") == 0)
            {
                // Event type - not used in this implementation
                return;
            }

            if (line.find("data:") == 0)
            {
                // Data payload
                std::string data = line.substr(5); // Remove "data:" prefix
                this->handle_api_response_(data);
                return;
            }
        }

        void APIResourceStatusComponent::handle_api_response_(const std::string &response)
        {
            // Parse JSON response
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, response);

            if (error)
            {
                ESP_LOGW(TAG, "JSON parsing failed: %s", error.c_str());
                return;
            }

            // Extract in_use status value from JSON (using camelCase inUse)
            if (!doc.containsKey("inUse"))
            {
                ESP_LOGW(TAG, "API response missing 'inUse' field");
                return;
            }

            bool in_use = doc["inUse"];
            this->last_in_use_ = in_use;

            // Check for event type to provide more detailed logging
            if (doc.containsKey("eventType"))
            {
                const char *event_type = doc["eventType"];
                if (strcmp(event_type, "resource.usage.started") == 0)
                {
                    ESP_LOGI(TAG, "Resource usage started event received");
                }
                else if (strcmp(event_type, "resource.usage.ended") == 0)
                {
                    ESP_LOGI(TAG, "Resource usage ended event received");
                }
                else
                {
                    ESP_LOGD(TAG, "Event type: %s", event_type);
                }
            }

            // Update in_use binary sensor
            if (this->in_use_sensor_ != nullptr)
            {
                this->in_use_sensor_->publish_state(in_use);
                ESP_LOGD(TAG, "Updated resource status: %s", in_use ? STATUS_IN_USE : STATUS_AVAILABLE);
            }

            // Update text sensor with human-readable status
            if (this->status_text_sensor_ != nullptr)
            {
                this->status_text_sensor_->publish_state(in_use ? STATUS_IN_USE : STATUS_AVAILABLE);
            }

            // Trigger callbacks
            for (auto &callback : this->callbacks_)
            {
                callback(in_use);
            }
        }

        void APIResourceStatusSensor::setup()
        {
            // No additional setup needed
        }

        void APIResourceInUseSensor::setup()
        {
            // No additional setup needed
        }

        void APIResourceAvailabilitySensor::setup()
        {
            // No additional setup needed
        }

    } // namespace attraccess_resource
} // namespace esphome