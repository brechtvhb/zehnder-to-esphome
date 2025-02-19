""" Creates module comfod """
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import binary_sensor, sensor, text_sensor, uart
from esphome.const import (CONF_ID, DEVICE_CLASS_CURRENT,
                           DEVICE_CLASS_EMPTY, DEVICE_CLASS_SPEED,
                           DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_VOLUME,
                           STATE_CLASS_MEASUREMENT, UNIT_AMPERE, UNIT_CELSIUS,
                           UNIT_CUBIC_METER, UNIT_HOUR, UNIT_MINUTE,
                           UNIT_PERCENT, UNIT_REVOLUTIONS_PER_MINUTE)

comfod_ns = cg.esphome_ns.namespace('comfod')
ComfoDComponent = comfod_ns.class_('ComfoDComponent', cg.Component)

DEPENDENCIES=['uart']
AUTO_LOAD = ['sensor', 'climate', 'binary_sensor', 'number', 'button']
REQUIRED_KEY_NAME = "name"
REQUIRED_KEY_UART_COMFOD = "uart_comfod"
REQUIRED_KEY_UART_COMFOSENSE = "uart_comfosense"

MULTI_CONF = True
CONF_HUB_ID = 'comfod'

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

helper_comfod_list = [
    CONF_FAN_SUPPLY_AIR_PERCENTAGE,
    CONF_FAN_EXHAUST_AIR_PERCENTAGE,
    CONF_FAN_SPEED_SUPPLY,
    CONF_FAN_SPEED_EXHAUST,
    CONF_IS_BYPASS_VALVE_OPEN,
    CONF_IS_PREHEATING,
    CONF_OUTSIDE_AIR_TEMPERATURE,
    CONF_SUPPLY_AIR_TEMPERATURE,
    CONF_RETURN_AIR_TEMPERATURE,
    CONF_EXHAUST_AIR_TEMPERATURE,
    CONF_ENTHALPY_TEMPERATURE,
    CONF_EWT_TEMPERATURE,
    CONF_REHEATING_TEMPERATURE,
    CONF_KITCHEN_HOOD_TEMPERATURE,
    CONF_RETURN_AIR_LEVEL,
    CONF_SUPPLY_AIR_LEVEL,
    CONF_IS_SUPPLY_FAN_ACTIVE,
    CONF_IS_FILTER_FULL,
    CONF_BYPASS_FACTOR,
    CONF_BYPASS_STEP,
    CONF_BYPASS_CORRECTION,
    CONF_IS_SUMMER_MODE,
    CONF_EWT_LOW_TEMPERATURE,
    CONF_EWT_HIGH_TEMPERATURE,
    CONF_EWT_SPEED_UP_PERCENTAGE,
    CONF_REHEATING_TARGET_TEMPERATURE,
    CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE,
]

comfod_sensors_schemas = cv.Schema({
cv.Optional(CONF_FAN_SUPPLY_AIR_PERCENTAGE): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_EXHAUST_AIR_PERCENTAGE): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_SPEED_SUPPLY): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_SPEED_EXHAUST): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_OUTSIDE_AIR_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_SUPPLY_AIR_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_RETURN_AIR_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EXHAUST_AIR_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_ENTHALPY_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EWT_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_REHEATING_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_KITCHEN_HOOD_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_RETURN_AIR_LEVEL): sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_CUBIC_METER, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_SUPPLY_AIR_LEVEL): sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_CUBIC_METER, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_FACTOR): sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_STEP): sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_CORRECTION): sensor.sensor_schema(device_class=DEVICE_CLASS_VOLUME, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_IS_BYPASS_VALVE_OPEN): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_PREHEATING): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_SUMMER_MODE): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_SUPPLY_FAN_ACTIVE): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_FILTER_FULL): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_EWT_LOW_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EWT_HIGH_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EWT_SPEED_UP_PERCENTAGE): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_REHEATING_TARGET_TEMPERATURE): sensor.sensor_schema(device_class=DEVICE_CLASS_TEMPERATURE, unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE): sensor.sensor_schema(device_class=DEVICE_CLASS_SPEED, unit_of_measurement=UNIT_PERCENT, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT).extend(),
})

CONFIG_SCHEMA = cv.All(
    cv.Schema( {
        cv.GenerateID(CONF_ID): cv.declare_id(ComfoDComponent),
        cv.Required(REQUIRED_KEY_NAME): cv.string,
        cv.Required(REQUIRED_KEY_UART_COMFOD): cv.string,
        cv.Optional(REQUIRED_KEY_UART_COMFOSENSE): cv.string,
    })
    .extend(comfod_sensors_schemas)
    .extend(cv.COMPONENT_SCHEMA)
)

