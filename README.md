# LED indicators using an RGB LED

This is a [ZMK module](https://zmk.dev/docs/features/modules) containing a simple widget that utilizes a (typically built-in) RGB LED controlled by three separate GPIOs.
It is used to indicate battery level and BLE connection status in a minimalist way.

This repository is a fork of the original [caksoylar/zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget).
The fork used here is [yawatajunk/zmk-rgbled-widget](https://github.com/yawatajunk/zmk-rgbled-widget), which keeps the original functionality and adds PWM LED dimming support for boards using `pwm-leds`.

## Fork and License

- Original project: [caksoylar/zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget)
- Fork used by this setup: [yawatajunk/zmk-rgbled-widget](https://github.com/yawatajunk/zmk-rgbled-widget)
- License: MIT

This fork is distributed under the same MIT license as the original project. When redistributing or modifying this module, keep the original copyright notice and MIT license text included in this repository's [LICENSE](LICENSE).

## Features

- Supports the original GPIO-based RGB LED setup using `gpio-leds`
- Supports PWM-based RGB LED setup using `pwm-leds`
- Adds configurable widget brightness using `CONFIG_RGBLED_WIDGET_BRIGHTNESS`

<details>
  <summary>Short video demo</summary>
  See below video for a short demo, running through power on, profile switching and power offs.

  https://github.com/caksoylar/zmk-rgbled-widget/assets/7876996/cfd89dd1-ff24-4a33-8563-2fdad2a828d4
</details>

### Battery status

- Blink 🟢/🟡/🔴 on boot depending on battery level, with thresholds [set](#configuration-details) by `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH` and `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW`
  - See [options](#battery-levels-for-splits) for showing battery levels for splits
- Blink 🔴 on every battery level change if below critical battery level (`CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL`)

### Connection status

- Blink 🔵 for connected, 🟡 for open (advertising), 🔴 for disconnected profiles on boot after the battery blink, and following every BT profile switch (only on central side for splits)
  - Enable `CONFIG_RGBLED_WIDGET_CONN_SHOW_USB` to blink cyan if USB currently has priority over BLE, instead of above
- Blink 🔵 for connected, 🔴 for disconnected on peripheral side of splits

### Layer state

You can pick one of the following methods (off by default) to indicate the highest active layer:

- Enable `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` to show the highest active layer on every layer activation
  using a sequence of N cyan color blinks, where N is the zero-based index of the layer, or
- Enable `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` to assign each layer its own color, which will remain on while that layer is the highest active layer

These layer indicators will only be active on the central part of a split keyboard, since peripheral parts aren't aware of the layer information.

> [!TIP]
> Also see [below](#showing-status-on-demand) for keymap behaviors you can use to show the battery and connection status on demand.

## Installation

To use, first add this module to your `config/west.yml` by adding a new entry to `projects`:

```yaml west.yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3           # Your ZMK version
      import: app/west.yml
    - name: zmk-rgbled-widget  # <-- new entry
      url: https://github.com/yawatajunk/zmk-rgbled-widget
      revision: v0.3           # MUST match your ZMK version!
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

For other keyboards, see the ["Adding support" section](#adding-support-in-custom-boardsshields) below.

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
> Make sure that you use the `rgbled_adapter` shield (or enable `CONFIG_RGBLED_WIDGET` if not using the adapter) only for the keyboard parts that support it.

## Battery levels for splits

For split keyboards, each part will indicate its own battery level with a single battery blink, by default.
However, for some scenarios like keyboards with dongles and no RGBLED widget on the peripherals, you might want the central part to show the battery levels of peripherals too.
This can be done by enabling one of the below settings:

- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS`: Blink for battery level of self and then the peripherals, in order
- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS`: Blink for battery level of only the peripherals, in order

These two settings only apply to split central parts.
The order of blinks for peripherals is determined by the initial pairing order for the split parts.
If a part is currently disconnected, a magenta/purple ([configurable](#configuration-details)) blink will be displayed.

## Configuration details

<details>
<summary>General</summary>

| Name                               | Description                                    | Default |
| ---------------------------------- | ---------------------------------------------- | ------- |
| `CONFIG_RGBLED_WIDGET_INTERVAL_MS` | Minimum wait duration between two blinks in ms | 500     |
| `CONFIG_RGBLED_WIDGET_BRIGHTNESS`  | Brightness percentage used for `pwm-leds`      | 100     |

</details>

<details>
<summary>Battery-related</summary>

| Name                                          | Description                                                           | Default       |
| --------------------------------------------- | --------------------------------------------------------------------- | ------------- |
| `CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS`       | Duration of battery level blink in ms                                 | 2000          |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH`     | High battery level percentage                                         | 80            |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW`      | Low battery level percentage                                          | 20            |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL` | Critical battery level percentage, blink periodically if under        | 5             |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL` | Critical battery level percentage, blink periodically if under        | 5             |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH`     | Color for high battery level (above `LEVEL_HIGH`)                     | Green (`2`)   |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM`   | Color for medium battery level (between `LEVEL_LOW` and `LEVEL_HIGH`) | Yellow (`3`)  |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW`      | Color for low battery level (below `LEVEL_LOW`)                       | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL` | Color for critical battery level (below `LEVEL_CRITICAL`)             | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MISSING`  | Color for battery not detected, or peripheral disconnected            | Magenta (`5`) |

Only one of the options below can be enabled.
The non-default ones (second and third below) only work on central parts of splits.

| Name                                                 | Description                                             | Default |
| ---------------------------------------------------- | ------------------------------------------------------- | ------- |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_SELF`             | Indicate battery level from self only                   | `n`     |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS`      | On a split central, also show peripheral battery levels | `n`     |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS` | On a split central, show only peripheral battery levels | `n`     |

</details>

<details>
<summary>Connectivity-related</summary>

| Name                                           | Description                                                 | Default      |
| ---------------------------------------------- | ----------------------------------------------------------- | ------------ |
| `CONFIG_RGBLED_WIDGET_CONN_BLINK_MS`           | Duration of BLE connection status blink in ms               | 1000         |
| `CONFIG_RGBLED_WIDGET_CONN_SHOW_USB`           | Show USB indicator instead of BLE status if it has priority | `n`          |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED`    | Color for connected BLE connection status                   | Blue (`4`)   |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_ADVERTISING`  | Color for advertising BLE connection status                 | Yellow (`3`) |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED` | Color for disconnected BLE connection status                | Red (`1`)    |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_USB`          | Color for USB endpoint active                               | Cyan (`6`)   |

</details>

<details>
<summary>Layers-related</summary>

Layer indicator only works on non-splits and central parts of splits.

Below enable and configure the sequence-based layer indicator.

| Name                                     | Description                                                                  | Default    |
| ---------------------------------------- | ---------------------------------------------------------------------------- | ---------- |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` | Indicate highest active layer on each layer change with a sequence of blinks | `n`        |
| `CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS`    | Blink and wait duration for layer indicator                                  | 100        |
| `CONFIG_RGBLED_WIDGET_LAYER_COLOR`       | Color to use for layer indicator                                             | Cyan (`6`) |
| `CONFIG_RGBLED_WIDGET_LAYER_DEBOUNCE_MS` | Wait duration after a layer change before showing the highest active layer   | 100        |

Below enable and configure the color-based layer indicator.

| Name                                     | Description                                                                | Default       |
| ---------------------------------------- | -------------------------------------------------------------------------- | ------------- |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` | Indicate highest active layer with a constant configurable color per layer | `n`           |
| `CONFIG_RGBLED_WIDGET_LAYER_0_COLOR`     | Color to use for the base layer                                            | Black (`0`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_1_COLOR`     | Color to use for layer 1                                                   | Red (`1`)     |
| `CONFIG_RGBLED_WIDGET_LAYER_2_COLOR`     | Color to use for layer 2                                                   | Green (`2`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_3_COLOR`     | Color to use for layer 3                                                   | Yellow (`3`)  |
| `CONFIG_RGBLED_WIDGET_LAYER_4_COLOR`     | Color to use for layer 4                                                   | Blue (`4`)    |
| `CONFIG_RGBLED_WIDGET_LAYER_5_COLOR`     | Color to use for layer 5                                                   | Magenta (`5`) |
| `CONFIG_RGBLED_WIDGET_LAYER_6_COLOR`     | Color to use for layer 6                                                   | Cyan (`6`)    |
| `CONFIG_RGBLED_WIDGET_LAYER_7_COLOR`     | Color to use for layer 7                                                   | White (`7`)   |
| `CONFIG_RGBLED_WIDGET_LAYER_xx_COLOR`    | Color to use for layer xx (change xx to the layer number to change)        | Black (`0`)   |

</details>

<details>
<summary>Mapping for color values</summary>
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
CONFIG_RGBLED_WIDGET_BRIGHTNESS=25
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH=50
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL=10
```

When using the PWM-enabled fork, `CONFIG_RGBLED_WIDGET_BRIGHTNESS` is the main brightness control in your `.conf` file. Typical values are:

- `10` to `20`: very dim
- `25` to `40`: moderate brightness
- `100`: maximum brightness

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

### PWM LED support

This fork also supports PWM-controlled RGB LEDs through `pwm-leds`.
Use this when your board exposes the RGB LED channels through a PWM controller and you want adjustable brightness.

In this setup, the `.conf` file enables the PWM drivers and sets the widget brightness, while the board or shield `.overlay`/`.dtsi` defines the PWM pins and LED nodes.

Example `.conf` settings:

```ini
CONFIG_PWM=y
CONFIG_LED_PWM=y
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_BRIGHTNESS=25
```

Example devicetree setup:

```dts
#include <dt-bindings/pinctrl/nrf-pinctrl.h>
#include <dt-bindings/pwm/pwm.h>

/ {
  aliases {
    led-red = &red_pwm_led;
    led-green = &green_pwm_led;
    led-blue = &blue_pwm_led;
  };
};

&pinctrl {
  pwm0_default: pwm0_default {
    group1 {
      psels = <NRF_PSEL(PWM_OUT0, 0, 26)>, <NRF_PSEL(PWM_OUT1, 0, 30)>,
          <NRF_PSEL(PWM_OUT2, 0, 6)>;
    };
  };

  pwm0_sleep: pwm0_sleep {
    group1 {
      psels = <NRF_PSEL(PWM_OUT0, 0, 26)>, <NRF_PSEL(PWM_OUT1, 0, 30)>,
          <NRF_PSEL(PWM_OUT2, 0, 6)>;
      low-power-enable;
    };
  };
};

&pwm0 {
  status = "okay";
  pinctrl-0 = <&pwm0_default>;
  pinctrl-1 = <&pwm0_sleep>;
  pinctrl-names = "default", "sleep";
};

/ {
  pwm_leds {
    compatible = "pwm-leds";
    status = "okay";

    red_pwm_led: red_pwm_led {
      pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };

    green_pwm_led: green_pwm_led {
      pwms = <&pwm0 1 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };

    blue_pwm_led: blue_pwm_led {
      pwms = <&pwm0 2 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };
  };
};
```

Notes for PWM setups:

- `CONFIG_RGBLED_WIDGET_BRIGHTNESS` controls runtime brightness for all widget indications.
- `CONFIG_PWM=y` and `CONFIG_LED_PWM=y` must be enabled in the `.conf` file.
- The `overlay` or `.dtsi` file must define the PWM pinctrl, enable the PWM controller, and expose `led-red`, `led-green`, and `led-blue` aliases.
- If the RGB LED is wired as active-low, use `PWM_POLARITY_INVERTED`; otherwise use `PWM_POLARITY_NORMAL`.

Finally, turn on the widget in the configuration:

```ini
CONFIG_RGBLED_WIDGET=y
```
