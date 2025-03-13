import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_API_URL = "api_url"
CONF_RESOURCE_ID = "resource_id"
CONF_REFRESH_INTERVAL = "refresh_interval"
CONF_USERNAME = "username"
CONF_PASSWORD = "password"

# Define namespace for our component
api_resource_ns = cg.esphome_ns.namespace("api_resource_status")
APIResourceStatusComponent = api_resource_ns.class_("APIResourceStatusComponent", cg.Component)

# Config schema for the main component
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(APIResourceStatusComponent),
    cv.Required(CONF_API_URL): cv.string,
    cv.Required(CONF_RESOURCE_ID): cv.string,
    cv.Optional(CONF_REFRESH_INTERVAL, default="60s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_USERNAME): cv.string,
    cv.Optional(CONF_PASSWORD): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_api_url(config[CONF_API_URL]))
    cg.add(var.set_resource_id(config[CONF_RESOURCE_ID]))
    cg.add(var.set_refresh_interval(config[CONF_REFRESH_INTERVAL]))
    
    if CONF_USERNAME in config:
        cg.add(var.set_username(config[CONF_USERNAME]))
    
    if CONF_PASSWORD in config:
        cg.add(var.set_password(config[CONF_PASSWORD]))
    
    # Add dependencies
    cg.add_library("ArduinoJson", "6.18.5")
    # WiFiClient is built-in, no need for external library 