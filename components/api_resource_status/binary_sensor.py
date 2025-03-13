import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_OCCUPANCY

from . import APIResourceStatusComponent, api_resource_ns

DEPENDENCIES = ["api_resource_status"]

APIResourceAvailabilitySensor = api_resource_ns.class_(
    "APIResourceAvailabilitySensor", binary_sensor.BinarySensor, cg.Component
)

APIResourceInUseSensor = api_resource_ns.class_(
    "APIResourceInUseSensor", binary_sensor.BinarySensor, cg.Component
)

CONF_API_RESOURCE_STATUS = "api_resource_status"
CONF_AVAILABILITY = "availability"
CONF_IN_USE = "in_use"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_API_RESOURCE_STATUS): cv.use_id(APIResourceStatusComponent),
    cv.Optional(CONF_AVAILABILITY): binary_sensor.binary_sensor_schema(
        APIResourceAvailabilitySensor,
        device_class=DEVICE_CLASS_CONNECTIVITY,
    ),
    cv.Optional(CONF_IN_USE): binary_sensor.binary_sensor_schema(
        APIResourceInUseSensor,
        device_class=DEVICE_CLASS_OCCUPANCY,
    ),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_API_RESOURCE_STATUS])
    
    if CONF_AVAILABILITY in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_AVAILABILITY])
        await cg.register_component(sens, config[CONF_AVAILABILITY])
        cg.add(parent.set_availability_sensor(sens))
        
    if CONF_IN_USE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_IN_USE])
        await cg.register_component(sens, config[CONF_IN_USE])
        cg.add(parent.set_in_use_sensor(sens)) 