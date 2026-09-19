# Nice Bus-T4 ESPHome Component

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![ESPHome](https://img.shields.io/badge/ESPHome-supported-green.svg)](https://esphome.io/)
[![ESP32](https://img.shields.io/badge/ESP32-supported-brightgreen.svg)](https://www.espressif.com/en/products/socs/esp32)

ESPHome external component for local integration of **Nice gate and garage-door controllers** over the Bus-T4 protocol.

This fork includes hardware-tested support for **Nice MC824H / MCA1R10**, including indexed encoder position, controller-backed configuration switches, and editable controller parameters with GET readback.

For the technical MC824H implementation notes, tested registers, and migration details, see [MC824H.md](MC824H.md).

## Features

- Native Home Assistant integration through ESPHome
- Local control without a cloud dependency
- Open, close, stop, step-by-step, and partial-open commands
- Real-time gate state: opening, closing, stopped, fully open, fully closed
- Encoder-based position when supported, with time-based fallback
- Automatic open/close duration learning and persistence
- Automatic Bus-T4 controller discovery
- Device-specific handling for Walky, Robus, Road 400, MC824H, and other compatible controllers
- OXI receiver discovery and remote-control logging
- Raw Bus-T4 command input for testing and reverse engineering
- Controller-backed configuration switches with real GET readback
- Controller-backed number entities for speed and close timers

## Tested Hardware

### Nice MC824H / MCA1R10

Hardware tested with an MC824H/MCA1R10 controller using the indexed 16-bit encoder protocol.

MC824H-specific behavior implemented by this fork:

- product detection by `MC824H` prefix
- explicit indexed position requests using selector/index `0x01`
- live encoder position from `04/11`
- physical encoder maximum from `04/12`
- programmed closed reference from `04/13`
- programmed open reference from `04/18`
- percentage calculated from the programmed closed/open references, not from the physical encoder maximum
- selector validation on 3-byte position replies
- endpoint protection so the UI stays at 99%/1% while the controller still reports movement
- generic `INF_IO (0xD1)` limit-switch interpretation disabled for MC824H because its layout is controller-specific

The tested installation reported:

```text
04/13 closed reference = 0
04/18 open reference   = 2832
04/12 physical maximum = 2932
```

These values are **not hard-coded**. Every MC824H reads its own values at startup, so another installation can use a different range automatically.

### Other controllers

The original project also contains handling for controllers including:

- Nice Robus
- Nice Walky / WLA1
- Nice Road 400
- Nice Spin and other Bus-T4 controllers

Device-specific behavior is detected from controller information where available.

## Hardware / UART

The Nice BiDi-WiFi module contains an ESP32-WROOM and can be used as the ESPHome Bus-T4 interface.

Typical Bus-T4 UART settings used by this component:

```yaml
uart:
  tx_pin: GPIO21
  rx_pin: GPIO18
  baud_rate: 19200
```

The protocol uses 19200 baud, 8 data bits, no parity, 1 stop bit.

## Quick Start

### External component

To use the current fork directly:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/fdriver33/esphome-BusT4
      ref: main
    components: [bus_t4]
```

For production installations, pin a known commit or release tag when you want completely reproducible builds.

### Minimal configuration

```yaml
uart:
  tx_pin: GPIO21
  rx_pin: GPIO18
  baud_rate: 19200

bus_t4:
  id: bus

cover:
  - platform: bus_t4
    name: "Gate"
    id: gate
    auto_learn_timing: true
    open_duration: 20s
    close_duration: 20s
    position_report_interval: 1s
```

The controller address is normally discovered automatically. It can also be pinned through the cover configuration when required.

## Controller-backed switches

The old examples used optimistic template switches. This fork now provides native `platform: bus_t4` switches that read the actual value from the controller.

Behavior:

1. GET the parameter from the controller.
2. Publish the controller value to Home Assistant.
3. When changed, send SET.
4. Immediately GET the same parameter again.
5. Publish only the readback returned by the controller.
6. Retry every 2 seconds until an initial valid state is received.
7. Refresh every 60 seconds to detect changes made outside Home Assistant.

### MC824H Level 1 switches

| Setting | Register | YAML setting | Hardware status on tested MC824H |
|---|---:|---|---|
| L1 Auto-Close | `0x80` | `auto_close` | GET + SET tested |
| L2 Close after Photo | `0x84` | `photo_close` | GET tested |
| L3 Always Close | `0x88` | `always_close` | GET + SET ON/OFF tested |
| L4 Standby | `0x8C` | `standby` | GET tested |
| Peak | `0x93` | `peak` | controller-dependent; tested MC824H rejected it |
| L6 Pre-Flash | `0x94` | `pre_flash` | GET tested |

Example:

```yaml
switch:
  - platform: bus_t4
    name: "L1 Auto-Close"
    icon: "mdi:timer"
    bus_t4_id: bus
    cover_id: gate
    setting: auto_close

  - platform: bus_t4
    name: "L2 Close after Photo"
    icon: "mdi:camera-timer"
    bus_t4_id: bus
    cover_id: gate
    setting: photo_close

  - platform: bus_t4
    name: "L3 Always Close"
    icon: "mdi:lock"
    bus_t4_id: bus
    cover_id: gate
    setting: always_close

  - platform: bus_t4
    name: "L4 Standby"
    icon: "mdi:power-standby"
    bus_t4_id: bus
    cover_id: gate
    setting: standby

  - platform: bus_t4
    name: "L6 Pre-Flash"
    icon: "mdi:alarm-light"
    bus_t4_id: bus
    cover_id: gate
    setting: pre_flash
```

`peak` is supported by the component register map but is intentionally omitted from the example because the tested MC824H did not accept register `0x93`.

## Controller-backed number entities

The component also provides native number entities. They use the same SET -> GET readback model as the switches.

| Parameter | Register | YAML setting | Range | Step | Hardware tested |
|---|---:|---|---:|---:|---|
| Opening Speed | `0x42` | `opening_speed` | 1-100 % | 1 | GET + SET |
| Closing Speed | `0x43` | `closing_speed` | 1-100 % | 1 | GET + SET |
| Auto-Close Pause Time | `0x81` | `pause_time` | 0-250 s | 5 s | GET + SET |
| Photo-Close Time | `0x85` | `photo_close_time` | 0-250 s | 1 s | GET + SET |
| Always-Close Time | `0x89` | `always_close_time` | 0-250 s | 1 s | GET + SET |

Example:

```yaml
number:
  - platform: bus_t4
    name: "Opening Speed"
    icon: "mdi:speedometer"
    unit_of_measurement: "%"
    mode: slider
    bus_t4_id: bus
    cover_id: gate
    setting: opening_speed

  - platform: bus_t4
    name: "Closing Speed"
    icon: "mdi:speedometer"
    unit_of_measurement: "%"
    mode: slider
    bus_t4_id: bus
    cover_id: gate
    setting: closing_speed

  - platform: bus_t4
    name: "Auto-Close Pause Time"
    icon: "mdi:timer-outline"
    unit_of_measurement: "s"
    mode: box
    bus_t4_id: bus
    cover_id: gate
    setting: pause_time

  - platform: bus_t4
    name: "Photo-Close Time"
    icon: "mdi:camera-timer"
    unit_of_measurement: "s"
    mode: box
    bus_t4_id: bus
    cover_id: gate
    setting: photo_close_time

  - platform: bus_t4
    name: "Always-Close Time"
    icon: "mdi:timer-lock"
    unit_of_measurement: "s"
    mode: box
    bus_t4_id: bus
    cover_id: gate
    setting: always_close_time
```

## MC824H position tracking

MC824H position replies are indexed. The request contains one selector byte and the successful response contains:

```text
[index] [position MSB] [position LSB]
```

On the tested controller, index `0x01` corresponds to the active M2/ENC2 channel.

The important position registers are:

| Register | Meaning | Used for percentage |
|---:|---|---|
| `04/11` | Current/live encoder position | current value |
| `04/12` | Physical maximum encoder position | no; diagnostics only on MC824H |
| `04/13` | Programmed closed reference | 0% reference |
| `04/18` | Programmed open reference | 100% reference |

The normalized position is calculated dynamically:

```text
position = (current - closed_reference) / (open_reference - closed_reference)
```

The result is clamped to 0-100%.

This distinction is important. The physical encoder limit can be beyond the programmed open position. Using `04/12` as 100% would therefore under-report the real gate position.

### Endpoint state handling

The live encoder can reach or slightly pass the programmed endpoint before the MC824H changes its state to fully opened/closed.

While movement is still reported:

- opening at/above 100% is held at 99%
- closing at/below 0% is held at 1%

The final 100% or 0% is published when the controller reports the actual Opened or Closed state.

## Available commands

Use commands from lambdas with `id(gate).send_cmd(COMMAND)`:

| Command | Description |
|---|---|
| `CMD_OPEN` | Open gate |
| `CMD_CLOSE` | Close gate |
| `CMD_STOP` | Stop movement |
| `CMD_STEP` | Step-by-step |
| `CMD_OPEN_PARTIAL_1` | Partial open 1 |
| `CMD_OPEN_PARTIAL_2` | Partial open 2 |
| `CMD_OPEN_PARTIAL_3` | Partial open 3 |

Example:

```yaml
button:
  - platform: template
    name: "Partial Open 1"
    on_press:
      - lambda: id(gate).send_cmd(CMD_OPEN_PARTIAL_1);
```

### Partial opening positions on MC824H

The partial-open command does not contain a percentage. The target positions are stored in the controller.

Observed MC824H registers:

```text
04/1B = Partial Open 1 position
04/1C = Partial Open 2 position
04/1D = Partial Open 3 position
```

If `04/1B` is equal to the programmed full-open position, `CMD_OPEN_PARTIAL_1` will open the gate fully. The current component sends the partial-open commands but does not yet expose `04/1B-04/1D` as editable number entities.

## Security commands

Security commands use the IT4WIFI device identity:

```yaml
lock:
  - platform: template
    name: "Gate Lock"
    optimistic: true
    on_lock:
      - lambda: 'id(gate).send_cmd(CMD_BLOCK, IT4WIFI);'
    on_unlock:
      - lambda: 'id(gate).send_cmd(CMD_RELEASE, IT4WIFI);'
```

Available security commands include:

- `CMD_BLOCK`
- `CMD_RELEASE`
- `CMD_OPEN_AND_BLOCK`
- `CMD_CLOSE_AND_BLOCK`
- `CMD_RELEASE_AND_OPEN`
- `CMD_RELEASE_AND_CLOSE`

## Legacy configuration methods

The original C++ methods are still available for lambdas:

```cpp
id(gate).set_auto_close(true);
id(gate).set_photo_close(true);
id(gate).set_always_close(true);
id(gate).set_standby(true);
id(gate).set_peak_mode(true);
id(gate).set_pre_flash(true);
```

For Home Assistant entities, prefer the native `platform: bus_t4` switches because they read the real controller state instead of being optimistic.

## Raw configuration access

`send_config_set(register, value)` remains available for unsupported/raw controller parameters:

```yaml
button:
  - platform: template
    name: "Example raw SET"
    on_press:
      - lambda: 'id(gate).send_config_set(0x92, 50);'
```

Use raw writes only when the register semantics are known for your controller.

## Raw Bus-T4 command input

For debugging and protocol testing, `send_raw_cmd()` accepts full hex packets. Dots, spaces, and other non-hex separators are stripped.

```yaml
text:
  - platform: template
    name: "Raw Command"
    id: raw_command
    optimistic: true
    mode: text
    on_value:
      then:
        - lambda: |-
            if (!x.empty()) {
              id(gate).send_raw_cmd(x);
            }
```

Examples of accepted formatting:

```text
55.0E.00.03.50.90....
550E00035090....
```

## How position tracking works

The cover uses multiple position strategies:

### Encoder position

When the controller supports a usable position register, encoder data is preferred over time estimation.

For MC824H, the component explicitly requests indexed `04/11` position while moving and during periodic idle refreshes.

### Time-based fallback

If recent encoder data is not available, position is estimated using the learned opening/closing duration.

- complete end-to-end movements can update learned duration
- learned values are persisted in flash
- interrupted movements do not update the learned duration
- encoder data remains the preferred source when fresh

### Periodic refresh

The cover requests controller status every 15 seconds while idle. Encoder position is refreshed as well on controllers where it is supported.

Configuration switch/number entities independently refresh their controller-backed values every 60 seconds.

## Configuration variables

### `bus_t4`

| Variable | Type | Default | Description |
|---|---|---|---|
| `address` | hex | `0x5090` | ESP device address on Bus-T4 |

### `cover` platform

| Variable | Type | Default | Description |
|---|---|---|---|
| `name` | string | required | Entity name |
| `auto_learn_timing` | boolean | `true` | Learn full open/close travel duration |
| `open_duration` | time | `20s` | Initial/fallback opening duration |
| `close_duration` | time | `20s` | Initial/fallback closing duration |
| `position_report_interval` | time | `1s` | Position update interval |
| `force_estimated_position` | boolean | `false` | Ignore encoder position and use timing |
| `controller_address` | hex | discovered | Pin controller address and skip discovery |

### `switch` platform

Required fields in addition to normal ESPHome switch options:

```yaml
bus_t4_id: bus
cover_id: gate
setting: auto_close
```

Supported `setting` values:

```text
auto_close
photo_close
always_close
standby
peak
pre_flash
```

### `number` platform

Required fields in addition to normal ESPHome number options:

```yaml
bus_t4_id: bus
cover_id: gate
setting: opening_speed
```

Supported `setting` values:

```text
opening_speed
closing_speed
pause_time
photo_close_time
always_close_time
```

## Example configuration

See [example.yaml](example.yaml) for a complete configuration including cover, commands, controller-backed switches, number entities, lock, and raw command input.

## Troubleshooting

### MC824H position is always zero

Make sure you are using a build that contains MC824H indexed position support. A plain unindexed `04/11` request is not reliable for selecting the active encoder channel on this controller.

A working startup log should contain something similar to:

```text
Product: MC824H
Mode: MC824H (indexed 16-bit encoder, index 0x01)
Position source: Encoder (primary)
```

### Position does not reach 100%

Check the startup values for both:

```text
Position range: <closed> - <programmed open>
Encoder max: <physical maximum>
```

On MC824H the programmed open point and physical encoder maximum can be different. Percentage must use the programmed open point.

### Configuration entities show no state

The `platform: bus_t4` switch/number entities do not publish an optimistic value. They wait for controller GET readback. Check the logs for messages such as:

```text
Parameter 0x80 readback: ON
Parameter 0x42 readback: 53
```

### BiDi-WiFi flashing

Back up the original firmware before replacing it. Typical flashing uses the board's TX/RX/IO0/EN/3V3/GND test points and an appropriate 3.3 V USB-TTL adapter.

## Credits

- Original Bus-T4 work by [@pruwait](https://github.com/pruwait/Nice_BusT4)
- BiDi-WiFi firmware by [@gashtaan](https://github.com/gashtaan/nice-bidiwifi-firmware)
- Initial ESPHome ESP32 PoC by [@andrein](https://github.com/andrein/esphome-BusT4)
- Upstream ESPHome Bus-T4 component by [@makstech](https://github.com/makstech/esphome-BusT4)

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE).

## Related resources

- [MC824H implementation notes](MC824H.md)
- [Home Assistant Community Discussion](https://community.home-assistant.io/t/nice-app-with-bidi-wifi-gate-automation/606241)
- [Nice BiDi-WiFi Product Page](https://www.niceforyou.com/uk/nicepost/bidi-wifi-new-pocket-programming-interface)
- [ESPHome Documentation](https://esphome.io/)
- [BiDi-WiFi schematic](https://github.com/gashtaan/nice-bidiwifi-firmware/blob/e6bc474c782ba9cd9ef3ada83a24e4970b281ae0/schematics/bidiwifi.pdf)
- [Pinout for 10-pin Bus4T](https://github.com/xdanik/Nice_BusT4/blob/9faa86262692da99e75979662b4f4ac555746ebf/img/connector.jpg)
- [Nice TTPCI](https://www.niceforyou.com/sites/default/files/upload/manuals/IS0326A00MM.pdf)
- [Nice DMBM](https://www.niceforyou.com/sites/default/files/upload/manuals/nice_dmbm_integration_protocol.pdf)
