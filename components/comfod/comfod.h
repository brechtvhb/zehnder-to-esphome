#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "constants.h"
#include "comfod_frame.h"

namespace esphome {
namespace comfod {

class ComfoDComponent : public climate::Climate, public PollingComponent {
public:

  ComfoDComponent()
    : Climate(), PollingComponent(1000) {}

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_visual_min_temperature(COMFOD_MIN_SUPPORTED_TEMP);
    traits.set_visual_max_temperature(COMFOD_MAX_SUPPORTED_TEMP);
    traits.set_visual_temperature_step(COMFOD_SUPPORTED_TEMP_STEP);
    traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_AUTO,
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
    int level = -1;

    if (call.get_mode().has_value()) {
      switch (*call.get_mode()) {
        case climate::CLIMATE_MODE_OFF:
          level = 0x01;
          break;
        case climate::CLIMATE_MODE_AUTO:
          level = 0x00;
          break;
        case climate::CLIMATE_MODE_FAN_ONLY:
          level = fan_mode_to_level_(fan_mode.value_or(climate::CLIMATE_FAN_LOW));
          if (level < 0x02)
            level = 0x02;
          break;
        default:
          break;
      }
    }

    if (call.get_fan_mode().has_value()) {
      level = fan_mode_to_level_(*call.get_fan_mode());
    }

