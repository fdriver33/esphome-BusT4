import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv

from .. import CONF_BUS_T4_ID, BusT4Component, bus_t4_ns
from ..cover import BusT4Cover

DEPENDENCIES = ["bus_t4"]

BusT4ConfigSwitch = bus_t4_ns.class_(
    "BusT4ConfigSwitch", switch.Switch, cg.Component
)

CONF_COVER_ID = "cover_id"
CONF_SETTING = "setting"

# Register map aligned with the working esphome-nice-bidiwifi implementation.
SETTINGS = {
    "auto_close": 0x80,
    "photo_close": 0x84,
    "always_close": 0x88,
    "standby": 0x8C,
    "peak": 0x93,
    "pre_flash": 0x94,
}

CONFIG_SCHEMA = (
    switch.switch_schema(BusT4ConfigSwitch)
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
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    bus = await cg.get_variable(config[CONF_BUS_T4_ID])
    cover = await cg.get_variable(config[CONF_COVER_ID])

    cg.add(var.set_parent(bus))
    cg.add(var.set_cover(cover))
    cg.add(var.set_parameter(SETTINGS[config[CONF_SETTING]]))
