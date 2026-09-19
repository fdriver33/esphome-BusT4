# MC824H PR #18 hardware test

This branch is based directly on the head commit of makstech/esphome-BusT4 PR #18 (`f2a5078cee6591b3c5966555b11662bcefb620c9`).

It is intentionally kept close to the original PR for the first hardware validation on MC824H/MCA1R10.

## ESPHome external component

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/fdriver33/esphome-BusT4
      ref: mc824h-pr18-test
    components: [bus_t4]
    refresh: Always
```

## What this branch should do on MC824H

The PR detects products starting with `MC824H` and uses DMP index `0x01` for position-related registers.

Expected indexed reads:

- `04/11` current position
- `04/12` physical max encoder position
- `04/13` fully closed encoder position
- `04/18` programmed open position

The three-byte position reply is interpreted as:

```text
[index][MSB][LSB]
```

For example:

```text
01 0B 12 -> index 1, position 0x0B12 = 2834
```

## Test sequence

1. Boot with the gate fully closed.
2. Save the complete initialization log.
3. Check the values reported for `04/13`, `04/18`, and `04/12`.
4. Open the gate fully and save the movement log.
5. Close the gate fully and save the movement log.
6. If possible, stop once around mid-travel and note the reported position.

## Key result we want to confirm

On the previously tested MC824H CD14e installation, indexed `04/11` with index `0x01` tracks the real encoder correctly, while indexed `04/19` is unsupported.

This test is specifically intended to verify whether indexed `04/13` is the correct closed-position register on that controller.

If `04/13 index 1` returns a value close to the settled closed encoder value (previously observed around 5-6), the preferred MC824H scale becomes:

```text
04/13 index 1 -> CLOSED
04/18 index 1 -> OPEN
04/11 index 1 -> CURRENT
```

Do not merge this branch into `main` based on the first test alone. The original PR still lacks some safeguards that can be added after the register behavior is confirmed, including selector validation and integration with newer `main` fixes.