    if (level >= 0) {
      set_level_(level);
      apply_level_(level);
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
    ESP_LOGCONFIG(TAG, "  ComfoSense proxy: %s", YESNO(uart_comfosense != nullptr));
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
    // Skip a poll rather than build up a backlog when the bus is busy.
    if (tx_queue_.size() >= 2)
      return;

    switch(update_counter_) {
      case -4:
        set_rs232_mode(3);
        break;
      case -3:
        ESP_LOGI(TAG, "Get bootloader version");
        request_(COMFOD_GET_BOOTLOADER_VERSION_REQUEST);
        break;
      case -2:
        ESP_LOGI(TAG, "Get firmware version");
        request_(COMFOD_GET_FIRMWARE_VERSION_REQUEST);
        break;
      case -1:
        ESP_LOGI(TAG, "Get board version");
        request_(COMFOD_GET_BOARD_VERSION_REQUEST);
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
    const uint32_t now = millis();
    read_comfod_(now);
    if (uart_comfosense != nullptr)
      read_comfosense_(now);
    send_next_command_(now);
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void reset_filter(void) {
    uint8_t reset_cmd[COMFOD_SET_RESET_LENGTH] = {0, 0, 0, 1};
    send_command_(COMFOD_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void set_uart_comfod(uart::UARTComponent *uart_comfod) { this->uart_comfod = uart_comfod; }
  void set_uart_comfosense(uart::UARTComponent *uart_comfosense) { this->uart_comfosense = uart_comfosense; }

  // Last EWT / reheating settings read from the ComfoD, for template numbers.
  optional<float> get_ewt_low_temperature_value() const { return ewt_reheating_setting_(0); }
  optional<float> get_ewt_high_temperature_value() const { return ewt_reheating_setting_(1); }
  optional<float> get_ewt_speed_up_percentage_value() const { return ewt_reheating_setting_(2); }

  void set_ewt_low_temperature_value(float value) { change_ewt_reheating_setting_(0, value); }
  void set_ewt_high_temperature_value(float value) { change_ewt_reheating_setting_(1, value); }
  void set_ewt_speed_up_percentage_value(int value) { change_ewt_reheating_setting_(2, value); }

 protected:

  struct QueuedCommand {
    std::vector<uint8_t> bytes;
    // Response command to wait for, or 0 when the ComfoD only sends an ACK.
    uint8_t response;
  };

  static int fan_mode_to_level_(climate::ClimateFanMode fan_mode) {
    switch (fan_mode) {
      case climate::CLIMATE_FAN_HIGH:
        return 0x04;
      case climate::CLIMATE_FAN_MEDIUM:
        return 0x03;
      case climate::CLIMATE_FAN_LOW:
        return 0x02;
      case climate::CLIMATE_FAN_OFF:
        return 0x01;
      case climate::CLIMATE_FAN_AUTO:
        return 0x00;
      default:
        return -1;
    }
  }

  void apply_level_(uint8_t level) {
    switch (level) {
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
  }

  void set_level_(int level) {
    if (level < 0 || level > 4) {
      ESP_LOGI(TAG, "Ignoring invalid level request: %i", level);
      return;
    }

    ESP_LOGI(TAG, "Setting level to: %i", level);
    uint8_t command[COMFOD_SET_LEVEL_LENGTH] = {(uint8_t) level};
    send_command_(COMFOD_SET_LEVEL_REQUEST, command, sizeof(command));
    request_(COMFOD_GET_VENTILATION_LEVEL_REQUEST);
  }

  void set_comfort_temperature_(float temperature) {
    if (temperature < COMFOD_MIN_SUPPORTED_TEMP || temperature > COMFOD_MAX_SUPPORTED_TEMP) {
      ESP_LOGI(TAG, "Ignoring invalid temperature request: %.1f", temperature);
      return;
    }

    ESP_LOGI(TAG, "Setting temperature to: %.1f", temperature);

    uint8_t command[COMFOD_SET_COMFORT_TEMPERATURE_LENGTH] = {(uint8_t) ((temperature + 20.0f) * 2.0f)};
    send_command_(COMFOD_SET_COMFORT_TEMPERATURE_REQUEST, command, sizeof(command));
  }

  optional<float> ewt_reheating_setting_(uint8_t index) const {
    if (!has_ewt_reheating_)
      return {};
    return (float) ewt_reheating_[index];
  }

  // index into the 0xEC response: 0 = EWT low, 1 = EWT high, 2 = EWT speed up %
  void change_ewt_reheating_setting_(uint8_t index, float value) {
    if (!has_ewt_reheating_) {
      ESP_LOGW(TAG, "EWT/Reheating settings not read from the ComfoD yet, ignoring the change.");
      return;
    }
    uint8_t settings[COMFOD_GET_EWT_REHEATER_LENGTH];
    memcpy(settings, ewt_reheating_, sizeof(settings));
    settings[index] = (uint8_t) std::lround(value);
    configure_ewt_reheating_settings_(settings[0], settings[1], settings[2], settings[3], settings[6]);
  }

  void configure_ewt_reheating_settings_(int low_temperature, int high_temperature, int ewt_speed_up_percentage, int kitchen_hood_speed_up_percentage, int reheating_target_temperature) {
    if (low_temperature < 0 || low_temperature > 15) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. Low temperature: %i", low_temperature);
      return;
    }

    if (high_temperature < 10 || high_temperature > 25) {
      ESP_LOGI(TAG, "Ignoring invalid EWT/Reheating configuration request. High temperature: %i", high_temperature);
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

    ESP_LOGI(TAG, "Configuring EWT & Reheater settings. Low temperature: %i, High temperature: %i, EWT speed up: %i%%, Kitchen hood speed up: %i%%, Reheating target temperature: %i", low_temperature, high_temperature, ewt_speed_up_percentage, kitchen_hood_speed_up_percentage, reheating_target_temperature);
    uint8_t command[COMFOD_SET_EWT_REHEATING_LENGTH] = {(uint8_t) low_temperature, (uint8_t) high_temperature, (uint8_t) ewt_speed_up_percentage, (uint8_t) kitchen_hood_speed_up_percentage, (uint8_t) reheating_target_temperature};
    send_command_(COMFOD_SET_EWT_REHEATING_REQUEST, command, sizeof(command));
    request_(COMFOD_GET_EWT_REHEATER_REQUEST);
  }

  void set_rs232_mode(int mode) {
    if (mode < 0 || mode > 4) {
      ESP_LOGI(TAG, "Ignoring invalid mode request: %i", mode);
      return;
    }

    ESP_LOGI(TAG, "Setting rs232 mode to: %i", mode);
    uint8_t command[COMFOD_SET_RS232_MODE_LENGTH] = {(uint8_t) mode};
    send_command_(COMFOD_SET_RS232_MODE_REQUEST, command, sizeof(command), COMFOD_SET_RS232_MODE_RESPONSE);
  }

  void request_(uint8_t command) {
    send_command_(command, nullptr, 0, command + 1);
  }

  void send_command_(uint8_t command, const uint8_t *command_data, uint8_t command_data_length, uint8_t response = 0) {
    if (tx_queue_.size() >= COMFOD_MAX_QUEUED_COMMANDS) {
      ESP_LOGW(TAG, "Command queue full, dropping command 0x%02X", command);
      return;
    }

    QueuedCommand queued{{COMFOD_MSG_PREFIX, COMFOD_MSG_HEAD, 0x00, command, command_data_length}, response};
    uint8_t checksum = 0xad + command + command_data_length;
    for (uint8_t i = 0; i < command_data_length; i++) {
      queued.bytes.push_back(command_data[i]);
      if (command_data[i] == COMFOD_MSG_PREFIX)
        queued.bytes.push_back(COMFOD_MSG_PREFIX);
      checksum += command_data[i];
    }
    queued.bytes.push_back(checksum);
    queued.bytes.push_back(COMFOD_MSG_PREFIX);
    queued.bytes.push_back(COMFOD_MSG_TAIL);
    tx_queue_.push_back(std::move(queued));
  }

  // The ComfoD handles one request at a time: only send when it is quiet and our previous request was answered.
  void send_next_command_(uint32_t now) {
    if (tx_queue_.empty())
      return;
    if (awaiting_reply_) {
      if (now - last_tx_at_ < COMFOD_REPLY_TIMEOUT_MS)
        return;
      ESP_LOGD(TAG, "No reply to command 0x%02X", awaiting_command_);
      awaiting_reply_ = false;
    }
    if (comfod_reader_.in_frame() || comfosense_reader_.in_frame())
      return;
    if (now - last_bus_activity_at_ < COMFOD_BUS_IDLE_MS)
      return;

    QueuedCommand &next = tx_queue_.front();
    ESP_LOGV(TAG, "Sending command 0x%02X", next.bytes[COMFOD_MSG_IDENTIFIER_IDX]);
    uart_comfod->write_array(next.bytes);
    awaiting_reply_ = true;
    awaiting_command_ = next.bytes[COMFOD_MSG_IDENTIFIER_IDX];
    awaiting_response_ = next.response;
    last_tx_at_ = now;
    last_bus_activity_at_ = now;
    tx_queue_.pop_front();
  }

  void read_comfod_(uint32_t now) {
    uint8_t buffer[64];
    size_t available;
    while ((available = uart_comfod->available()) > 0) {
      size_t length = std::min(available, sizeof(buffer));
      if (!uart_comfod->read_array(buffer, length))
        break;
      last_bus_activity_at_ = now;

      // The ComfoSense listens to the ComfoD directly, as if the ESP were not there.
      if (uart_comfosense != nullptr)
        uart_comfosense->write_array(buffer, length);

      for (size_t i = 0; i < length; i++) {
        auto result = comfod_reader_.feed(buffer[i], now);
        log_reader_problems_(comfod_reader_, "ComfoD");
        if (result == ComfoDFrameReader::ACK_RECEIVED) {
          if (awaiting_reply_ && awaiting_response_ == 0)
            awaiting_reply_ = false;
        } else if (result == ComfoDFrameReader::FRAME_RECEIVED) {
          if (awaiting_reply_ && comfod_reader_.command() == awaiting_response_)
            awaiting_reply_ = false;
          // Without a ComfoSense attached nobody else acknowledges the ComfoD.
          if (uart_comfosense == nullptr)
            uart_comfod->write_array(COMFOD_ACK, sizeof(COMFOD_ACK));
          parse_data_();
        }
      }
    }
  }

  // Forwards complete frames so they never interleave with commands of the ESP.
  void read_comfosense_(uint32_t now) {
    uint8_t buffer[64];
    size_t available;
    while ((available = uart_comfosense->available()) > 0) {
      size_t length = std::min(available, sizeof(buffer));
      if (!uart_comfosense->read_array(buffer, length))
        break;
      last_bus_activity_at_ = now;

      for (size_t i = 0; i < length; i++) {
        auto result = comfosense_reader_.feed(buffer[i], now);
        log_reader_problems_(comfosense_reader_, "ComfoSense");
        if (result != ComfoDFrameReader::NONE) {
          ESP_LOGV(TAG, "Proxying 0x%02X from ComfoSense to ComfoD", result == ComfoDFrameReader::ACK_RECEIVED ? COMFOD_MSG_ACK : comfosense_reader_.command());
          uart_comfod->write_array(comfosense_reader_.raw(), comfosense_reader_.raw_length());
        }
      }
    }
  }

  void log_reader_problems_(ComfoDFrameReader &reader, const char *source) {
    if (reader.take_dropped()) {
      char hex[format_hex_pretty_size(ComfoDFrameReader::MAX_RAW_LENGTH)];
      format_hex_pretty_to(hex, reader.dropped_raw(), reader.dropped_raw_length(), ' ');
      ESP_LOGW(TAG, "Dropped frame from %s (%s): %s", source, reader.dropped_reason(), hex);
    }
    uint32_t noise = reader.take_noise_bytes();
    if (noise > 0)
      ESP_LOGV(TAG, "Ignored %u byte(s) outside a frame from %s", (unsigned) noise, source);
  }

  static float decode_temperature_(uint8_t value) { return (float) value / 2.0f - 20.0f; }

  static void publish_if_changed_(sensor::Sensor *sensor, float value) {
    if (sensor != nullptr && sensor->get_raw_state() != value)
      sensor->publish_state(value);
  }

  static void publish_if_changed_(binary_sensor::BinarySensor *sensor, bool value) {
    if (sensor != nullptr)
      sensor->publish_state(value);
  }

  void parse_data_() {
    status_clear_warning();
    const uint8_t command = comfod_reader_.command();
    const uint8_t length = comfod_reader_.data_length();
    const uint8_t *msg = comfod_reader_.data();

    auto has_length = [&](uint8_t minimum) {
      if (length >= minimum)
        return true;
      ESP_LOGD(TAG, "Ignoring response 0x%02X with %u data bytes, expected %u", command, length, minimum);
      return false;
    };

    switch (command) {
      case COMFOD_GET_BOOTLOADER_VERSION_RESPONSE:
        memcpy(bootloader_version_, msg, std::min<size_t>(length, sizeof(bootloader_version_)));
        break;
      case COMFOD_GET_FIRMWARE_VERSION_RESPONSE:
        memcpy(firmware_version_, msg, std::min<size_t>(length, sizeof(firmware_version_)));
        break;
      case COMFOD_GET_BOARD_VERSION_RESPONSE:
        memcpy(connector_board_version_, msg, std::min<size_t>(length, sizeof(connector_board_version_)));
        break;
      case COMFOD_GET_FAN_STATUS_RESPONSE: {
        if (!has_length(COMFOD_GET_FAN_STATUS_LENGTH))
          break;
        publish_if_changed_(fan_supply_air_percentage, msg[0]);
        publish_if_changed_(fan_exhaust_air_percentage, msg[1]);
        uint16_t supply_period = encode_uint16(msg[2], msg[3]);
        uint16_t exhaust_period = encode_uint16(msg[4], msg[5]);
        publish_if_changed_(fan_speed_supply, supply_period == 0 ? 0.0f : std::round(1875000.0f / supply_period));
        publish_if_changed_(fan_speed_exhaust, exhaust_period == 0 ? 0.0f : std::round(1875000.0f / exhaust_period));
        break;
      }
      case COMFOD_GET_VALVE_STATUS_RESPONSE: {
        if (!has_length(2))
          break;
        publish_if_changed_(is_bypass_valve_open, msg[0] != 0 && msg[0] != 255);
        publish_if_changed_(is_preheating, msg[1] == 1);
        break;
      }
      case COMFOD_GET_BYPASS_CONTROL_RESPONSE: {
        if (!has_length(COMFOD_GET_BYPASS_CONTROL_LENGTH))
          break;
        publish_if_changed_(bypass_factor, msg[2]);
        publish_if_changed_(bypass_step, msg[3]);
        publish_if_changed_(bypass_correction, msg[4]);
        publish_if_changed_(is_summer_mode, msg[6] != 0);
        break;
      }
      case COMFOD_GET_TEMPERATURE_RESPONSE: {
        if (!has_length(COMFOD_GET_TEMPERATURE_LENGTH))
          break;
        publish_if_changed_(outside_air_temperature, decode_temperature_(msg[0]));
        publish_if_changed_(supply_air_temperature, decode_temperature_(msg[1]));
        publish_if_changed_(return_air_temperature, decode_temperature_(msg[2]));
        publish_if_changed_(exhaust_air_temperature, decode_temperature_(msg[3]));
        break;
      }
      case COMFOD_GET_SENSOR_DATA_RESPONSE: {
        if (!has_length(1))
          break;
        publish_if_changed_(enthalpy_temperature, decode_temperature_(msg[0]));
        break;
      }
      case COMFOD_GET_VENTILATION_LEVEL_RESPONSE: {
        if (!has_length(10))
          break;
        ESP_LOGV(TAG, "Level %02x", msg[8]);

        publish_if_changed_(return_air_level, msg[6]);
        publish_if_changed_(supply_air_level, msg[7]);

        auto current_fan_mode = fan_mode;
        auto current_mode = mode;
        apply_level_(msg[8]);
        if (fan_mode != current_fan_mode || mode != current_mode) {
          publish_state();
        }

        // Supply air fan active (1 = active / 0 = inactive)
        publish_if_changed_(is_supply_fan_active, msg[9] == 1);
        break;
      }
      case COMFOD_GET_ERROR_STATE_RESPONSE: {
        if (!has_length(9))
          break;
        publish_if_changed_(is_filter_full, msg[8] != 0);
        break;
      }
      case COMFOD_GET_TEMPERATURES_RESPONSE: {
        if (!has_length(COMFOD_GET_TEMPERATURES_LENGTH))
          break;

        auto current_target_temperature = target_temperature;
        auto current_current_temperature = current_temperature;

        target_temperature = decode_temperature_(msg[0]);
        // T3 / return air is the indoor temperature
        current_temperature = decode_temperature_(msg[3]);

        // Wait for the ventilation level so the first state carries a valid fan mode.
        if (fan_mode.has_value() && (current_target_temperature != target_temperature || current_current_temperature != current_temperature)) {
          publish_state();
        }

        const uint8_t present = msg[5];
        if (present & 0x01)
          publish_if_changed_(outside_air_temperature, decode_temperature_(msg[1]));
        if (present & 0x02)
          publish_if_changed_(supply_air_temperature, decode_temperature_(msg[2]));
        if (present & 0x04)
          publish_if_changed_(return_air_temperature, decode_temperature_(msg[3]));
        if (present & 0x08)
          publish_if_changed_(exhaust_air_temperature, decode_temperature_(msg[4]));
        if (present & 0x10)
          publish_if_changed_(ewt_temperature, decode_temperature_(msg[6]));
        if (present & 0x20)
          publish_if_changed_(reheating_temperature, decode_temperature_(msg[7]));
        if (present & 0x40)
          publish_if_changed_(kitchen_hood_temperature, decode_temperature_(msg[8]));
        break;
      }
      case COMFOD_GET_EWT_REHEATER_RESPONSE: {
        if (!has_length(COMFOD_GET_EWT_REHEATER_LENGTH))
          break;
        memcpy(ewt_reheating_, msg, sizeof(ewt_reheating_));
        has_ewt_reheating_ = true;
        publish_if_changed_(ewt_low_temperature, msg[0]);
        publish_if_changed_(ewt_high_temperature, msg[1]);
        publish_if_changed_(ewt_speed_up_percentage, msg[2]);
        publish_if_changed_(kitchen_hood_speed_up_percentage, msg[3]);
        publish_if_changed_(reheating_target_temperature, msg[6]);
        break;
      }
    }
  }

  void get_fan_status_() {
    if (fan_supply_air_percentage != nullptr || fan_exhaust_air_percentage != nullptr || fan_speed_supply != nullptr || fan_speed_exhaust != nullptr) {
      ESP_LOGV(TAG, "getting fan status");
      request_(COMFOD_GET_FAN_STATUS_REQUEST);
    }
  }

  void get_valve_status_() {
    if (is_bypass_valve_open != nullptr || is_preheating != nullptr) {
      ESP_LOGV(TAG, "getting valve status");
      request_(COMFOD_GET_VALVE_STATUS_REQUEST);
    }
  }

  void get_error_status_() {
    if (is_filter_full != nullptr) {
      ESP_LOGV(TAG, "getting error status");
      request_(COMFOD_GET_ERROR_STATE_REQUEST);
    }
  }

  void get_bypass_control_status_() {
    if (bypass_factor != nullptr || bypass_step != nullptr || bypass_correction != nullptr || is_summer_mode != nullptr) {
      ESP_LOGV(TAG, "getting bypass control");
      request_(COMFOD_GET_BYPASS_CONTROL_REQUEST);
    }
  }

  // Always polled: changing an EWT setting resends the other settings from this response.
  void get_ewt_reheating_status_() {
    ESP_LOGV(TAG, "getting ewt reheating status");
    request_(COMFOD_GET_EWT_REHEATER_REQUEST);
  }

  void get_sensor_data_() {
    if (enthalpy_temperature != nullptr) {
      ESP_LOGV(TAG, "getting sensor data");
      request_(COMFOD_GET_SENSOR_DATA_REQUEST);
    }
  }

  void get_ventilation_level_() {
    ESP_LOGV(TAG, "getting ventilation level");
    request_(COMFOD_GET_VENTILATION_LEVEL_REQUEST);
  }

  void get_temperatures_() {
    ESP_LOGV(TAG, "getting temperatures");
    request_(COMFOD_GET_TEMPERATURES_REQUEST);
  }

  ComfoDFrameReader comfod_reader_;
  ComfoDFrameReader comfosense_reader_;
  std::deque<QueuedCommand> tx_queue_;
  bool awaiting_reply_{false};
  uint8_t awaiting_command_{0};
  uint8_t awaiting_response_{0};
  uint32_t last_tx_at_{0};
  uint32_t last_bus_activity_at_{0};

  int8_t update_counter_{-10};

  uint8_t bootloader_version_[13]{0};
  uint8_t firmware_version_[13]{0};
  uint8_t connector_board_version_[14]{0};
  uint8_t ewt_reheating_[COMFOD_GET_EWT_REHEATER_LENGTH]{0};
  bool has_ewt_reheating_{false};

  uart::UARTComponent *uart_comfod{nullptr};
  uart::UARTComponent *uart_comfosense{nullptr};

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
