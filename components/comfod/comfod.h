#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "esphome/components/sensor/sensor.h"
#include "constants.h"

namespace esphome {
namespace comfod {

class ComfoDComponent : public climate::Climate, public PollingComponent, public uart::UARTDevice {
public:

  ComfoDComponent()
    : Climate(), PollingComponent(600) {}

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_visual_min_temperature(COMFOD_MIN_SUPPORTED_TEMP);
    traits.set_visual_max_temperature(COMFOD_MAX_SUPPORTED_TEMP);
    traits.set_visual_temperature_step(COMFOD_SUPPORTED_TEMP_STEP);
    traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_FAN_ONLY
    });
    traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_OFF,
    }); 
    return traits;
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    if (call.get_fan_mode().has_value()) {
      int level;

      fan_mode = *call.get_fan_mode();
      switch (fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
          level = 0x04;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          level = 0x03;
          break;
        case climate::CLIMATE_FAN_LOW:
          level = 0x02;
          break;
        case climate::CLIMATE_FAN_OFF:
          level = 0x01;
          break;
        case climate::CLIMATE_FAN_AUTO:
          level = 0x00;
          break;
        case climate::CLIMATE_FAN_FOCUS:
        case climate::CLIMATE_FAN_ON:
        case climate::CLIMATE_FAN_MIDDLE:
        case climate::CLIMATE_FAN_DIFFUSE:
        default:
          level = -1;
          break;
      }

      if (level >= 0) {
        set_level_(level);
      }

    }
    if (call.get_target_temperature().has_value()) {
      target_temperature = *call.get_target_temperature();
      set_comfort_temperature_(target_temperature);
    }

    publish_state();
  }

  void dump_config() override {
    uint8_t *p;
    ESP_LOGCONFIG(TAG, "COMFOD:");
    LOG_UPDATE_INTERVAL(this);
    p = bootloader_version_;
    ESP_LOGCONFIG(TAG, "  Bootloader %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
    p = firmware_version_;
    ESP_LOGCONFIG(TAG, "  Firmware %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
    p = connector_board_version_;
    ESP_LOGCONFIG(TAG, "  Connector Board %.10s v%0d.%02d", p + 2, *p, *(p + 1));

    if (*(p + 12) != 0) {
      ESP_LOGCONFIG(TAG, "  CC-Ease v%0d.%02d", *(p + 12) >> 4, *(p + 12) & 0x0f);
    }
    if (*(p + 13) != 0) {
      ESP_LOGCONFIG(TAG, "  CC-Luxe v%0d.%02d", *(p + 13) >> 4, *(p + 13) & 0x0f);
    }
  }

  void update() override {
    switch(update_counter_) {
      case -4:
        set_rs232_mode(3);
        break;
      case -3:
        ESP_LOGI(TAG, "Get bootloader version");
        write_command_(COMFOD_GET_BOOTLOADER_VERSION_REQUEST, nullptr, 0);
        break;
      case -2:
        ESP_LOGI(TAG, "Get firmware version");
        write_command_(COMFOD_GET_FIRMWARE_VERSION_REQUEST, nullptr, 0);
        break;
      case -1:
        ESP_LOGI(TAG, "Get board version");
        write_command_(COMFOD_GET_BOARD_VERSION_REQUEST, nullptr, 0);
        break;
      case 0:
        get_fan_status_();
        break;
      case 1:
        get_valve_status_();
        break;
      case 2:
        get_sensor_data_();
        break;
      case 3:
        get_ventilation_level_();
        break;
      case 4:
        get_temperatures_();
        break;
      case 5:
        get_error_status_();
        break;
      case 6:
        get_bypass_control_status_();
        break;
      case 7:
        get_ewt_reheating_status_();
        break;
    }

    update_counter_++;
    if (update_counter_ > 7)
      update_counter_ = 0;
  }

  void loop() override {
    // Proxy commands from the display (ComfoSense -> ComfoD).
    while(uart_comfosense->available() != 0) {
      uart_comfosense->read_byte(&comfosense_data_[comfosense_data_index_]);
      auto check = check_byte_(comfosense_data_, comfosense_data_index_);
      if (!check.has_value()) {
        ESP_LOGI(TAG, "Proxying command 0x%02X from ComfoSense to ComfoD with %i bytes.", comfosense_data_[COMFOD_MSG_IDENTIFIER_IDX], comfosense_data_index_+1);
        uart_comfod->write_array(comfosense_data_, comfosense_data_index_+1);
        uart_comfod->flush();
        comfosense_data_index_ = 0;
        break;
      } else if (!*check) {
        // wrong data
        ESP_LOGI(TAG, "Byte %i of received data frame is invalid.", comfosense_data_index_);
        comfosense_data_index_ = 0;
      } else {
        comfosense_data_index_++;
      }
    }

    while (uart_comfod->available() != 0) {
      uart_comfod->read_byte(&comfod_data_[comfod_data_index_]);
      auto check = check_byte_(comfod_data_, comfod_data_index_);
      if (!check.has_value()) {

        // finished
        if (comfod_data_[COMFOD_MSG_ACK_IDX] != COMFOD_MSG_ACK) {
          parse_data_();
        }
        
        ESP_LOGI(TAG, "Proxying response 0x%02X from ComfoD to ComfoSense with %i bytes.", comfod_data_[COMFOD_MSG_IDENTIFIER_IDX], comfod_data_index_+1);
        uart_comfosense->write_array(comfod_data_, comfod_data_index_+1);
        uart_comfosense->flush();
        comfod_data_index_ = 0;
      } else if (!*check) {
        // wrong data
        ESP_LOGI(TAG, "Byte %i of received data frame is invalid.", comfod_data_index_);
        comfod_data_index_ = 0;
      } else {
        // next byte
        comfod_data_index_++;
      }
    }
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void reset_filter(void) {
    uint8_t reset_cmd[COMFOD_SET_RESET_LENGTH] = {0, 0, 0, 1};
    write_command_(COMFOD_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void set_name(const char* value) {name = value;}
  void set_uart_comfod(uart::UARTComponent *uart_comfod) { uart_comfod = uart_comfod; }
  void set_uart_comfosense(uart::UARTComponent *uart_comfosense) { uart_comfosense = uart_comfosense; }
  
  void update_ewt_reheater_state_(uint8_t* msg) {
    //set input numbers
    if (ewt_low_temperature != nullptr && ewt_low_temperature->state != (float) msg[0] / 2.0f - 20.0f) {
      ewt_low_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
    }
    if (ewt_high_temperature != nullptr && ewt_high_temperature->state != (float) msg[1] / 2.0f - 20.0f) {
      ewt_high_temperature->publish_state((float) msg[1] / 2.0f - 20.0f);
    }
    if (ewt_speed_up_percentage != nullptr && ewt_speed_up_percentage->state != msg[2]) {
      ewt_speed_up_percentage->publish_state(msg[2]);
    }
  
    //kitchen hood speed up %
    if (kitchen_hood_speed_up_percentage != nullptr && kitchen_hood_speed_up_percentage->state != msg[3]) {
      kitchen_hood_speed_up_percentage->publish_state(msg[3]);
    }
    // reheating target temperature
    if (reheating_target_temperature != nullptr && reheating_target_temperature->state != (float) msg[6] / 2.0f - 20.0f) {
      reheating_target_temperature->publish_state((float) msg[6] / 2.0f - 20.0f);
    }
  }

  void set_ewt_low_temperature_value(float value) {
    configure_ewt_reheating_settings_(value, ewt_high_temperature->state, ewt_speed_up_percentage->state, kitchen_hood_speed_up_percentage->state, reheating_target_temperature->state);
    ewt_low_temperature->publish_state(value);
  }
  void set_ewt_high_temperature_value(float vlaue) {
    configure_ewt_reheating_settings_(ewt_low_temperature->state, vlaue, ewt_speed_up_percentage->state, kitchen_hood_speed_up_percentage->state, reheating_target_temperature->state);
    ewt_high_temperature->publish_state(vlaue);
  }
  void set_ewt_speed_up_percentage_value(int value) {
    configure_ewt_reheating_settings_(ewt_low_temperature->state, ewt_high_temperature->state, value, kitchen_hood_speed_up_percentage->state, reheating_target_temperature->state);
    ewt_speed_up_percentage->publish_state(value);
  }
  
 protected:

  void set_level_(int level) {
    if (level < 0 || level > 4) {
      ESP_LOGI(TAG, "Ignoring invalid level request: %i", level);
      return;
    }

    ESP_LOGI(TAG, "Setting level to: %i", level);
    uint8_t command[COMFOD_SET_LEVEL_LENGTH] = {(uint8_t) level};
    write_command_(COMFOD_SET_LEVEL_REQUEST, command, sizeof(command));
  }

  void set_comfort_temperature_(float temperature) {
    if (temperature < COMFOD_MIN_SUPPORTED_TEMP || temperature > COMFOD_MAX_SUPPORTED_TEMP) {
      ESP_LOGI(TAG, "Ignoring invalid temperature request: %.1f", temperature);
      return;
    }

    ESP_LOGI(TAG, "Setting temperature to: %.1f", temperature);
    
    uint8_t command[COMFOD_SET_COMFORT_TEMPERATURE_LENGTH] = {(uint8_t) ((temperature + 20.0f) * 2.0f)};
    write_command_(COMFOD_SET_COMFORT_TEMPERATURE_REQUEST, command, sizeof(command));
  }

  void configure_ewt_reheating_settings_(float low_temperature, float high_temperature, int ewt_speed_up_percentage, int kitchen_hood_speed_up_percentage, int reheating_target_temperature) {
    if (low_temperature < 0 || low_temperature > 15) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. Low temperature: %i", low_temperature);
      return;
    }
    
    if (high_temperature < 10 || high_temperature > 25) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. High temperature: %i", low_temperature);
      return;
    }
    
    if (ewt_speed_up_percentage < 0 || ewt_speed_up_percentage > 99) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. Speed up percentage: %i", ewt_speed_up_percentage);
      return;
    }
    
    if (kitchen_hood_speed_up_percentage < 0 || kitchen_hood_speed_up_percentage > 99) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. Kitchen hood speed up percentage: %i", kitchen_hood_speed_up_percentage);
      return;
    }

    if (reheating_target_temperature < 5 || reheating_target_temperature > 40) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. Reheater temperature: %i", reheating_target_temperature);
      return;
    }

    ESP_LOGI(TAG, "Configuring EWT & Reheater settings. Low Temperature: %.1f, High Temperature: %.1f, EWT speed up: %i%, Kitchen hood speed up percentage: %i%, Reheating target temperature: %i", low_temperature, high_temperature, ewt_speed_up_percentage, kitchen_hood_speed_up_percentage, reheating_target_temperature);
    uint8_t command[COMFOD_SET_EWT_REHEATING_LENGTH] = {(uint8_t) ((low_temperature + 20.0f) * 2.0f), (uint8_t) ((high_temperature + 20.0f) * 2.0f), (uint8_t) (ewt_speed_up_percentage), (uint8_t) (kitchen_hood_speed_up_percentage), (uint8_t) ((reheating_target_temperature + 20.0f) * 2.0f)};
    write_command_(COMFOD_SET_EWT_REHEATING_REQUEST, command, sizeof(command));
  }
  
  void set_rs232_mode(int mode) {
    if (mode < 0 || mode > 4) {
      ESP_LOGI(TAG, "Ignoring invalid mode request: %i", mode);
      return;
    }

    ESP_LOGI(TAG, "Setting rs232 mode to: %i", mode);
    uint8_t command[] = {(uint8_t) mode};
    write_command_(COMFOD_SET_RS232_MODE_REQUEST, command, sizeof(command));
  }

  void write_command_(const uint8_t command, const uint8_t *command_data, uint8_t command_data_length) {
    uart_comfod->write_byte(COMFOD_MSG_PREFIX);
    uart_comfod->write_byte(COMFOD_MSG_HEAD);
    uart_comfod->write_byte(0x00);
    uart_comfod->write_byte(command);
    uart_comfod->write_byte(command_data_length);
    if (command_data_length > 0) {
      uart_comfod->write_array(command_data, command_data_length);
      uart_comfod->write_byte((command + command_data_length + comfod_checksum_(command_data, command_data_length)) & 0xff);
    } else {
      uart_comfod->write_byte(comfod_checksum_(&command, 1));
    }
    uart_comfod->write_byte(COMFOD_MSG_PREFIX);
    uart_comfod->write_byte(COMFOD_MSG_TAIL);
    uart_comfod->flush();
  }

  uint8_t comfod_checksum_(const uint8_t *command_data, uint8_t length) const {
    uint8_t sum = 0;
    bool last_seven = false;
    for (uint8_t i = 0; i < length; i++) {
      if (command_data[i] == 0x07) {
        if (last_seven) {
          last_seven = false;
          continue;
        }
        last_seven = true;
      }
      sum += command_data[i];
    }
    return sum + 0xad;
  }

  optional<bool> check_byte_(uint8_t* data, uint8_t index) const {
    const uint8_t byte = data[index];

    if (index == 0)
      return byte == COMFOD_MSG_PREFIX;

    if (index == 1) {
      if (byte == COMFOD_MSG_ACK)
        return {};
      else
        return byte == COMFOD_MSG_HEAD;
    }

    if (index == 2)
      return byte == 0x00;

    if (index < COMFOD_MSG_HEAD_LENGTH)
      return true;

    uint8_t data_length = data[COMFOD_MSG_DATA_LENGTH_IDX];

    if ((COMFOD_MSG_HEAD_LENGTH + data_length + COMFOD_MSG_TAIL_LENGTH) > sizeof(comfod_data_)) {
      ESP_LOGW(TAG, "COMFOD message too large. %i was larger than %i", (COMFOD_MSG_HEAD_LENGTH + data_length + COMFOD_MSG_TAIL_LENGTH), sizeof(data));
      return false;
    }

    if (index < COMFOD_MSG_HEAD_LENGTH + data_length)
      return true;

    if (index == COMFOD_MSG_HEAD_LENGTH + data_length) {
      // checksum is without checksum bytes
      uint8_t checksum = comfod_checksum_(data + 2, COMFOD_MSG_HEAD_LENGTH + data_length - 2);
      if (checksum != byte) {
        //ESP_LOG_BUFFER_HEX(TAG, data_, index+1);
        ESP_LOGW(TAG, "COMFOD Checksum doesn't match: 0x%02X!=0x%02X", byte, checksum);
        return false;
      }
      return true;
    }

    if (index == COMFOD_MSG_HEAD_LENGTH + data_length + 1)
      return byte == COMFOD_MSG_PREFIX;

    if (index == COMFOD_MSG_HEAD_LENGTH + data_length + 2) {
      if (byte != COMFOD_MSG_TAIL)
        return false;
    }

    return {};
  }

  void parse_data_() {
    status_clear_warning();
    uint8_t *msg = &comfod_data_[COMFOD_MSG_HEAD_LENGTH];

    switch (comfod_data_[COMFOD_MSG_IDENTIFIER_IDX]) {
      case COMFOD_GET_BOOTLOADER_VERSION_RESPONSE:
        memcpy(bootloader_version_, msg, comfod_data_[COMFOD_MSG_DATA_LENGTH_IDX]);
        break;
      case COMFOD_GET_FIRMWARE_VERSION_RESPONSE:
        memcpy(firmware_version_, msg, comfod_data_[COMFOD_MSG_DATA_LENGTH_IDX]);
        break;
      case COMFOD_GET_BOARD_VERSION_RESPONSE:
        memcpy(connector_board_version_, msg, comfod_data_[COMFOD_MSG_DATA_LENGTH_IDX]);
        break;
      case COMFOD_GET_FAN_STATUS_RESPONSE: {
          if (fan_supply_air_percentage != nullptr && fan_supply_air_percentage->state != msg[0]) {
            fan_supply_air_percentage->publish_state(msg[0]);
          }
          if (fan_exhaust_air_percentage != nullptr && fan_exhaust_air_percentage->state != msg[1]) {
            fan_exhaust_air_percentage->publish_state(msg[1]);
          }
          if (fan_speed_supply != nullptr && fan_speed_supply->state != 1875000.0f / get_uint16_(2)) {
            fan_speed_supply->publish_state(1875000.0f / get_uint16_(2));
          }
          if (fan_speed_exhaust != nullptr && fan_speed_exhaust->state != 1875000.0f / get_uint16_(4)) {
            fan_speed_exhaust->publish_state(1875000.0f / get_uint16_(4));
          }
          break;
        }
      case COMFOD_GET_VALVE_STATUS_RESPONSE: {
        if (is_bypass_valve_open != nullptr) {
          is_bypass_valve_open->publish_state(msg[0] != 0 && msg[0] != 255);
        }
        if (is_preheating != nullptr) {
            is_preheating->publish_state(msg[1] == 1);
        }
        break;
      }
      case COMFOD_GET_BYPASS_CONTROL_RESPONSE: {
        if (bypass_factor != nullptr) {
          bypass_factor->publish_state(msg[2]);
        }
        if (bypass_step != nullptr) {
          bypass_step->publish_state(msg[3]);
        }
        if (bypass_correction != nullptr) {
          bypass_correction->publish_state(msg[4]);
        }
        if (is_summer_mode != nullptr) {
          is_summer_mode->publish_state(msg[6] != 0);
        }
        break;
      }
      case COMFOD_GET_TEMPERATURE_RESPONSE: {

        // T1 / outside air
        if (outside_air_temperature != nullptr) {
          outside_air_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
        }
        // T2 / supply air
        if (supply_air_temperature != nullptr) {
          supply_air_temperature->publish_state((float) msg[1] / 2.0f - 20.0f);
        }
        // T3 / return air
        if (return_air_temperature != nullptr) {
          return_air_temperature->publish_state((float) msg[2] / 2.0f - 20.0f);
        }
        // T4 / exhaust air
        if (exhaust_air_temperature != nullptr) {
          exhaust_air_temperature->publish_state((float) msg[3] / 2.0f - 20.0f);
        }
        break;
      }
      case COMFOD_GET_SENSOR_DATA_RESPONSE: {

        if (enthalpy_temperature != nullptr) {
          enthalpy_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
        }

        break;
      }
      case COMFOD_GET_VENTILATION_LEVEL_RESPONSE: {

        ESP_LOGD(TAG, "Level %02x", msg[8]);

        if (return_air_level != nullptr && return_air_level->state != msg[6]) {
          return_air_level->publish_state(msg[6]);
        }
        if (supply_air_level != nullptr && supply_air_level->state != msg[7]) {
          supply_air_level->publish_state(msg[7]);
        }

        auto current_fan_mode = fan_mode;
        auto current_mode = mode;
        // Fan Speed
        switch(msg[8]) {
          case 0x00:
            fan_mode = climate::CLIMATE_FAN_AUTO;
            mode = climate::CLIMATE_MODE_AUTO;
            break;
          case 0x01:
            fan_mode = climate::CLIMATE_FAN_OFF;
            mode = climate::CLIMATE_MODE_OFF;
            break;
          case 0x02:
            fan_mode = climate::CLIMATE_FAN_LOW;
            mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
          case 0x03:
            fan_mode = climate::CLIMATE_FAN_MEDIUM;
            mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
          case 0x04:
            fan_mode = climate::CLIMATE_FAN_HIGH;
            mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
        }

        if (fan_mode != current_fan_mode || mode != current_mode) {
          publish_state();
        }

        // Supply air fan active (1 = active / 0 = inactive)
        if (is_supply_fan_active != nullptr) {
          is_supply_fan_active->publish_state(msg[9] == 1);
        }
        break;
      }
      case COMFOD_GET_ERROR_STATE_RESPONSE: {
        if (is_filter_full != nullptr) {
          is_filter_full->publish_state(msg[8] != 0);
        }
        break;
      }
      case COMFOD_GET_TEMPERATURES_RESPONSE: {

        auto current_target_temperature = target_temperature;
        auto current_current_temperature = current_temperature;

        // comfort temperature
        target_temperature = (float) msg[0] / 2.0f - 20.0f;
        //indoor temperature
        current_temperature = (float) msg[3] / 2.0f - 20.0f;

        //Not yet known on startup, causes MQTT error (Invalid fan_modes mode:, Invalid modes mode:) STILL NOT SOLVED
        if ((fan_mode && mode) && (current_target_temperature != target_temperature || current_current_temperature != current_temperature)) {
          publish_state();
        }

        // T1 / outside air
        if (outside_air_temperature != nullptr && outside_air_temperature->state != (float) msg[1] / 2.0f - 20.0f && msg[5] & 0x01) {
          outside_air_temperature->publish_state((float) msg[1] / 2.0f - 20.0f);
        }
        // T2 / supply air
        if (supply_air_temperature != nullptr && supply_air_temperature->state != (float) msg[2] / 2.0f - 20.0f && msg[5] & 0x02) {
          supply_air_temperature->publish_state((float) msg[2] / 2.0f - 20.0f);
        }
        // T3 / returned air
        if (return_air_temperature != nullptr && return_air_temperature->state != (float) msg[3] / 2.0f - 20.0f && msg[5] & 0x04) {
          return_air_temperature->publish_state((float) msg[3] / 2.0f - 20.0f);
        }
        // T4 / exhaust air
        if (exhaust_air_temperature != nullptr && exhaust_air_temperature->state != (float) msg[4] / 2.0f - 20.0f && msg[5] & 0x08) {
          exhaust_air_temperature->publish_state((float) msg[4] / 2.0f - 20.0f);
        }
        // EWT
        if (ewt_temperature != nullptr && ewt_temperature->state != (float) msg[6] / 2.0f - 20.0f && msg[5] & 0x10) {
          ewt_temperature->publish_state((float) msg[6] / 2.0f - 20.0f);
        }
        // reheating
        if (reheating_temperature != nullptr && reheating_temperature->state != (float) msg[7] / 2.0f - 20.0f && msg[5] & 0x20) {
          reheating_temperature->publish_state((float) msg[7] / 2.0f - 20.0f);
        }
        // kitchen hood
        if (kitchen_hood_temperature != nullptr && kitchen_hood_temperature->state != (float) msg[8] / 2.0f - 20.0f && msg[5] & 0x40) {
          kitchen_hood_temperature->publish_state((float) msg[8] / 2.0f - 20.0f);
        }

        break;
      }
      case COMFOD_GET_EWT_REHEATER_RESPONSE: {
        update_ewt_reheater_state_(msg);
      }
    }
  }

  void get_fan_status_() {
    if (fan_supply_air_percentage != nullptr || fan_exhaust_air_percentage != nullptr || fan_speed_supply != nullptr || fan_speed_exhaust != nullptr) {
      ESP_LOGD(TAG, "getting fan status");
      write_command_(COMFOD_GET_FAN_STATUS_REQUEST, nullptr, 0);
    }
  }

  void get_valve_status_() {
    if (is_bypass_valve_open != nullptr || is_preheating != nullptr) {
      ESP_LOGD(TAG, "getting valve status");
      write_command_(COMFOD_GET_VALVE_STATUS_REQUEST, nullptr, 0);
    }
  }

  void get_error_status_() {
    if (is_filter_full != nullptr) {
      ESP_LOGD(TAG, "getting error status");
      write_command_(COMFOD_GET_ERROR_STATE_REQUEST, nullptr, 0);
    }
  }

  void get_bypass_control_status_() {
    if (bypass_factor != nullptr || bypass_step != nullptr || bypass_correction != nullptr || is_summer_mode != nullptr) {
      ESP_LOGD(TAG, "getting bypass control");
      write_command_(COMFOD_GET_BYPASS_CONTROL_REQUEST, nullptr, 0);
    }
  }

  void get_ewt_reheating_status_() {
    if (kitchen_hood_speed_up_percentage != nullptr || reheating_target_temperature != nullptr) {
      ESP_LOGD(TAG, "getting ewt reheating status");
      write_command_(COMFOD_GET_EWT_REHEATER_REQUEST, nullptr, 0);
    }
  }
  void get_temperature_() {
    if (outside_air_temperature != nullptr || supply_air_temperature != nullptr || return_air_temperature != nullptr || outside_air_temperature != nullptr) {
      ESP_LOGD(TAG, "getting temperature");
      write_command_(COMFOD_GET_TEMPERATURE_REQUEST, nullptr, 0);
    }
  }

  void get_sensor_data_() {
    if (enthalpy_temperature != nullptr) {
      ESP_LOGD(TAG, "getting sensor data");
      write_command_(COMFOD_GET_SENSOR_DATA_REQUEST, nullptr, 0);
    }
  }

  void get_ventilation_level_() {
    ESP_LOGD(TAG, "getting ventilation level");
    write_command_(COMFOD_GET_VENTILATION_LEVEL_REQUEST, nullptr, 0);
  }

  void get_temperatures_() {
    ESP_LOGD(TAG, "getting temperatures");
    write_command_(COMFOD_GET_TEMPERATURES_REQUEST, nullptr, 0);
  }

  uint8_t get_uint8_t_(uint8_t start_index) const {
    return comfod_data_[COMFOD_MSG_HEAD_LENGTH + start_index];
  }

  uint16_t get_uint16_(uint8_t start_index) const {
    return (uint16_t(comfod_data_[COMFOD_MSG_HEAD_LENGTH + start_index + 1] | comfod_data_[COMFOD_MSG_HEAD_LENGTH + start_index] << 8));
  }

  uint8_t comfod_data_[30];
  uint8_t comfod_data_index_{0};
  uint8_t comfosense_data_[30];
  uint8_t comfosense_data_index_{0};
  
  int8_t update_counter_{-10};

  uint8_t bootloader_version_[13]{0};
  uint8_t firmware_version_[13]{0};
  uint8_t connector_board_version_[14]{0};
  const char* name{0};
  
  uart::UARTComponent* uart_comfod;
  uart::UARTComponent* uart_comfosense;
  
