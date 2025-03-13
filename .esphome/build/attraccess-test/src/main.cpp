// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
using std::isnan;
using std::min;
using std::max;
using namespace sensor;
using namespace text_sensor;
using namespace binary_sensor;
logger::Logger *logger_logger_id;
web_server_base::WebServerBase *web_server_base_webserverbase_id;
captive_portal::CaptivePortal *captive_portal_captiveportal_id;
wifi::WiFiComponent *wifi_wificomponent_id;
mdns::MDNSComponent *mdns_mdnscomponent_id;
esphome::ESPHomeOTAComponent *esphome_esphomeotacomponent_id;
safe_mode::SafeModeComponent *safe_mode_safemodecomponent_id;
preferences::IntervalSyncer *preferences_intervalsyncer_id;
attraccess_resource::APIResourceStatusComponent *attraccess_component;
attraccess_resource::APIResourceStatusSensor *resource_status;
attraccess_resource::APIResourceAvailabilitySensor *api_availability;
attraccess_resource::APIResourceInUseSensor *resource_in_use;
#define yield() esphome::yield()
#define millis() esphome::millis()
#define micros() esphome::micros()
#define delay(x) esphome::delay(x)
#define delayMicroseconds(x) esphome::delayMicroseconds(x)
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // async_tcp:
  //   {}
  // esphome:
  //   name: attraccess-test
  //   friendly_name: attraccess-test
  //   min_version: 2025.2.2
  //   build_path: build/attraccess-test
  //   area: ''
  //   platformio_options: {}
  //   includes: []
  //   libraries: []
  //   name_add_mac_suffix: false
  App.pre_setup("attraccess-test", "attraccess-test", "", "", __DATE__ ", " __TIME__, false);
  // sensor:
  // text_sensor:
  // binary_sensor:
  // logger:
  //   level: DEBUG
  //   id: logger_logger_id
  //   baud_rate: 115200
  //   tx_buffer_size: 512
  //   deassert_rts_dtr: false
  //   hardware_uart: UART0
  //   logs: {}
  logger_logger_id = new logger::Logger(115200, 512);
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_DEBUG);
  logger_logger_id->set_uart_selection(logger::UART_SELECTION_UART0);
  logger_logger_id->pre_setup();
  logger_logger_id->set_component_source("logger");
  App.register_component(logger_logger_id);
  // web_server_base:
  //   id: web_server_base_webserverbase_id
  web_server_base_webserverbase_id = new web_server_base::WebServerBase();
  web_server_base_webserverbase_id->set_component_source("web_server_base");
  App.register_component(web_server_base_webserverbase_id);
  // captive_portal:
  //   id: captive_portal_captiveportal_id
  //   web_server_base_id: web_server_base_webserverbase_id
  captive_portal_captiveportal_id = new captive_portal::CaptivePortal(web_server_base_webserverbase_id);
  captive_portal_captiveportal_id->set_component_source("captive_portal");
  App.register_component(captive_portal_captiveportal_id);
  // wifi:
  //   ap:
  //     ssid: Attraccess-Test Fallback Hotspot
  //     password: mT24lkkKvQ3c
  //     id: wifi_wifiap_id
  //     ap_timeout: 1min
  //   id: wifi_wificomponent_id
  //   domain: .local
  //   reboot_timeout: 15min
  //   power_save_mode: LIGHT
  //   fast_connect: false
  //   passive_scan: false
  //   enable_on_boot: true
  //   networks:
  //   - ssid: !secret 'wifi_ssid'
  //     password: !secret 'wifi_password'
  //     id: wifi_wifiap_id_2
  //     priority: 0.0
  //   use_address: attraccess-test.local
  wifi_wificomponent_id = new wifi::WiFiComponent();
  wifi_wificomponent_id->set_use_address("attraccess-test.local");
  {
  wifi::WiFiAP wifi_wifiap_id_2 = wifi::WiFiAP();
  wifi_wifiap_id_2.set_ssid("Darknet");
  wifi_wifiap_id_2.set_password("WirWollen16Kekse");
  wifi_wifiap_id_2.set_priority(0.0f);
  wifi_wificomponent_id->add_sta(wifi_wifiap_id_2);
  }
  {
  wifi::WiFiAP wifi_wifiap_id = wifi::WiFiAP();
  wifi_wifiap_id.set_ssid("Attraccess-Test Fallback Hotspot");
  wifi_wifiap_id.set_password("mT24lkkKvQ3c");
  wifi_wificomponent_id->set_ap(wifi_wifiap_id);
  }
  wifi_wificomponent_id->set_ap_timeout(60000);
  wifi_wificomponent_id->set_reboot_timeout(900000);
  wifi_wificomponent_id->set_power_save_mode(wifi::WIFI_POWER_SAVE_LIGHT);
  wifi_wificomponent_id->set_fast_connect(false);
  wifi_wificomponent_id->set_passive_scan(false);
  wifi_wificomponent_id->set_enable_on_boot(true);
  wifi_wificomponent_id->set_component_source("wifi");
  App.register_component(wifi_wificomponent_id);
  // mdns:
  //   id: mdns_mdnscomponent_id
  //   disabled: false
  //   services: []
  mdns_mdnscomponent_id = new mdns::MDNSComponent();
  mdns_mdnscomponent_id->set_component_source("mdns");
  App.register_component(mdns_mdnscomponent_id);
  // ota:
  // ota.esphome:
  //   platform: esphome
  //   password: c80ec2c641cf9cc7fb2ec53bbfec4485
  //   id: esphome_esphomeotacomponent_id
  //   version: 2
  //   port: 3232
  esphome_esphomeotacomponent_id = new esphome::ESPHomeOTAComponent();
  esphome_esphomeotacomponent_id->set_port(3232);
  esphome_esphomeotacomponent_id->set_auth_password("c80ec2c641cf9cc7fb2ec53bbfec4485");
  esphome_esphomeotacomponent_id->set_component_source("esphome.ota");
  App.register_component(esphome_esphomeotacomponent_id);
  // safe_mode:
  //   id: safe_mode_safemodecomponent_id
  //   boot_is_good_after: 1min
  //   disabled: false
  //   num_attempts: 10
  //   reboot_timeout: 5min
  safe_mode_safemodecomponent_id = new safe_mode::SafeModeComponent();
  safe_mode_safemodecomponent_id->set_component_source("safe_mode");
  App.register_component(safe_mode_safemodecomponent_id);
  if (safe_mode_safemodecomponent_id->should_enter_safe_mode(10, 300000, 60000)) return;
  // esp32:
  //   board: esp32dev
  //   framework:
  //     version: 2.0.5
  //     advanced:
  //       ignore_efuse_custom_mac: false
  //     source: ~3.20005.0
  //     platform_version: platformio/espressif32@5.4.0
  //     type: arduino
  //   flash_size: 4MB
  //   variant: ESP32
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  preferences_intervalsyncer_id = new preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  preferences_intervalsyncer_id->set_component_source("preferences");
  App.register_component(preferences_intervalsyncer_id);
  // external_components:
  //   - source:
  //       path: components
  //       type: local
  //     components:
  //     - attraccess_resource
  //     refresh: 1d
  // attraccess_resource:
  //   id: attraccess_component
  //   api_url: http:localhost:3000
  //   resource_id: '1'
  //   refresh_interval: 30s
  attraccess_component = new attraccess_resource::APIResourceStatusComponent();
  attraccess_component->set_component_source("attraccess_resource");
  App.register_component(attraccess_component);
  attraccess_component->set_api_url("http://localhost:3000");
  attraccess_component->set_resource_id("1");
  attraccess_component->set_refresh_interval(30000);
  // text_sensor.attraccess_resource:
  //   platform: attraccess_resource
  //   resource: attraccess_component
  //   name: Resource Status
  //   id: resource_status
  //   disabled_by_default: false
  resource_status = new attraccess_resource::APIResourceStatusSensor();
  App.register_text_sensor(resource_status);
  resource_status->set_name("Resource Status");
  resource_status->set_object_id("resource_status");
  resource_status->set_disabled_by_default(false);
  resource_status->set_component_source("attraccess_resource.text_sensor");
  App.register_component(resource_status);
  attraccess_component->set_status_text_sensor(resource_status);
  // binary_sensor.attraccess_resource:
  //   platform: attraccess_resource
  //   resource: attraccess_component
  //   availability:
  //     name: API Availability
  //     id: api_availability
  //     disabled_by_default: false
  //     device_class: connectivity
  //   in_use:
  //     name: Resource In Use
  //     id: resource_in_use
  //     disabled_by_default: false
  //     device_class: occupancy
  api_availability = new attraccess_resource::APIResourceAvailabilitySensor();
  App.register_binary_sensor(api_availability);
  api_availability->set_name("API Availability");
  api_availability->set_object_id("api_availability");
  api_availability->set_disabled_by_default(false);
  api_availability->set_device_class("connectivity");
  api_availability->set_component_source("attraccess_resource.binary_sensor");
  App.register_component(api_availability);
  attraccess_component->set_availability_sensor(api_availability);
  resource_in_use = new attraccess_resource::APIResourceInUseSensor();
  App.register_binary_sensor(resource_in_use);
  resource_in_use->set_name("Resource In Use");
  resource_in_use->set_object_id("resource_in_use");
  resource_in_use->set_disabled_by_default(false);
  resource_in_use->set_device_class("occupancy");
  resource_in_use->set_component_source("attraccess_resource.binary_sensor");
  App.register_component(resource_in_use);
  attraccess_component->set_in_use_sensor(resource_in_use);
  // md5:
  // socket:
  //   implementation: bsd_sockets
  // network:
  //   enable_ipv6: false
  //   min_ipv6_addr_count: 0
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