def to_code(config):
    """Generates code"""
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_name(config[REQUIRED_KEY_NAME]))
    uart_comfod = yield cg.get_variable(config[REQUIRED_KEY_UART_COMFOD])
    uart_comfosense = yield cg.get_variable(config[REQUIRED_KEY_UART_COMFOSENSE])
    cg.add(var.set_uart_comfod(uart_comfod))
    cg.add(var.set_uart_comfosense(uart_comfosense))

    if CONF_FAN_SUPPLY_AIR_PERCENTAGE in config:
        sensor_id = config[CONF_FAN_SUPPLY_AIR_PERCENTAGE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_FAN_SUPPLY_AIR_PERCENTAGE)
        cg.add(func(sensor))

    if CONF_FAN_EXHAUST_AIR_PERCENTAGE in config:
        sensor_id = config[CONF_FAN_EXHAUST_AIR_PERCENTAGE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_FAN_EXHAUST_AIR_PERCENTAGE)
        cg.add(func(sensor))

    if CONF_FAN_SPEED_SUPPLY in config:
        sensor_id = config[CONF_FAN_SPEED_SUPPLY]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_FAN_SPEED_SUPPLY)
        cg.add(func(sensor))

    if CONF_FAN_SPEED_EXHAUST in config:
        sensor_id = config[CONF_FAN_SPEED_EXHAUST]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_FAN_SPEED_EXHAUST)
        cg.add(func(sensor))

    if CONF_OUTSIDE_AIR_TEMPERATURE in config:
        sensor_id = config[CONF_OUTSIDE_AIR_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_OUTSIDE_AIR_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_SUPPLY_AIR_TEMPERATURE in config:
        sensor_id = config[CONF_SUPPLY_AIR_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_SUPPLY_AIR_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_RETURN_AIR_TEMPERATURE in config:
        sensor_id = config[CONF_RETURN_AIR_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_RETURN_AIR_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_EXHAUST_AIR_TEMPERATURE in config:
        sensor_id = config[CONF_EXHAUST_AIR_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_EXHAUST_AIR_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_ENTHALPY_TEMPERATURE in config:
        sensor_id = config[CONF_ENTHALPY_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_ENTHALPY_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_EWT_TEMPERATURE in config:
        sensor_id = config[CONF_EWT_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_EWT_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_REHEATING_TEMPERATURE in config:
        sensor_id = config[CONF_REHEATING_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_REHEATING_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_KITCHEN_HOOD_TEMPERATURE in config:
        sensor_id = config[CONF_KITCHEN_HOOD_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_KITCHEN_HOOD_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_RETURN_AIR_LEVEL in config:
        sensor_id = config[CONF_RETURN_AIR_LEVEL]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_RETURN_AIR_LEVEL)
        cg.add(func(sensor))

    if CONF_SUPPLY_AIR_LEVEL in config:
        sensor_id = config[CONF_SUPPLY_AIR_LEVEL]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_SUPPLY_AIR_LEVEL)
        cg.add(func(sensor))

    if CONF_BYPASS_FACTOR in config:
        sensor_id = config[CONF_BYPASS_FACTOR]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_BYPASS_FACTOR)
        cg.add(func(sensor))

    if CONF_BYPASS_STEP in config:
        sensor_id = config[CONF_BYPASS_STEP]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_BYPASS_STEP)
        cg.add(func(sensor))

    if CONF_BYPASS_CORRECTION in config:
        sensor_id = config[CONF_BYPASS_CORRECTION]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_BYPASS_CORRECTION)
        cg.add(func(sensor))

    if CONF_IS_BYPASS_VALVE_OPEN in config:
        sensor_id = config[CONF_IS_BYPASS_VALVE_OPEN]
        sensor = yield binary_sensor.new_binary_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_IS_BYPASS_VALVE_OPEN)
        cg.add(func(sensor))

    if CONF_IS_PREHEATING in config:
        sensor_id = config[CONF_IS_PREHEATING]
        sensor = yield binary_sensor.new_binary_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_IS_PREHEATING)
        cg.add(func(sensor))

    if CONF_IS_SUMMER_MODE in config:
        sensor_id = config[CONF_IS_SUMMER_MODE]
        sensor = yield binary_sensor.new_binary_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_IS_SUMMER_MODE)
        cg.add(func(sensor))

    if CONF_IS_SUPPLY_FAN_ACTIVE in config:
        sensor_id = config[CONF_IS_SUPPLY_FAN_ACTIVE]
        sensor = yield binary_sensor.new_binary_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_IS_SUPPLY_FAN_ACTIVE)
        cg.add(func(sensor))

    if CONF_IS_FILTER_FULL in config:
        sensor_id = config[CONF_IS_FILTER_FULL]
        sensor = yield binary_sensor.new_binary_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_IS_FILTER_FULL)
        cg.add(func(sensor))

    if CONF_EWT_LOW_TEMPERATURE in config:
        sensor_id = config[CONF_EWT_LOW_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_EWT_LOW_TEMPERATURE)
        cg.add(func(sensor))

    if CONF_EWT_HIGH_TEMPERATURE in config:
        sensor_id = config[CONF_EWT_HIGH_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_EWT_HIGH_TEMPERATURE)
        cg.add(func(sensor))
    
    if CONF_EWT_SPEED_UP_PERCENTAGE in config:
        sensor_id = config[CONF_EWT_SPEED_UP_PERCENTAGE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_EWT_SPEED_UP_PERCENTAGE)
        cg.add(func(sensor))
    
    if CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE in config:
        sensor_id = config[CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_KITCHEN_HOOD_SPEED_UP_PERCENTAGE)
        cg.add(func(sensor))
        
    if CONF_REHEATING_TARGET_TEMPERATURE in config:
        sensor_id = config[CONF_REHEATING_TARGET_TEMPERATURE]
        sensor = yield sensor.new_sensor(sensor_id)
        func = getattr(var, 'set_'+ CONF_REHEATING_TARGET_TEMPERATURE)
        cg.add(func(sensor))


    cg.add(cg.App.register_climate(var))