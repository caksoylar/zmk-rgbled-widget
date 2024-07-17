# LED indicators using an RGB LED

This is a ZMK module containing a simple widget that utilizes a (typically built-in) RGB LED controlled by three separate GPIOs.
It is used to indicate battery level and BLE connection status in a minimalist way.

## Usage

To use, first add this module to your `config/west.yml` by adding a new entry to `remotes` and `projects`:

```yaml west.yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: caksoylar
      url-base: https://github.com/caksoylar
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-rgbled-widget
      remote: caksoylar
      revision: main
  self:
    path: config
```

If you are building locally, see the instructions for [building with external modules](https://zmk.dev/docs/development/build-flash#building-with-external-modules)
in ZMK docs.

Then, if you are using one of the boards supported by the [`rgbled_adapter`](/boards/shields/rgbled_adapter) shield such as Xiao BLE,
just add the `rgbled_adapter` as an additional shield to your build, e.g. in `build.yaml`:

```yaml build.yaml
---
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_adapter
```

For other keyboards, see the "Adding support" section below.

## Features

<details>
  <summary>Short video demo</summary>
  See below video for a short demo, running through power on, profile switching and power offs.
  
  https://github.com/caksoylar/zmk-rgbled-widget/assets/7876996/cfd89dd1-ff24-4a33-8563-2fdad2a828d4
</details>

Currently the widget does the following:

- Blink 🟢/🟡/🔴 on boot depending on battery level (for both central/peripherals), thresholds set by `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH` and `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW`
- Blink 🔴 on every battery level change if below critical battery level (`CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL`)
- Blink 🔵 for connected, 🟡 for open (advertising), 🔴 for disconnected profiles on every BT profile switch (on central side for splits)
- Blink 🔵 for connected, 🔴 for disconnected on peripheral side of splits

In addition, you can enable `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` to show the highest active layer on every layer activation
using a sequence of N cyan color blinks, where N is the zero-based index of the layer.

## Configuration

Blink durations can also be adjusted, see the [Kconfig file](Kconfig) for available config properties.
You can add these settings to your keyboard conf file to modify the config values, e.g. in `config/hummingbird.conf`:

```ini
CONFIG_RGBLED_WIDGET_INTERVAL_MS=250
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH=50
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL=10
```

## Adding support in custom boards/shields

To be able to use this widget, you need three LEDs controlled by GPIOs (_not_ smart LEDs), ideally red, green and blue colors.
Once you have these LED definitions in your board/shield, simply set the appropriate `aliases` to the RGB LED node labels.

As an example, here is a definition for three LEDs connected to VCC and separate GPIOs for a nRF52840 controller:

```dts
/ {
    aliases {
        led-red = &led0;
        led-green = &led1;
        led-blue = &led2;
    };

    leds {
        compatible = "gpio-leds";
        status = "okay";
        led0: led_0 {
            gpios = <&gpio0 26 GPIO_ACTIVE_LOW>;  // red LED, connected to P0.26
        };
        led1: led_1 {
            gpios = <&gpio0 30 GPIO_ACTIVE_LOW>;  // green LED, connected to P0.30
        };
        led2: led_2 {
            gpios = <&gpio0 6 GPIO_ACTIVE_LOW>;  // blue LED, connected to P0.06
        };
    };
};
```

(If the LEDs are wired between GPIO and GND instead, use `GPIO_ACTIVE_HIGH` flag.)

Finally, turn on the widget in the configuration:

```ini
CONFIG_RGBLED_WIDGET=y
```
