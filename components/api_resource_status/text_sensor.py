import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import APIResourceStatusComponent, api_resource_ns

DEPENDENCIES = ["api_resource_status"]

APIResourceStatusSensor = api_resource_ns.class_(
    "APIResourceStatusSensor", text_sensor.TextSensor, cg.Component
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    APIResourceStatusSensor
).extend({
    cv.GenerateID(CONF_ID): cv.use_id(APIResourceStatusComponent),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_status_text_sensor(var)) 