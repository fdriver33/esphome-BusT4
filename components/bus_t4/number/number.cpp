#include "number.h"

#include "esphome/core/log.h"

namespace esphome::bus_t4 {

static const char *TAG = "bus_t4.number";
static constexpr uint32_t INITIAL_RETRY_INTERVAL = 2000;
static constexpr uint32_t STATE_REFRESH_INTERVAL = 60000;
static constexpr uint8_t DATA_OFFSET = 12;

void BusT4ConfigNumber::setup() {
  last_request_ = millis();
}

void BusT4ConfigNumber::loop() {
  const uint32_t now = millis();
  const uint32_t interval = received_state_ ? STATE_REFRESH_INTERVAL : INITIAL_RETRY_INTERVAL;
  if (now - last_request_ >= interval) {
    request_state_();
  }
}

void BusT4ConfigNumber::dump_config() {
  LOG_NUMBER("  ", "Bus T4 Config Number", this);
  ESP_LOGCONFIG(TAG, "  Parameter: 0x%02X", parameter_);
  ESP_LOGCONFIG(TAG, "  State source: controller readback");
}

void BusT4ConfigNumber::sync_target_() {
  if (cover_ != nullptr) {
    target_address_ = cover_->get_target_address();
  }
}

void BusT4ConfigNumber::request_state_() {
  if (parent_ == nullptr) return;

  sync_target_();
  send_info_request(FOR_CU, static_cast<T4InfoCommand>(parameter_));
  last_request_ = millis();
}

void BusT4ConfigNumber::control(float value) {
  if (parent_ == nullptr) return;

  sync_target_();
  const uint8_t byte_value = static_cast<uint8_t>(value);
  ESP_LOGI(TAG, "Setting parameter 0x%02X to %u", parameter_, byte_value);
  send_config_set(parameter_, byte_value);

  // Publish only the value read back from the controller.
  received_state_ = false;
  request_state_();
}

void BusT4ConfigNumber::on_packet(const T4Packet &packet) {
  if (packet.header.protocol != DMP || packet.header.from != target_address_) return;
  if (packet.size < 13 || packet.message.device != FOR_CU || packet.message.command != parameter_) return;

  const uint8_t flags = packet.message.dmp.flags;
  const uint8_t status = packet.message.dmp.status;

  // SET acknowledgements use RSP_SET_COMPLETE. The entity state comes from GET.
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

  // Match the proven nice-bidiwifi number parser: one-byte registers use the
  // first payload byte; if a controller returns two bytes, use the second.
  const uint8_t value = (payload_len >= 2 && packet.size > DATA_OFFSET + 1)
                            ? packet.data[DATA_OFFSET + 1]
                            : packet.data[DATA_OFFSET];

  received_state_ = true;
  publish_state(static_cast<float>(value));
  ESP_LOGD(TAG, "Parameter 0x%02X readback: %u", parameter_, value);
}

}  // namespace esphome::bus_t4
