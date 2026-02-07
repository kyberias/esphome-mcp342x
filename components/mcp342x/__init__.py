import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_ADDRESS, CONF_UPDATE_INTERVAL

CODEOWNERS = ["@kyberias"]
DEPENDENCIES = ["i2c"]

mcp342x_ns = cg.esphome_ns.namespace("mcp342x")
MCP342xComponent = mcp342x_ns.class_("MCP342xComponent", cg.PollingComponent, i2c.I2CDevice)

CONF_GAIN = "gain"
CONF_RESOLUTION = "resolution"
CONF_VREF = "vref"

# gain: 1/2/4/8
GAIN_MAP = {
    1: 0,
    2: 1,
    4: 2,
    8: 3,
}

RES_MAP = {
    12: 0,
    14: 1,
    16: 2,
    18: 3,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(MCP342xComponent),
            cv.Optional(CONF_ADDRESS, default=0x68): cv.i2c_address,
            cv.Optional(CONF_GAIN, default=1): cv.one_of(*GAIN_MAP.keys(), int=True),
            cv.Optional(CONF_RESOLUTION, default=16): cv.one_of(*RES_MAP.keys(), int=True),
            cv.Optional(CONF_VREF, default=2.048): cv.float_,
            cv.Optional(CONF_UPDATE_INTERVAL, default="200ms"): cv.update_interval,
        }
    )
    .extend(cv.polling_component_schema("200ms"))
    .extend(i2c.i2c_device_schema(0x68))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_gain_code(GAIN_MAP[config[CONF_GAIN]]))
    cg.add(var.set_resolution_code(RES_MAP[config[CONF_RESOLUTION]]))
    cg.add(var.set_vref(config[CONF_VREF]))
