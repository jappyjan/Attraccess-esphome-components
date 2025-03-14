import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import i2c, binary_sensor, text_sensor
import esphome.components.attraccess_resource as attraccess_resource
import esphome.components.http_request as http_request
from esphome.const import (
    CONF_ID,
    CONF_TIMEOUT,
    CONF_TRIGGER_ID,
    CONF_URL,
)
from esphome.core import CORE

# Define CONF_RESOURCE_ID locally if not in esphome.const
CONF_RESOURCE_ID = "resource_id"

DEPENDENCIES = ["i2c", "attraccess_resource", "http_request"]
AUTO_LOAD = ["attraccess_resource", "http_request"]

CONF_RESOURCE = "resource"
CONF_LED_PIN = "led_pin"
CONF_BUTTON_PIN = "button_pin"
CONF_RESET_PIN = "reset_pin"
CONF_IRQ_PIN = "irq_pin"
CONF_DEVICE_ID = "device_id"
CONF_API_KEY = "api_key"
CONF_ON_CARD_AUTHORIZED = "on_card_authorized"
CONF_ON_CARD_DENIED = "on_card_denied"
CONF_ON_CARD_READ = "on_card_read"

attraccess_desfire_ns = cg.esphome_ns.namespace("attraccess_desfire")
AttracccessDesfireComponent = attraccess_desfire_ns.class_(
    "AttracccessDesfireComponent", cg.Component, i2c.I2CDevice
)

# Triggers
CardAuthorizedTrigger = attraccess_desfire_ns.class_(
    "CardAuthorizedTrigger", automation.Trigger.template()
)
CardDeniedTrigger = attraccess_desfire_ns.class_(
    "CardDeniedTrigger", automation.Trigger.template()
)
CardReadTrigger = attraccess_desfire_ns.class_(
    "CardReadTrigger", automation.Trigger.template(cg.std_string)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AttracccessDesfireComponent),
        cv.Required(CONF_RESOURCE): cv.use_id(
            attraccess_resource.APIResourceStatusComponent
        ),
        cv.Optional(CONF_RESOURCE_ID): cv.string,
        cv.Optional(CONF_LED_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_BUTTON_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_DEVICE_ID): cv.string,
        cv.Optional(CONF_API_KEY): cv.string,
        cv.Optional(CONF_TIMEOUT, default=30): cv.positive_time_period_seconds,
        cv.Optional(CONF_ON_CARD_AUTHORIZED): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CardAuthorizedTrigger),
            }
        ),
        cv.Optional(CONF_ON_CARD_DENIED): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CardDeniedTrigger),
            }
        ),
        cv.Optional(CONF_ON_CARD_READ): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CardReadTrigger),
            }
        ),
    }
).extend(i2c.i2c_device_schema(0x24))  # Default I2C address for PN532


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Resource is now required
    resource = await cg.get_variable(config[CONF_RESOURCE])
    cg.add(var.set_resource(resource))

    if CONF_RESOURCE_ID in config:
        cg.add(var.set_resource_id(config[CONF_RESOURCE_ID]))

    if CONF_LED_PIN in config:
        led_pin = await cg.gpio_pin_expression(config[CONF_LED_PIN])
        cg.add(var.set_led_pin(led_pin))

    if CONF_BUTTON_PIN in config:
        button_pin = await cg.gpio_pin_expression(config[CONF_BUTTON_PIN])
        cg.add(var.set_button_pin(button_pin))

    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset_pin))

    if CONF_IRQ_PIN in config:
        irq_pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
        cg.add(var.set_irq_pin(irq_pin))
    
    if CONF_DEVICE_ID in config:
        cg.add(var.set_device_id(config[CONF_DEVICE_ID]))
    
    if CONF_API_KEY in config:
        cg.add(var.set_api_key(config[CONF_API_KEY]))

    cg.add(var.set_timeout(config[CONF_TIMEOUT]))

    # Register triggers
    for conf in config.get(CONF_ON_CARD_AUTHORIZED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_CARD_DENIED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_CARD_READ, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "uid")], conf)

    cg.add_library("WiFi", None) 
    cg.add_library("HTTPClient", None) 