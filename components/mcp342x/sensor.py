import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_NAME, CONF_UNIT_OF_MEASUREMENT, CONF_ACCURACY_DECIMALS, DEVICE_CLASS_VOLTAGE, STATE_CLASS_MEASUREMENT

from . import mcp342x_ns, MCP342xComponent

CONF_MCP342X_ID = "mcp342x_id"
CONF_CHANNEL = "channel"

MCP342xVoltageSensor = mcp342x_ns.class_("MCP342xVoltageSensor", sensor.Sensor)

CONFIG_SCHEMA = sensor.sensor_schema(
    MCP342xVoltageSensor,
    unit_of_measurement="V",
    accuracy_decimals=4,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.Required(CONF_MCP342X_ID): cv.use_id(MCP342xComponent),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=3),
    }
)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_MCP342X_ID])
    var = await sensor.new_sensor(config)
    cg.add(hub.register_channel_sensor(config[CONF_CHANNEL], var))
