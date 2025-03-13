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

            // Set initial availability state to false until we successfully connect
            if (this->availability_sensor_ != nullptr)
            {
                // Start with false
                this->availability_sensor_->publish_state(false);
            }

            // Initialize the text sensor with the default "Available" state
            if (this->status_text_sensor_ != nullptr)
            {
                ESP_LOGI(TAG, "Initializing resource status text sensor to '%s'", STATUS_AVAILABLE);
                this->status_text_sensor_->publish_state(STATUS_AVAILABLE);
            }

            // Initial connection
            this->connect_sse_();

            // Don't publish state here as connect_sse_ already sets the appropriate state
            // and setting it here might override what connect_sse_ did
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

            // Ensure availability sensor matches the connected state
            // This helps if the sensor somehow got out of sync with the actual state
            if (this->availability_sensor_ != nullptr && !this->availability_sensor_->state)
            {
                ESP_LOGI(TAG, "Syncing API availability sensor with connected state");
                this->availability_sensor_->publish_state(true);
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
                // Read all available bytes to process them efficiently
                while (this->client_->available())
                {
                    char c = this->client_->read();
                    this->last_data_received_ = millis();

                    // Only log bytes at VERY verbose level - this was causing log spam
                    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE)
                    {
                        ESP_LOGVV(TAG, "Data byte: %c (%d)", c, (int)c);
                    }

                    if (c == '\n')
                    {
                        // Process complete line
                        if (!this->buffer_.empty())
                        {
                            // Only log complete lines at debug level or higher
                            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG)
                            {
                                ESP_LOGD(TAG, "Line: '%s'", this->buffer_.c_str());
                            }
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
        }

        void APIResourceStatusComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "API Resource Status (SSE):");
            ESP_LOGCONFIG(TAG, "  API URL: %s", this->api_url_.c_str());
            ESP_LOGCONFIG(TAG, "  Resource ID: %s", this->resource_id_.c_str());
            ESP_LOGCONFIG(TAG, "  Reconnect Interval: %u ms", this->refresh_interval_);
            ESP_LOGCONFIG(TAG, "  Monitoring: Device Usage Status (In Use/Available)");
            ESP_LOGCONFIG(TAG, "  Connection Status: %s", this->connected_ ? "Connected" : "Disconnected");
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

            // Ensure availability sensor shows disconnected state while connecting
            if (this->availability_sensor_ != nullptr && this->connected_)
            {
                this->connected_ = false;
                this->availability_sensor_->publish_state(false);
                ESP_LOGD(TAG, "Setting API availability to false before reconnecting");
            }

            // Test network connectivity only in verbose mode
            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
            {
                this->debug_network_connectivity_();
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
            int port = 80; // Default HTTP port

            // Find protocol separator
            size_t protocol_end = full_url.find("://");
            if (protocol_end != std::string::npos)
            {
                protocol = full_url.substr(0, protocol_end);
                size_t host_start = protocol_end + 3;

                // Find path start
                size_t path_start = full_url.find("/", host_start);

                std::string host_part;
                if (path_start != std::string::npos)
                {
                    host_part = full_url.substr(host_start, path_start - host_start);
                    path = full_url.substr(path_start);
                }
                else
                {
                    host_part = full_url.substr(host_start);
                    path = "/";
                }

                // Check if host includes port number (e.g., example.com:8080)
                size_t port_separator = host_part.find(":");
                if (port_separator != std::string::npos)
                {
                    host = host_part.substr(0, port_separator);
                    std::string port_str = host_part.substr(port_separator + 1);
                    port = atoi(port_str.c_str());
                    ESP_LOGD(TAG, "Custom port specified: %d", port);
                }
                else
                {
                    host = host_part;
                }
            }
            else
            {
                ESP_LOGE(TAG, "Invalid URL format: %s", full_url.c_str());
                return;
            }

            // Determine port based on protocol if not specified in URL
            if (port == 80 && protocol == "https")
            {
                port = 443;
            }

            if (protocol == "https")
            {
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

                // Update text sensor to show unknown state when connection fails
                if (this->status_text_sensor_ != nullptr)
                {
                    ESP_LOGD(TAG, "Setting resource status to 'Unknown' due to connection failure");
                    this->status_text_sensor_->publish_state("Unknown");
                }
                return;
            }

            // Build HTTP request for SSE
            String request = "GET " + String(path.c_str()) + " HTTP/1.1\r\n" +
                             "Host: " + String(host.c_str()) + (port != 80 ? ":" + String(port) : "") + "\r\n" +
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

            request += "Connection: keep-alive\r\n";
            request += "\r\n";

            // Only show full request headers in debug mode
            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG)
            {
                ESP_LOGD(TAG, "Sending HTTP request headers: \n%s", request.c_str());
            }
            else
            {
                ESP_LOGI(TAG, "Sending SSE connection request");
            }

            this->client_->print(request);
            this->last_connect_attempt_ = millis();
            this->last_data_received_ = millis(); // Reset timeout counter
            this->buffer_.clear();

            // Check for socket errors
            int socket_error = this->client_->getWriteError();
            if (socket_error)
            {
                ESP_LOGE(TAG, "Socket write error: %d", socket_error);
            }

            // Connection state will be updated after we receive a valid response
            ESP_LOGD(TAG, "SSE connection request sent");

            // Wait a short time for the initial HTTP response (status line)
            uint32_t start_time = millis();
            bool response_started = false;
            while (millis() - start_time < 5000)
            { // 5 second timeout
                if (this->client_->available())
                {
                    response_started = true;
                    ESP_LOGD(TAG, "Received initial HTTP response");

                    // Process HTTP headers immediately
                    ESP_LOGD(TAG, "Processing HTTP headers...");
                    std::string http_status_line;
                    bool status_ok = false;

                    // Read HTTP status line
                    while (this->client_->available())
                    {
                        char c = this->client_->read();
                        this->last_data_received_ = millis();

                        if (c == '\n')
                        {
                            if (!http_status_line.empty())
                            {
                                ESP_LOGI(TAG, "HTTP Status: %s", http_status_line.c_str());
                                if (http_status_line.find("HTTP/1.1 200") != std::string::npos)
                                {
                                    status_ok = true;
                                    ESP_LOGI(TAG, "SSE connection successful (HTTP 200 OK)");
                                }
                                else
                                {
                                    ESP_LOGW(TAG, "SSE connection failed with non-200 status");
                                }
                                break;
                            }
                        }
                        else if (c != '\r')
                        {
                            http_status_line += c;
                        }
                    }

                    if (status_ok)
                    {
                        // Process remaining headers until empty line
                        std::string header_line;
                        bool is_sse_content = false;

                        while (true)
                        {
                            // Wait for more data if needed
                            uint32_t header_start = millis();
                            while (!this->client_->available() && millis() - header_start < 1000)
                            {
                                delay(10);
                            }

                            if (!this->client_->available())
                            {
                                ESP_LOGW(TAG, "Timeout waiting for HTTP headers");
                                break;
                            }

                            char c = this->client_->read();
                            this->last_data_received_ = millis();

                            if (c == '\n')
                            {
                                if (header_line.empty())
                                {
                                    // Empty line marks end of headers
                                    ESP_LOGI(TAG, "Headers complete, SSE stream established");

                                    // First set our internal flag
                                    bool was_connected = this->connected_;
                                    this->connected_ = true;

                                    // Now update the sensor only if we weren't already connected
                                    // This prevents multiple state changes in quick succession
                                    if (this->availability_sensor_ != nullptr && !was_connected)
                                    {
                                        ESP_LOGI(TAG, "Updating API availability status to connected");
                                        this->availability_sensor_->publish_state(true);
                                    }
                                    break;
                                }
                                else
                                {
                                    // Check important headers
                                    ESP_LOGD(TAG, "Header: %s", header_line.c_str());
                                    if (header_line.find("Content-Type:") != std::string::npos &&
                                        header_line.find("text/event-stream") != std::string::npos)
                                    {
                                        is_sse_content = true;
                                        ESP_LOGI(TAG, "Confirmed SSE content type");
                                    }
                                    header_line.clear();
                                }
                            }
                            else if (c != '\r')
                            {
                                header_line += c;
                            }
                        }

                        if (!is_sse_content)
                        {
                            ESP_LOGW(TAG, "Content-Type is not text/event-stream, SSE might not work correctly");
                        }
                    }
                    break;
                }
                delay(10); // Small delay to prevent CPU hogging
            }

            if (!response_started)
            {
                ESP_LOGW(TAG, "No initial HTTP response received, but connection might still be valid for SSE");
                ESP_LOGD(TAG, "SSE connections may not return data until an event occurs");
                // We'll consider this potentially valid and wait for events
                // Don't mark as connected yet - we'll do that when we get headers and first event
            }
        }

        void APIResourceStatusComponent::disconnect_sse_()
        {
            // Close the physical connection if it exists
            if (this->client_ != nullptr && this->client_->connected())
            {
                this->client_->stop();
                ESP_LOGD(TAG, "Closed SSE connection socket");
            }

            // Only update internal state and sensor if we were previously connected
            bool was_connected = this->connected_;
            this->connected_ = false;

            if (was_connected && this->availability_sensor_ != nullptr)
            {
                ESP_LOGI(TAG, "Setting API availability to false due to explicit disconnect");
                this->availability_sensor_->publish_state(false);
            }

            ESP_LOGD(TAG, "SSE connection closed");
        }

        void APIResourceStatusComponent::check_connection_()
        {
            bool physically_connected = (this->client_ != nullptr && this->client_->connected());

            // If we think we're connected but the client isn't physically connected anymore
            if (this->connected_ && !physically_connected)
            {
                ESP_LOGW(TAG, "SSE connection lost (TCP disconnected)");
                this->connected_ = false;

                if (this->availability_sensor_ != nullptr)
                {
                    ESP_LOGI(TAG, "Setting API availability to false due to connection loss");
                    this->availability_sensor_->publish_state(false);
                }
            }

            // If the connection state and sensor state are out of sync, fix it
            if (this->availability_sensor_ != nullptr &&
                this->availability_sensor_->state != this->connected_)
            {
                ESP_LOGD(TAG, "Syncing API availability sensor with internal state: %s",
                         this->connected_ ? "connected" : "disconnected");
                this->availability_sensor_->publish_state(this->connected_);
            }
        }

        void APIResourceStatusComponent::debug_network_connectivity_()
        {
            // Try a simple ping to the target server to check network connectivity
            IPAddress remote_ip;
            ESP_LOGD(TAG, "Testing network connectivity to %s", this->api_url_.c_str());

            // Extract host from the URL
            std::string url = this->api_url_;
            size_t protocol_end = url.find("://");
            if (protocol_end != std::string::npos)
            {
                size_t host_start = protocol_end + 3;
                size_t host_end = url.find("/", host_start);
                if (host_end == std::string::npos)
                {
                    host_end = url.length();
                }

                std::string host_part = url.substr(host_start, host_end - host_start);
                size_t port_separator = host_part.find(":");
                std::string host;
                int port = 80;

                if (port_separator != std::string::npos)
                {
                    host = host_part.substr(0, port_separator);
                    port = atoi(host_part.substr(port_separator + 1).c_str());
                }
                else
                {
                    host = host_part;
                }

                ESP_LOGD(TAG, "Testing connectivity to host: %s", host.c_str());

                // Try TCP connection
                WiFiClient test_client;
                ESP_LOGD(TAG, "Testing TCP connection to %s:%d...", host.c_str(), port);

                if (test_client.connect(host.c_str(), port))
                {
                    ESP_LOGD(TAG, "TCP connection successful!");

                    // Try a basic HTTP request to test server responsiveness
                    ESP_LOGD(TAG, "Sending test HTTP request...");
                    test_client.println("GET /api/resources/1 HTTP/1.1");
                    test_client.println("Host: " + String(host.c_str()) + ":" + String(port));
                    test_client.println("Connection: close");
                    test_client.println();

                    // Wait up to 5 seconds for response
                    uint32_t start_time = millis();
                    bool response_received = false;
                    while (millis() - start_time < 5000)
                    {
                        if (test_client.available())
                        {
                            response_received = true;

                            // Only log response details in VERBOSE mode
                            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
                            {
                                ESP_LOGD(TAG, "Response received (first 5 lines):");
                                // Reduce from 10 to 5 lines to decrease verbosity
                                for (int i = 0; i < 5; i++)
                                {
                                    String line = test_client.readStringUntil('\n');
                                    if (line.length() > 0)
                                    {
                                        ESP_LOGD(TAG, "  %s", line.c_str());
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                ESP_LOGD(TAG, "Response received from server");
                                // Still need to read the response to clear it, but don't log
                                while (test_client.available())
                                {
                                    test_client.read();
                                }
                            }
                            break;
                        }
                        delay(10); // Small delay to not block CPU
                    }

                    if (!response_received)
                    {
                        ESP_LOGW(TAG, "No response received from server within timeout");
                    }

                    test_client.stop();

                    // Now test the SSE endpoint specifically - only in verbose mode
                    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
                    {
                        ESP_LOGD(TAG, "Testing SSE endpoint directly...");
                        WiFiClient sse_test_client;

                        if (sse_test_client.connect(host.c_str(), port))
                        {
                            ESP_LOGD(TAG, "SSE test connection successful, sending request...");
                            // Send an SSE request
                            sse_test_client.println("GET /api/resources/" + String(this->resource_id_.c_str()) + "/events HTTP/1.1");
                            sse_test_client.println("Host: " + String(host.c_str()) + ":" + String(port));
                            sse_test_client.println("Cache-Control: no-cache");
                            sse_test_client.println("Accept: text/event-stream");
                            sse_test_client.println("Connection: close"); // Use close for the test to ensure we get a response
                            sse_test_client.println();

                            // Wait up to 5 seconds for SSE response
                            start_time = millis();
                            response_received = false;
                            while (millis() - start_time < 5000)
                            {
                                if (sse_test_client.available())
                                {
                                    response_received = true;
                                    ESP_LOGD(TAG, "SSE test response received (first 5 lines):");
                                    // Reduce to 5 lines
                                    for (int i = 0; i < 5; i++)
                                    {
                                        String line = sse_test_client.readStringUntil('\n');
                                        if (line.length() > 0)
                                        {
                                            ESP_LOGD(TAG, "  SSE: %s", line.c_str());
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                    break;
                                }
                                delay(10); // Small delay to not block CPU
                            }

                            if (!response_received)
                            {
                                ESP_LOGD(TAG, "No SSE test response received within timeout - this is normal if no events are ready");
                            }

                            sse_test_client.stop();
                        }
                        else
                        {
                            ESP_LOGW(TAG, "SSE test connection failed");
                        }
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "TCP connection failed");
                }
            }
        }

        void APIResourceStatusComponent::process_sse_line_(const std::string &line)
        {
            // Only log at very verbose level to reduce spam
            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE)
            {
                ESP_LOGV(TAG, "SSE line: %s", line.c_str());
            }

            // Skip comments
            if (line.empty() || line[0] == ':')
            {
                ESP_LOGVV(TAG, "Skipping comment or empty line");
                return;
            }

            // Check for HTTP header to confirm connection
            if (!this->connected_ && line.find("HTTP/1.1") != std::string::npos)
            {
                if (line.find("200") != std::string::npos)
                {
                    ESP_LOGI(TAG, "SSE connection established with HTTP 200 OK");
                    // We'll set connected to true after we've processed all headers
                }
                else if (line.find("401") != std::string::npos)
                {
                    ESP_LOGE(TAG, "SSE connection failed with HTTP 401 Unauthorized: %s", line.c_str());
                    ESP_LOGE(TAG, "Authentication credentials may be required for this endpoint");
                }
                else if (line.find("403") != std::string::npos)
                {
                    ESP_LOGE(TAG, "SSE connection failed with HTTP 403 Forbidden: %s", line.c_str());
                    ESP_LOGE(TAG, "Your credentials don't have permission to access this SSE endpoint");
                }
                else if (line.find("404") != std::string::npos)
                {
                    ESP_LOGE(TAG, "SSE connection failed with HTTP 404 Not Found: %s", line.c_str());
                    ESP_LOGE(TAG, "Check your resource_id and api_url configuration");
                }
                else if (line.find("5") != std::string::npos) // 500, 501, 503, etc.
                {
                    ESP_LOGE(TAG, "SSE connection failed with server error: %s", line.c_str());
                    ESP_LOGE(TAG, "The server had an error processing the request");
                }
                else
                {
                    ESP_LOGE(TAG, "SSE connection failed with non-200 status: %s", line.c_str());
                }
                return;
            }

            // Process HTTP headers - look for relevant headers
            if (!this->connected_ && line.find(":") != std::string::npos)
            {
                // Look for Content-Type to verify this is an SSE response
                if (line.find("Content-Type:") != std::string::npos &&
                    line.find("text/event-stream") != std::string::npos)
                {
                    ESP_LOGI(TAG, "Confirmed SSE content type");
                }

                // Add other header processing if needed
                return;
            }

            // If we get an empty line after HTTP headers, mark as connected
            if (!this->connected_ && line.empty())
            {
                ESP_LOGI(TAG, "HTTP headers complete, marking connection as established");
                this->connected_ = true;
                if (this->availability_sensor_ != nullptr)
                {
                    this->availability_sensor_->publish_state(true);
                }
                return;
            }

            // Process SSE protocol messages
            if (this->connected_)
            {
                // Handle SSE id lines (comes before the data line)
                if (line.find("id:") == 0)
                {
                    std::string id_value = line.substr(3);
                    // Trim leading/trailing whitespace
                    id_value.erase(0, id_value.find_first_not_of(" \t"));
                    id_value.erase(id_value.find_last_not_of(" \t") + 1);

                    // Move to debug level
                    ESP_LOGD(TAG, "Received SSE event ID: %s", id_value.c_str());
                    return;
                }

                // Handle SSE event type lines
                if (line.find("event:") == 0)
                {
                    ESP_LOGD(TAG, "Received event type indicator: %s", line.c_str());
                    return;
                }

                // Handle SSE data lines - this is where our JSON payloads come in
                if (line.find("data:") == 0)
                {
                    // Standard SSE data payload
                    std::string data = line.substr(5); // Remove "data:" prefix
                    // Trim leading/trailing whitespace
                    data.erase(0, data.find_first_not_of(" \t"));

                    ESP_LOGD(TAG, "Received SSE data: %s", data.c_str());

                    // Handle keepalive messages specially - don't try to parse as regular data
                    if (data.find("{\"keepalive\":true}") != std::string::npos)
                    {
                        // Downgrade keepalive messages to debug level to reduce spam
                        ESP_LOGD(TAG, "Received keepalive message, connection is healthy");
                        // Don't do anything else with keepalive messages
                        return;
                    }

                    // Process normal data message
                    this->handle_api_response_(data);
                    return;
                }
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
                ESP_LOGW(TAG, "Failed JSON: %s", response.c_str());
                return;
            }

            // Print the full JSON for debugging - only at verbose level
            if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
            {
                String json_str;
                serializeJson(doc, json_str);
                ESP_LOGD(TAG, "Parsed JSON: %s", json_str.c_str());
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
            else
            {
                ESP_LOGD(TAG, "Status update event received");
            }

            // Update in_use binary sensor
            if (this->in_use_sensor_ != nullptr)
            {
                this->in_use_sensor_->publish_state(in_use);
                ESP_LOGD(TAG, "Updated resource status: %s", in_use ? STATUS_IN_USE : STATUS_AVAILABLE);
            }

            // Update text sensor with human-readable status
            // Always publish the text sensor state, even if it hasn't changed
            const char *status_text = in_use ? STATUS_IN_USE : STATUS_AVAILABLE;
            if (this->status_text_sensor_ != nullptr)
            {
                ESP_LOGD(TAG, "Setting resource status text sensor to '%s'", status_text);
                this->status_text_sensor_->publish_state(status_text);
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