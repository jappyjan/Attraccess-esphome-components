import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import APIResourceStatusComponent, api_resource_ns

DEPENDENCIES = ["attraccess_resource"]

APIResourceStatusSensor = api_resource_ns.class_(
    "APIResourceStatusSensor", text_sensor.TextSensor, cg.Component
)

CONF_PARENT_ID = "resource"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    APIResourceStatusSensor
).extend({
    cv.Required(CONF_PARENT_ID): cv.use_id(APIResourceStatusComponent),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(parent.set_status_text_sensor(var)) 