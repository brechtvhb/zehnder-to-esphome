""" Creates module comfod """
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import binary_sensor, sensor, uart, climate
from esphome.const import (DEVICE_CLASS_EMPTY, DEVICE_CLASS_TEMPERATURE,
                           DEVICE_CLASS_VOLUME, STATE_CLASS_MEASUREMENT,
                           UNIT_CELSIUS, UNIT_CUBIC_METER, UNIT_PERCENT,
                           UNIT_REVOLUTIONS_PER_MINUTE)

comfod_ns = cg.esphome_ns.namespace('comfod')
ComfoDComponent = comfod_ns.class_('ComfoDComponent', climate.Climate, cg.PollingComponent)

DEPENDENCIES=['uart']
AUTO_LOAD = ['sensor', 'climate', 'binary_sensor']
REQUIRED_KEY_UART_COMFOD = "uart_comfod_id"
OPTIONAL_KEY_UART_COMFOSENSE = "uart_comfosense_id"

CONF_FAN_SUPPLY_AIR_PERCENTAGE = "fan_supply_air_percentage"
CONF_FAN_EXHAUST_AIR_PERCENTAGE = "fan_exhaust_air_percentage"
CONF_FAN_SPEED_SUPPLY = "fan_speed_supply"
CONF_FAN_SPEED_EXHAUST = "fan_speed_exhaust"
CONF_IS_BYPASS_VALVE_OPEN = "is_bypass_valve_open"
CONF_IS_PREHEATING = "is_preheating"
CONF_OUTSIDE_AIR_TEMPERATURE = "outside_air_temperature"
CONF_SUPPLY_AIR_TEMPERATURE = "supply_air_temperature"
CONF_RETURN_AIR_TEMPERATURE = "return_air_temperature"
CONF_EXHAUST_AIR_TEMPERATURE = "exhaust_air_temperature"
CONF_ENTHALPY_TEMPERATURE = "enthalpy_temperature"
CONF_EWT_TEMPERATURE = "ewt_temperature"
CONF_REHEATING_TEMPERATURE = "reheating_temperature"
CONF_KITCHEN_HOOD_TEMPERATURE = "kitchen_hood_temperature"
CONF_RETURN_AIR_LEVEL = "return_air_level"
CONF_SUPPLY_AIR_LEVEL = "supply_air_level"
CONF_IS_SUPPLY_FAN_ACTIVE = "is_supply_fan_active"
CONF_IS_FILTER_FULL = "is_filter_full"
CONF_BYPASS_FACTOR = "bypass_factor"
CONF_BYPASS_STEP = "bypass_step"
CONF_BYPASS_CORRECTION = "bypass_correction"
CONF_IS_SUMMER_MODE = "is_summer_mode"

CONF_EWT_LOW_TEMPERATURE = "ewt_low_temperature"
CONF_EWT_HIGH_TEMPERATURE = "ewt_high_temperature"
CONF_EWT_SPEED_UP_PERCENTAGE = "ewt_speed_up_percentage"
CONF_REHEATING_TARGET_TEMPERATURE = "reheating_target_temperature"
CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE = "kitchen_hood_speed_up_percentage"

SENSORS = {
    CONF_FAN_SUPPLY_AIR_PERCENTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0),
    CONF_FAN_EXHAUST_AIR_PERCENTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0),
    CONF_FAN_SPEED_SUPPLY: sensor.sensor_schema(unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE, accuracy_decimals=0),
    CONF_FAN_SPEED_EXHAUST: sensor.sensor_schema(unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE, accuracy_decimals=0),
    CONF_OUTSIDE_AIR_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_SUPPLY_AIR_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_RETURN_AIR_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_EXHAUST_AIR_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_ENTHALPY_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_EWT_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_REHEATING_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_KITCHEN_HOOD_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_RETURN_AIR_LEVEL: sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_CUBIC_METER, accuracy_decimals=0),
    CONF_SUPPLY_AIR_LEVEL: sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_CUBIC_METER, accuracy_decimals=0),
    CONF_BYPASS_FACTOR: sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BYPASS_STEP: sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_BYPASS_CORRECTION: sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    CONF_EWT_LOW_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_EWT_HIGH_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_EWT_SPEED_UP_PERCENTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=1),
    CONF_REHEATING_TARGET_TEMPERATURE: sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
    CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE: sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=1),
}

BINARY_SENSORS = {
    CONF_IS_BYPASS_VALVE_OPEN: binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY),
    CONF_IS_PREHEATING: binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY),
    CONF_IS_SUMMER_MODE: binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY),
    CONF_IS_SUPPLY_FAN_ACTIVE: binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY),
    CONF_IS_FILTER_FULL: binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY),
}

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(ComfoDComponent)
    .extend({
        cv.Required(REQUIRED_KEY_UART_COMFOD): cv.use_id(uart.UARTComponent),
        cv.Optional(OPTIONAL_KEY_UART_COMFOSENSE): cv.use_id(uart.UARTComponent),
    })
    .extend({cv.Optional(key): schema for key, schema in SENSORS.items()})
    .extend({cv.Optional(key): schema for key, schema in BINARY_SENSORS.items()})
    .extend(cv.polling_component_schema("1s"))
)

async def to_code(config):
    """Generates code"""
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    uart_comfod = await cg.get_variable(config[REQUIRED_KEY_UART_COMFOD])
    cg.add(var.set_uart_comfod(uart_comfod))
    if OPTIONAL_KEY_UART_COMFOSENSE in config:
        uart_comfosense = await cg.get_variable(config[OPTIONAL_KEY_UART_COMFOSENSE])
        cg.add(var.set_uart_comfosense(uart_comfosense))

    for key in SENSORS:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, 'set_' + key)(sens))

    for key in BINARY_SENSORS:
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, 'set_' + key)(sens))
