# Adapter shield for RGB LED widget

This is a shield definition that defines the necessary aliases to use the
RGB LED widget for certain boards, and automatically enables that feature.

Boards currently supported are:

- Xiao BLE (`seeeduino_xiao_ble`)
- nRF52840 M.2 Module (`nrf52840_m2`)
- nRF52840 MDK USB Dongle (`nrf52840_mdk_usb_dongle`)
- Xiao RP2040 (`seeeduino_xiao_rp2040`[^1])

Please see the [module README](../../../README.md) on general usage instructions, or
to add support to a custom board/shield.

[^1]:
    By default it doesn't do anything for RP2040 since there is no battery
    or BLE support for it. But you can enable one of `CONFIG_RGBLED_WIDGET_SHOW_LAYER`
    or `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` to indicate the layer.
