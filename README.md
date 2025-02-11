# LED indicators using an RGB LED

This is a [ZMK module](https://zmk.dev/docs/features/modules) containing a simple widget that utilizes a (typically built-in) RGB LED controlled by three separate GPIOs.
It is used to indicate battery level and BLE connection status in a minimalist way.

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

In addition, if desired you can pick one of the following methods (off by default) to indicate the layer:

- enable `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` to show the highest active layer on every layer activation
  using a sequence of N cyan color blinks, where N is the zero-based index of the layer.
- or enable `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` to assign each layer its own color, which will remain on while that layer is active.

In addition, there are keymap behaviors you can use to show the status on demand, see [below](#showing-status-on-demand).

## Installation

To use, first add this module to your `config/west.yml` by adding a new entry to `remotes` and `projects`:

```yaml west.yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: caksoylar  # <-- new entry
      url-base: https://github.com/caksoylar
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-rgbled-widget  # <-- new entry
      remote: caksoylar
      revision: main
  self:
    path: config
```

For more information, including instructions for building locally, check out the ZMK docs on [building with modules](https://zmk.dev/docs/features/modules#building-with-modules).

Then, if you are using one of the boards supported by the [`rgbled_adapter`](boards/shields/rgbled_adapter) shield such as Xiao BLE,
just add the `rgbled_adapter` as an additional shield to your build, e.g. in `build.yaml`:

```yaml build.yaml
---
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_adapter
```

For other keyboards, see the "Adding support" section below.

## Showing status on demand

This module also defines keymap [behaviors](https://zmk.dev/docs/keymaps/behaviors) to let you show battery or connection status on demand:

```dts
#include <behaviors/rgbled_widget.dtsi>  // needed to use the behaviors

/ {
    keymap {
        ...
        some_layer {
            bindings = <
                ...
                &ind_bat  // indicate battery level
                &ind_con  // indicate connectivity status
                ...
            >;
        };
    };
};
```

When you invoke the behavior by pressing the corresponding key (or combo), it will trigger the LED color display.
This will happen on all keyboard parts for split keyboards, so make sure to flash firmware to all parts after enabling.

> [!NOTE]
> The behaviors can be used even when you use split keyboards with different controllers that don't all support the widget.
> Make sure that you use the `rgbled_adapter` shield (or enable `CONFIG_RGBLED_WIDGET`) only for the keyboard parts that support it.

## Configuration

<details>
<summary>Expand to see available configuration options</summary>

| Name                                           | Description                                                                  | Default       |
| ---------------------------------------------- | ---------------------------------------------------------------------------- | ------------- |
| `CONFIG_RGBLED_WIDGET_INTERVAL_MS`             | Minimum wait duration between two blinks in ms                               | 500           |
| `CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS`        | Duration of battery level blink in ms                                        | 2000          |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH`      | High battery level percentage                                                | 80            |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW`       | Low battery level percentage                                                 | 20            |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL`  | Critical battery level percentage, blink periodically if under               | 5             |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH`      | Color for high battery level (above `LEVEL_HIGH`)                            | Green (`2`)   |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM`    | Color for medium battery level (between `LEVEL_LOW` and `LEVEL_HIGH`)        | Yellow (`3`)  |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW`       | Color for low battery level (below `LEVEL_LOW`)                              | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL`  | Color for critical battery level (below `LEVEL_CRITICAL`)                    | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_CONN_BLINK_MS`           | Duration of BLE connection status blink in ms                                | 1000          |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED`    | Color for connected BLE connection status                                    | Blue (`4`)    |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_ADVERTISING`  | Color for advertising BLE connection status                                  | Yellow (`3`)  |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED` | Color for disconnected BLE connection status                                 | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE`       | Indicate highest active layer on each layer change with a sequence of blinks | `n`           |
| `CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS`          | Blink and wait duration for layer indicator                                  | 100           |
| `CONFIG_RGBLED_WIDGET_LAYER_COLOR`             | Color to use for layer indicator                                             | Cyan (`6`)    |
| `CONFIG_RGBLED_WIDGET_LAYER_DEBOUNCE_MS`       | Wait duration after a layer change before showing the highest active layer   | 100           |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS`       | Indicate highest active layer with a constant configurable color per layer   | `n`           |
| `CONFIG_RGBLED_WIDGET_LAYER_0_COLOR`           | Color to use for the base layer                                              | Black (`0`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_1_COLOR`           | Color to use for layer 1                                                     | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_LAYER_2_COLOR`           | Color to use for layer 2                                                     | Green (`2`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_3_COLOR`           | Color to use for layer 3                                                     | Yellow (`3`)  |
| `CONFIG_RGBLED_WIDGET_LAYER_4_COLOR`           | Color to use for layer 4                                                     | Blue (`4`)    |
| `CONFIG_RGBLED_WIDGET_LAYER_5_COLOR`           | Color to use for layer 5                                                     | Magenta (`5`) |
| `CONFIG_RGBLED_WIDGET_LAYER_6_COLOR`           | Color to use for layer 6                                                     | Cyan (`6`)    |
| `CONFIG_RGBLED_WIDGET_LAYER_7_COLOR`           | Color to use for layer 7                                                     | White (`7`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_xx_COLOR`          | Color to use for layer xx (change xx to the layer number to change)          | Black (`0`)   |

Color settings use the following integer values:

| Color        | Value |
| ------------ | ----- |
| Black (none) | `0`   |
| Red          | `1`   |
| Green        | `2`   |
| Yellow       | `3`   |
| Blue         | `4`   |
| Magenta      | `5`   |
| Cyan         | `6`   |
| White        | `7`   |

</details>

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
