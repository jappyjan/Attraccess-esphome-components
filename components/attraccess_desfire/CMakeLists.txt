esphome_ns = esphome_ns.namespace("attraccess_desfire")

CODEOWNERS = ["@jappyjan"]

ESP_PLATFORMS = ["ESP32"]

DEPENDENCIES = ["attraccess_resource", "wifi", "http_request"]

AUTO_LOAD = ["attraccess_resource"]

SOURCES = [
    "attraccess_desfire.cpp",
    "pn532_interface.cpp",
    "api_protocol.cpp",
] 