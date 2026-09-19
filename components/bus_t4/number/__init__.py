import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv

from .. import CONF_BUS_T4_ID, BusT4Component, bus_t4_ns
from ..cover import BusT4Cover

DEPENDENCIES = ["bus_t4"]

BusT4ConfigNumber = bus_t4_ns.class_(
    "BusT4ConfigNumber", number.Number, cg.Component
)

CONF_COVER_ID = "cover_id"
CONF_SETTING = "setting"

SETTINGS = {
    "opening_speed": {"register": 0x42, "min": 1, "max": 100, "step": 1},
    "closing_speed": {"register": 0x43, "min": 1, "max": 100, "step": 1},
    "pause_time": {"register": 0x81, "min": 0, "max": 250, "step": 5},
    "photo_close_time": {"register": 0x85, "min": 0, "max": 250, "step": 1},
    "always_close_time": {"register": 0x89, "min": 0, "max": 250, "step": 1},
}

CONFIG_SCHEMA = (
    number.number_schema(BusT4ConfigNumber)
    .extend(
        {
            cv.GenerateID(CONF_BUS_T4_ID): cv.use_id(BusT4Component),
            cv.Required(CONF_COVER_ID): cv.use_id(BusT4Cover),
            cv.Required(CONF_SETTING): cv.one_of(*SETTINGS, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    setting = SETTINGS[config[CONF_SETTING]]
    var = await number.new_number(
        config,
        min_value=setting["min"],
        max_value=setting["max"],
        step=setting["step"],
    )
    await cg.register_component(var, config)

    bus = await cg.get_variable(config[CONF_BUS_T4_ID])
    cover = await cg.get_variable(config[CONF_COVER_ID])

    cg.add(var.set_parent(bus))
    cg.add(var.set_cover(cover))
    cg.add(var.set_parameter(setting["register"]))