public:
  sensor::Sensor *fan_supply_air_percentage{nullptr};
  sensor::Sensor *fan_exhaust_air_percentage{nullptr};
  sensor::Sensor *fan_speed_supply{nullptr};
  sensor::Sensor *fan_speed_exhaust{nullptr};
  binary_sensor::BinarySensor *is_bypass_valve_open{nullptr};
  binary_sensor::BinarySensor *is_preheating{nullptr};
  sensor::Sensor *outside_air_temperature{nullptr};
  sensor::Sensor *supply_air_temperature{nullptr};
  sensor::Sensor *return_air_temperature{nullptr};
  sensor::Sensor *exhaust_air_temperature{nullptr};
  sensor::Sensor *enthalpy_temperature{nullptr};
  sensor::Sensor *ewt_temperature{nullptr};
  sensor::Sensor *reheating_temperature{nullptr};
  sensor::Sensor *kitchen_hood_temperature{nullptr};
  sensor::Sensor *return_air_level{nullptr};
  sensor::Sensor *supply_air_level{nullptr};
  binary_sensor::BinarySensor *is_supply_fan_active{nullptr};
  binary_sensor::BinarySensor *is_filter_full{nullptr};
  
  sensor::Sensor *bypass_factor{nullptr};
  sensor::Sensor *bypass_step{nullptr};
  sensor::Sensor *bypass_correction{nullptr};
  binary_sensor::BinarySensor *is_summer_mode{nullptr};

  sensor::Sensor *ewt_low_temperature{nullptr};
  sensor::Sensor *ewt_high_temperature{nullptr};
  sensor::Sensor *ewt_speed_up_percentage{nullptr};
  sensor::Sensor *kitchen_hood_speed_up_percentage{nullptr};
  sensor::Sensor *reheating_target_temperature{nullptr};

  void set_fan_supply_air_percentage(sensor::Sensor *fan_supply_air_percentage) { this->fan_supply_air_percentage = fan_supply_air_percentage; }
  void set_fan_exhaust_air_percentage(sensor::Sensor *fan_exhaust_air_percentage) { this->fan_exhaust_air_percentage = fan_exhaust_air_percentage; }
  void set_fan_speed_supply(sensor::Sensor *fan_speed_supply) { this->fan_speed_supply = fan_speed_supply; }
  void set_fan_speed_exhaust(sensor::Sensor *fan_speed_exhaust) { this->fan_speed_exhaust = fan_speed_exhaust; }
  void set_is_bypass_valve_open(binary_sensor::BinarySensor *is_bypass_valve_open) { this->is_bypass_valve_open = is_bypass_valve_open; }
  void set_is_preheating(binary_sensor::BinarySensor *is_preheating) { this->is_preheating = is_preheating; }
  void set_outside_air_temperature(sensor::Sensor *outside_air_temperature) { this->outside_air_temperature = outside_air_temperature; }
  void set_supply_air_temperature(sensor::Sensor *supply_air_temperature) { this->supply_air_temperature = supply_air_temperature; }
  void set_return_air_temperature(sensor::Sensor *return_air_temperature) { this->return_air_temperature = return_air_temperature; }
  void set_exhaust_air_temperature(sensor::Sensor *exhaust_air_temperature) { this->exhaust_air_temperature = exhaust_air_temperature; }
  void set_enthalpy_temperature(sensor::Sensor *enthalpy_temperature) { this->enthalpy_temperature = enthalpy_temperature; }
  void set_ewt_temperature(sensor::Sensor *ewt_temperature) { this->ewt_temperature = ewt_temperature; }
  void set_ewt_low_temperature(sensor::Sensor *ewt_low_temperature) { this->ewt_low_temperature = ewt_low_temperature; }
  void set_ewt_high_temperature(sensor::Sensor *ewt_high_temperature) { this->ewt_high_temperature = ewt_high_temperature; }
  void set_ewt_speed_up_percentage(sensor::Sensor *ewt_speed_up_percentage) { this->ewt_speed_up_percentage = ewt_speed_up_percentage; }
  void set_kitchen_hood_speed_up_percentage(sensor::Sensor *kitchen_hood_speed_up_percentage) { this->kitchen_hood_speed_up_percentage = kitchen_hood_speed_up_percentage; }
  void set_reheating_target_temperature(sensor::Sensor *reheating_target_temperature) { this->reheating_target_temperature = reheating_target_temperature; }
  void set_kitchen_hood_temperature(sensor::Sensor *kitchen_hood_temperature) { this->kitchen_hood_temperature = kitchen_hood_temperature; }
  void set_reheating_temperature(sensor::Sensor *reheating_temperature) { this->reheating_temperature = reheating_temperature; }
  void set_return_air_level(sensor::Sensor *return_air_level) { this->return_air_level = return_air_level; }
  void set_supply_air_level(sensor::Sensor *supply_air_level) { this->supply_air_level = supply_air_level; }
  void set_is_supply_fan_active(binary_sensor::BinarySensor *is_supply_fan_active) { this->is_supply_fan_active = is_supply_fan_active; }
  void set_is_filter_full(binary_sensor::BinarySensor *is_filter_full) { this->is_filter_full = is_filter_full; }
  void set_bypass_factor(sensor::Sensor *bypass_factor) { this->bypass_factor = bypass_factor; }
  void set_bypass_step(sensor::Sensor *bypass_step) { this->bypass_step = bypass_step; }
  void set_bypass_correction(sensor::Sensor *bypass_correction) { this->bypass_correction = bypass_correction; }
  void set_is_summer_mode(binary_sensor::BinarySensor *is_summer_mode) { this->is_summer_mode = is_summer_mode; }
};

}  // namespace comfod
}  // namespace esphome