#include "cover.h"

namespace esphome {
namespace bus_t4 {

static const char *TAG = "bus_t4.cover";

cover::CoverTraits BusT4Cover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(false);
  traits.set_supports_tilt(false);
  traits.set_supports_toggle(false);
  traits.set_supports_stop(true);
  return traits;
}

void BusT4Cover::control(const cover::CoverCall &call) {
  if (call.get_position() == cover::COVER_OPEN) {
    ESP_LOGD(TAG, "Opening cover");
    send_cmd(T4Command::CMD_OPEN);
    return;
  }

  if (call.get_position() == cover::COVER_CLOSED) {
    ESP_LOGD(TAG, "Closing cover");
    send_cmd(T4Command::CMD_CLOSE);
    return;
  }

  if (call.get_stop()) {
    ESP_LOGD(TAG, "Stopping cover");
    send_cmd(T4Command::CMD_STOP);
    return;
  }
}

} // bus_t4
} // esphome
