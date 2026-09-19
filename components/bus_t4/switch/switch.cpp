#include "switch.h"

#include "esphome/core/log.h"

namespace esphome::bus_t4 {

static const char *TAG = "bus_t4.switch";
static constexpr uint32_t INITIAL_RETRY_INTERVAL = 2000;
static constexpr uint32_t STATE_REFRESH_INTERVAL = 60000;
static constexpr uint8_t DATA_OFFSET = 12;

void BusT4ConfigSwitch::setup() {
  last_request_ = millis();
}

void BusT4ConfigSwitch::loop() {
  const uint32_t now = millis();
  const uint32_t interval = received_state_ ? STATE_REFRESH_INTERVAL : INITIAL_RETRY_INTERVAL;
  if (now - last_request_ >= interval) {
    request_state_();
  }
}

void BusT4ConfigSwitch::dump_config() {
  LOG_SWITCH("  ", "Bus T4 Config Switch", this);
  ESP_LOGCONFIG(TAG, "  Parameter: 0x%02X", parameter_);
  ESP_LOGCONFIG(TAG, "  State source: controller readback");
}

void BusT4ConfigSwitch::sync_target_() {
  if (cover_ != nullptr) {
    target_address_ = cover_->get_target_address();
  }
}

void BusT4ConfigSwitch::request_state_() {
  if (parent_ == nullptr) return;

  sync_target_();
  send_info_request(FOR_CU, static_cast<T4InfoCommand>(parameter_));
  last_request_ = millis();
}

void BusT4ConfigSwitch::write_state(bool state) {
  if (parent_ == nullptr) return;

  sync_target_();
  ESP_LOGI(TAG, "Setting parameter 0x%02X to %s", parameter_, state ? "ON" : "OFF");
  send_config_set(parameter_, state ? 0x01 : 0x00);

  // Do not publish optimistically. Queue a GET behind the SET and publish only
  // the value actually reported by the controller.
  received_state_ = false;
  request_state_();
}

void BusT4ConfigSwitch::on_packet(const T4Packet &packet) {
  if (packet.header.protocol != DMP || packet.header.from != target_address_) return;
  if (packet.size < 13 || packet.message.device != FOR_CU || packet.message.command != parameter_) return;

  const uint8_t flags = packet.message.dmp.flags;
  const uint8_t status = packet.message.dmp.status;

  // SET acknowledgements use RSP_SET_COMPLETE. State is published only from
  // the GET readback that follows the write.
  if (flags != RSP_GET_COMPLETE && flags != RSP_GET_INCOMPLETE) return;

  if (status != ERR_NONE) {
    ESP_LOGW(TAG, "Readback failed for parameter 0x%02X: status=0x%02X", parameter_, status);
    return;
  }

  const uint8_t payload_len = t4_dmp_payload_len(packet);
  if (payload_len < 1 || packet.size <= DATA_OFFSET) {
    ESP_LOGW(TAG, "Empty readback for parameter 0x%02X", parameter_);
    return;
  }

  const uint8_t value = packet.data[DATA_OFFSET];
  if (value > 1) {
    ESP_LOGW(TAG, "Unexpected boolean value 0x%02X for parameter 0x%02X", value, parameter_);
    return;
  }

  received_state_ = true;
  publish_state(value == 0x01);
  ESP_LOGD(TAG, "Parameter 0x%02X readback: %s", parameter_, value == 0x01 ? "ON" : "OFF");
}

}  // namespace esphome::bus_t4
