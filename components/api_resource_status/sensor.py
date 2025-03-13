import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ICON_INFORMATION_OUTLINE

from . import APIResourceStatusComponent, api_resource_ns, CONF_API_URL

DEPENDENCIES = ["api_resource_status"]

APIResourceStatusSensor = api_resource_ns.class_("APIResourceStatusSensor", text_sensor.TextSensor, cg.Component)

CONF_RESOURCE_STATUS = "resource_status"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    APIResourceStatusSensor,
    icon=ICON_INFORMATION_OUTLINE,
).extend({
    cv.GenerateID(CONF_ID): cv.use_id(APIResourceStatusComponent),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(parent.set_status_text_sensor(var)) 