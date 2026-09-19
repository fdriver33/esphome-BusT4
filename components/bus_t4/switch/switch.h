#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "../bus_t4.h"
#include "../cover/cover.h"

namespace esphome::bus_t4 {

class BusT4ConfigSwitch final : public switch_::Switch, public BusT4Device, public Component {
 public:
  void set_cover(BusT4Cover *cover) { cover_ = cover; }
  void set_parameter(uint8_t parameter) { parameter_ = parameter; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_packet(const T4Packet &packet) override;

 protected:
  void write_state(bool state) override;

 private:
  void sync_target_();
  void request_state_();

  BusT4Cover *cover_{nullptr};
  uint8_t parameter_{0};
  bool received_state_{false};
  uint32_t last_request_{0};
};

}  // namespace esphome::bus_t4
