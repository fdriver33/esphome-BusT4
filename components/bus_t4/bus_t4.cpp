#include "bus_t4.h"

namespace esphome::bus_t4 {

void BusT4Device::send_cmd(const T4Command cmd) {
  const T4Source dst = T4Source{0x00, 0x03};
  uint8_t message[4] = { OVIEW, RUN, cmd, 0x64 };
  T4Packet packet(dst, parent_->get_address(), DEP, message, sizeof(message));
  write(&packet, 0);
}

} // namespace esphome::bus_t4
