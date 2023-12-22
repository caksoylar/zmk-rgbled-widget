# Adapter shield for RGB LED widget

This is a shield definition that defines the necessary aliases to use the
RGB LED widget for certain boards, and automatically enables that feature.

Boards currently supported are:
- Xiao BLE (`seeeduino_xiao_ble`)
- nRF52840 M.2 Module (`nrf52840_m2`)
- Xiao RP2040 (`seeeduino_xiao_rp2040`[^1])

Please see the [module README](/README.md) on general usage instructions, or
to add support to a custom board/shield.

[^1]: By default it doesn't do anything for RP2040 since there is no battery
or BLE support for it. But you can extend the code in [`leds.c`](/leds.c) to
listen to and display other events like sent keycodes.
