# LED indicators using an RGB LED

> [!IMPORTANT]
> This module uses a versioning scheme that is compatible with ZMK versions.
> As a general rule, the `main` branch is targeting compatibility with ZMK's `main`.
>
> **If you have build failures with ZMK's latest release (like `v0.3`) make sure to [use the corresponding revision](#installation) for `zmk-rgbled-widget` in your `west.yml`**.

This is a [ZMK module](https://zmk.dev/docs/features/modules) containing a simple widget that utilizes a (typically built-in) RGB LED controlled by three separate GPIOs.
It is used to indicate battery level and BLE connection status in a minimalist way.

## Features

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

## WS2812 Enhanced Features

This module now supports **WS2812 addressable LEDs** (NeoPixels) as an enhanced alternative to GPIO LEDs, offering significant improvements in functionality and visual feedback.

### 🌟 Key Enhancements with WS2812

- **Multi-LED Support**: Use 1-4 addressable LEDs for simultaneous status display
- **Spatial Status Mapping**: Assign different status types to specific LED positions
- **Advanced Animations**: Static colors, blink patterns, pulse effects, and smooth fade transitions
- **LED Sharing System**: Layer indication can temporarily borrow other LEDs with priority-based arbitration
- **Enhanced Visual Feedback**: Brighter colors, configurable brightness, and sophisticated patterns
- **Backward Compatibility**: All existing GPIO functionality preserved when WS2812 is disabled

### State Change Colors and Behaviors

The following tables document all LED colors, patterns, and trigger conditions for different state changes in WS2812 mode:

#### Battery Status State Changes

| Battery Level | LED Color | Pattern | Trigger Condition | Animation |
|---------------|-----------|---------|-------------------|----------|
| High (>80%) | 🟢 Green | Static | Boot, level change | Solid color |
| Medium (20-80%) | 🟡 Yellow | Static | Boot, level change | Solid color |
| Low (<20%) | 🔴 Red | Blink | Boot, level change | 500ms on/off |
| Critical (<5%) | 🔴 Red | Fast Blink | Continuous warning | 250ms on/off |
| Missing/Disconnected | 🟣 Magenta | Blink | Peripheral disconnect | 1000ms on/off |

#### Connectivity Status State Changes

| Connection State | LED Color | Pattern | Trigger Condition | Animation |
|------------------|-----------|---------|-------------------|----------|
| BLE Connected | 🔵 Blue | Static | Connection established | Solid color |
| BLE Advertising | 🟡 Yellow | Pulse | Searching for device | Breathing effect |
| BLE Disconnected | 🔴 Red | Blink | Connection lost | 1000ms on/off |
| USB Active | 🔵 Cyan | Static | USB priority enabled | Solid color |
| Peripheral Connected | 🔵 Blue | Static | Split keyboard link | Solid color |
| Peripheral Disconnected | 🔴 Red | Blink | Split link lost | 1000ms on/off |

#### Layer Status State Changes

| Layer Change Type | LED Color | Pattern | Trigger Condition | Animation |
|-------------------|-----------|---------|-------------------|----------|
| Layer 0 (Base) | ⚫ Black/Off | Static | Layer deactivation | LED turns off |
| Layer 1 | 🔴 Red | Static/Sequence | Layer activation | Solid or 1 blink |
| Layer 2 | 🟢 Green | Static/Sequence | Layer activation | Solid or 2 blinks |
| Layer 3 | 🟡 Yellow | Static/Sequence | Layer activation | Solid or 3 blinks |
| Layer 4 | 🔵 Blue | Static/Sequence | Layer activation | Solid or 4 blinks |
| Layer 5 | 🟣 Magenta | Static/Sequence | Layer activation | Solid or 5 blinks |
| Layer 6 | 🔵 Cyan | Static/Sequence | Layer activation | Solid or 6 blinks |
| Layer 7 | ⚪ White | Static/Sequence | Layer activation | Solid or 7 blinks |
| Sequence Mode | 🔵 Cyan | N Blinks | N = layer index | Quick succession |
| Shared LED Mode | 🔵 Cyan | Timeout | Temporary display | Returns after 500ms |

### Spatial LED Mapping

With spatial mapping enabled, different status types are assigned to specific LED positions:

```
4-LED Strip Configuration:
┌─────┬─────┬─────┬─────┐
│LED 0│LED 1│LED 2│LED 3│
│🔋   │📶   │📋   │🎯   │
│Batt │Conn │Layer│Custom│
└─────┴─────┴─────┴─────┘

2-LED Strip with Sharing:
┌─────┬─────┐
│LED 0│LED 1│
│🔋📋 │📶📋 │
│Bat+L│Con+L│  (L = Layer sharing)
└─────┴─────┘
```

### LED Sharing System

The LED sharing system provides intelligent LED management with priority-based arbitration:

**Priority Hierarchy** (highest to lowest):
1. **Critical Battery** (🔴 <5%) - Never shareable, highest priority
2. **Connection Changes** - Can interrupt sharing temporarily
3. **Layer Changes** - Can use shared LEDs
4. **Manual Triggers** - Normal priority
5. **Ambient Effects** - Always shareable, lowest priority

**Sharing Behavior**:
- Layer indication can temporarily "borrow" battery or connectivity LEDs
- Shared LEDs automatically return to their primary function after timeout (default: 500ms)
- Critical battery warnings always override sharing
- Configurable timeout and sharing preferences

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
      url: https://github.com/caksoylar/zmk-rgbled-widget
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
  - board: xiao_ble//zmk
    shield: hummingbird rgbled_adapter
```

For other keyboards, see the ["Adding support" section](#adding-support-in-custom-boardsshields) below.

### WS2812 Installation

To use WS2812 addressable LEDs instead of GPIO LEDs, use the `rgbled_ws2812` shield:

```yaml build.yaml
---
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_ws2812
```

Or enable WS2812 support manually in your configuration:

```ini config.conf
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_WS2812=y
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
```

### WS2812 Configuration Examples

#### Basic 2-LED Setup
```conf
# Enable WS2812 widget
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_WS2812=y

# 2 LEDs with spatial mapping
CONFIG_RGBLED_WIDGET_LED_COUNT=2
CONFIG_RGBLED_WIDGET_SPATIAL_MAPPING=y
CONFIG_RGBLED_WIDGET_LED_SHARING=y

# LED assignments
CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX=0
CONFIG_RGBLED_WIDGET_CONN_LED_INDEX=1
CONFIG_RGBLED_WIDGET_LAYER_SHARE_BATTERY=y
CONFIG_RGBLED_WIDGET_LAYER_SHARE_CONN=y

# Appearance
CONFIG_RGBLED_WIDGET_BRIGHTNESS=96
CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS=500
```

#### Advanced 3-LED Setup with Animations
```conf
# Enable enhanced features
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_WS2812=y
CONFIG_RGBLED_WIDGET_LED_COUNT=3
CONFIG_RGBLED_WIDGET_ANIMATIONS=y

# LED mapping (dedicated layer LED)
CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX=0
CONFIG_RGBLED_WIDGET_CONN_LED_INDEX=1
CONFIG_RGBLED_WIDGET_LAYER_LED_INDEX=2

# Layer color indication
CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS=y
CONFIG_RGBLED_WIDGET_LAYER_0_COLOR=0  # Black (base)
CONFIG_RGBLED_WIDGET_LAYER_1_COLOR=1  # Red
CONFIG_RGBLED_WIDGET_LAYER_2_COLOR=2  # Green
CONFIG_RGBLED_WIDGET_LAYER_3_COLOR=4  # Blue

# Brightness and timing
CONFIG_RGBLED_WIDGET_BRIGHTNESS=128
```

#### Device Tree Configuration

Add WS2812 strip definition to your board/shield:

```dts
/ {
    aliases {
        status-ws2812 = &ws2812_strip;
    };
};

&spi1 {
    status = "okay";
    
    ws2812_strip: ws2812@0 {
        compatible = "worldsemi,ws2812-spi";
        reg = <0>;
        spi-max-frequency = <4000000>;
        chain-length = <3>;  /* Number of LEDs */
        color-mapping = <LED_COLOR_ID_GREEN
                        LED_COLOR_ID_RED
                        LED_COLOR_ID_BLUE>;
        reset-delay = <280>;
        status = "okay";
    };
};
```

### Animation Patterns

WS2812 mode supports advanced animation patterns:

#### Static Colors
- **Description**: Solid color display
- **Use Case**: Stable status indication
- **Configuration**: Default pattern for most statuses

#### Blink Patterns
- **Description**: On/off cycles with configurable timing
- **Use Case**: Attention-getting alerts (disconnected, low battery)
- **Configuration**: `pattern-id = <1>` in behaviors

#### Pulse Effects
- **Description**: Breathing/fade patterns with sine wave interpolation
- **Use Case**: BLE advertising, subtle notifications
- **Configuration**: `pattern-id = <2>` in behaviors

#### Fade Transitions
- **Description**: Smooth color transitions between states
- **Use Case**: Layer changes, smooth state transitions
- **Configuration**: `pattern-id = <3>` in behaviors

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

### Enhanced WS2812 Behaviors

With WS2812 support enabled, you gain access to enhanced behaviors with advanced features:

```dts
#include <behaviors/rgbled_widget.dtsi>  // Include for compatibility

/ {
    behaviors {
        // Enhanced battery indication with animations
        batt_ws: battery_ws2812 {
            compatible = "zmk,behavior-ws2812-widget";
            indicate-battery;
        };
        
        // Enhanced connectivity with patterns  
        conn_ws: connectivity_ws2812 {
            compatible = "zmk,behavior-ws2812-widget";
            indicate-connectivity;
        };
        
        // Layer indication with sharing support
        layer_ws: layer_ws2812 {
            compatible = "zmk,behavior-ws2812-widget";
            indicate-layer;
            use-shared;  // Allow borrowing other LEDs
        };
        
        // Manual LED control with patterns
        led_pulse: led_pulse_cyan {
            compatible = "zmk,behavior-ws2812-widget";
            led-index = <2>;
            color = <6>;      // Cyan
            pattern-id = <2>; // Pulse pattern
            duration-ms = <2000>;
        };
        
        // Custom color flash
        led_flash: led_flash_red {
            compatible = "zmk,behavior-ws2812-widget";
            led-index = <1>;
            color = <1>;      // Red
            pattern-id = <1>; // Blink pattern
            duration-ms = <1000>;
        };
    };
    
    keymap {
        compatible = "zmk,keymap";
        
        default_layer {
            bindings = <
                // ... other keys ...
                &batt_ws     // Enhanced battery status
                &conn_ws     // Enhanced connectivity status
                &layer_ws    // Layer status with sharing
                &led_pulse   // Custom pulse effect
                &led_flash   // Custom flash effect
                // ... other keys ...
            >;
        };
    };
};
```

#### Behavior Properties

| Property | Type | Description | Example |
|----------|------|-------------|----------|
| `indicate-battery` | boolean | Show battery status | `indicate-battery;` |
| `indicate-connectivity` | boolean | Show connection status | `indicate-connectivity;` |
| `indicate-layer` | boolean | Show layer status | `indicate-layer;` |
| `led-index` | int | Target LED (0-3) | `led-index = <2>;` |
| `color` | int | Color index (0-7) | `color = <6>;` (cyan) |
| `pattern-id` | int | Animation pattern | `pattern-id = <2>;` (pulse) |
| `duration-ms` | int | Effect duration | `duration-ms = <1500>;` |
| `use-shared` | boolean | Allow LED sharing | `use-shared;` |

#### Pattern IDs

| ID | Pattern | Description | Use Case |
|----|---------|-------------|----------|
| 0 | Static | Solid color | Status indication |
| 1 | Blink | On/off cycles | Alerts, warnings |
| 2 | Pulse | Breathing effect | Advertising, standby |
| 3 | Fade | Color transition | Layer changes |

#### Color Values

| Value | Color | Hex | Use Case |
|-------|-------|-----|----------|
| 0 | Black | #000000 | Off/disabled |
| 1 | Red | #FF0000 | Errors, low battery |
| 2 | Green | #00FF00 | Good status, high battery |
| 3 | Yellow | #FFFF00 | Warnings, medium battery |
| 4 | Blue | #0000FF | Connected, info |
| 5 | Magenta | #FF00FF | Missing, special states |
| 6 | Cyan | #00FFFF | Layer changes, USB |
| 7 | White | #FFFFFF | Highest layers, full brightness |

## Battery levels for splits

For split keyboards, each part will indicate its own battery level with a single battery blink, by default.
However, for some scenarios like keyboards with dongles and no RGBLED widget on the peripherals, you might want the central part to show the battery levels of peripherals too.
This can be done by enabling one of the below settings:

- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS`: Blink for battery level of self and then the peripherals, in order
- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS`: Blink for battery level of only the peripherals, in order

These two settings only apply to split central parts.
The order of blinks for peripherals is determined by the initial pairing order for the split parts.
If a part is currently disconnected, a magenta/purple ([configurable](#configuration-details)) blink will be displayed.

## Migration Guide

### From GPIO LEDs to WS2812

Upgrading from traditional GPIO LEDs to WS2812 addressable LEDs provides significant enhancements while maintaining full backward compatibility.

#### Step 1: Hardware Changes

**Before (GPIO LEDs):**
- Required 3 separate GPIO pins
- Individual red, green, blue LEDs
- Higher power consumption
- Limited to simple on/off control

**After (WS2812):**
- Single GPIO pin (SPI MOSI)
- Addressable LED strip (1-4 LEDs)
- Lower power with brightness control
- Advanced patterns and animations

#### Step 2: Configuration Migration

**Old GPIO Configuration:**
```conf
# Legacy GPIO LED setup
CONFIG_RGBLED_WIDGET=y
CONFIG_LED=y
# GPIO pins defined in device tree
```

**New WS2812 Configuration:**
```conf
# Enhanced WS2812 setup
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_WS2812=y
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y

# Optional enhancements
CONFIG_RGBLED_WIDGET_LED_COUNT=2
CONFIG_RGBLED_WIDGET_SPATIAL_MAPPING=y
CONFIG_RGBLED_WIDGET_ANIMATIONS=y
CONFIG_RGBLED_WIDGET_LED_SHARING=y
CONFIG_RGBLED_WIDGET_BRIGHTNESS=128
```

#### Step 3: Device Tree Changes

**Remove GPIO LED definitions:**
```dts
// DELETE: Old GPIO LED configuration
/ {
    aliases {
        led-red = &red_led;      // Remove
        led-green = &green_led;  // Remove  
        led-blue = &blue_led;    // Remove
    };
    
    leds {
        compatible = "gpio-leds";  // Remove entire block
        red_led: led_0 {
            gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
        };
        // ... other GPIO LEDs
    };
};
```

**Add WS2812 strip definition:**
```dts
// ADD: New WS2812 configuration
/ {
    chosen {
        widget_rgb = &ws2812_strip;
    };
    
    aliases {
        status-ws2812 = &ws2812_strip;
    };
};

&spi1 {
    status = "okay";
    
    ws2812_strip: ws2812@0 {
        compatible = "worldsemi,ws2812-spi";
        reg = <0>;
        spi-max-frequency = <4000000>;
        chain-length = <2>;  /* Adjust LED count */
        color-mapping = <LED_COLOR_ID_GREEN
                        LED_COLOR_ID_RED
                        LED_COLOR_ID_BLUE>;
        reset-delay = <280>;
        status = "okay";
    };
};
```

#### Step 4: Shield Selection

**Before:**
```yaml
# Old shield usage
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_adapter
```

**After:**
```yaml
# New WS2812 shield
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_ws2812
```

#### Step 5: Verification

After migration, verify the enhanced features work:

1. **Multiple LEDs**: Different statuses should display on different LEDs
2. **Animations**: Advertising should show pulse effect, disconnected should blink
3. **Sharing**: Layer changes should temporarily use other LEDs if configured
4. **Brightness**: LEDs should be dimmer and more power-efficient

### Backward Compatibility

The migration is fully reversible:

- **GPIO Mode**: Set `CONFIG_RGBLED_WIDGET_WS2812=n` to return to GPIO LEDs
- **API Compatibility**: All existing `indicate_*()` functions work unchanged
- **Keymap Behaviors**: Original behaviors (`&ind_bat`, `&ind_con`) still function
- **Configuration**: Original settings remain valid for GPIO mode

### Common Migration Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Build errors | Missing SPI config | Add `CONFIG_SPI=y` and related options |
| No LED output | Wrong device tree alias | Ensure `status-ws2812` alias points to strip |
| Dim colors | Low brightness setting | Increase `CONFIG_RGBLED_WIDGET_BRIGHTNESS` |
| Single LED only | No spatial mapping | Enable `CONFIG_RGBLED_WIDGET_SPATIAL_MAPPING=y` |
| No sharing | Sharing disabled | Enable `CONFIG_RGBLED_WIDGET_LED_SHARING=y` |

## Configuration details

<details>
<summary>WS2812-specific settings</summary>

| Name                                    | Description                                                  | Default |
| --------------------------------------- | ------------------------------------------------------------ | ------- |
| `CONFIG_RGBLED_WIDGET_WS2812`           | Enable WS2812 addressable LED support                       | `n`     |
| `CONFIG_RGBLED_WIDGET_LED_COUNT`        | Number of LEDs in strip (1-4)                               | 2       |
| `CONFIG_RGBLED_WIDGET_SPATIAL_MAPPING`  | Enable spatial status mapping to specific LEDs              | `y`     |
| `CONFIG_RGBLED_WIDGET_ANIMATIONS`       | Enable advanced animation patterns                           | `y`     |
| `CONFIG_RGBLED_WIDGET_LED_SHARING`      | Enable LED sharing for multi-function display               | `y`     |
| `CONFIG_RGBLED_WIDGET_BRIGHTNESS`       | Default LED brightness (0-255)                              | 128     |
| `CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS` | LED sharing timeout in milliseconds                         | 500     |

**LED Mapping (when spatial mapping enabled):**

| Name                                      | Description                            | Default |
| ----------------------------------------- | -------------------------------------- | ------- |
| `CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX`  | LED index for battery status          | 0       |
| `CONFIG_RGBLED_WIDGET_CONN_LED_INDEX`     | LED index for connectivity status     | 1       |
| `CONFIG_RGBLED_WIDGET_LAYER_LED_INDEX`    | LED index for layer status            | 2       |

**LED Sharing Options:**

| Name                                         | Description                                    | Default |
| -------------------------------------------- | ---------------------------------------------- | ------- |
| `CONFIG_RGBLED_WIDGET_LAYER_SHARE_BATTERY`   | Allow layer indication to share battery LED   | `y`     |
| `CONFIG_RGBLED_WIDGET_LAYER_SHARE_CONN`      | Allow layer indication to share connectivity LED | `y`   |

</details>

<details>
<summary>General</summary>

| Name                               | Description                                    | Default |
| ---------------------------------- | ---------------------------------------------- | ------- |
| `CONFIG_RGBLED_WIDGET_INTERVAL_MS` | Minimum wait duration between two blinks in ms | 500     |

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
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH=50
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL=10
```

## Adding support in custom boards/shields

### GPIO LEDs (Traditional)

To be able to use this widget with traditional GPIO LEDs, you need three LEDs controlled by GPIOs (_not_ smart LEDs), ideally red, green and blue colors.
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

### WS2812 Addressable LEDs (Enhanced)

For enhanced functionality with WS2812 addressable LEDs, you need a single data line connected to an SPI-capable GPIO pin.

#### Hardware Requirements

- **LED Strip**: WS2812, WS2812B, or SK6812 addressable LEDs (1-4 LEDs recommended)
- **Power**: 5V preferred (3.3V compatible), appropriate current capacity
- **Data Connection**: Single GPIO pin connected to strip's DI (Data In)
- **SPI Interface**: Available SPI peripheral (uses MOSI pin for data)

#### Device Tree Configuration

```dts
/ {
    chosen {
        widget_rgb = &ws2812_strip;
    };
    
    aliases {
        status-ws2812 = &ws2812_strip;
    };
};

&spi1 {  /* Use appropriate SPI instance for your board */
    status = "okay";
    
    ws2812_strip: ws2812@0 {
        compatible = "worldsemi,ws2812-spi";
        reg = <0>;
        spi-max-frequency = <4000000>;
        chain-length = <3>;  /* Number of LEDs in your strip */
        color-mapping = <LED_COLOR_ID_GREEN
                        LED_COLOR_ID_RED
                        LED_COLOR_ID_BLUE>;
        reset-delay = <280>;
        status = "okay";
    };
};
```

#### Configuration

```ini
# Enable WS2812 support
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_WS2812=y

# Required drivers
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y

# Optional enhancements
CONFIG_RGBLED_WIDGET_LED_COUNT=3
CONFIG_RGBLED_WIDGET_SPATIAL_MAPPING=y
CONFIG_RGBLED_WIDGET_ANIMATIONS=y
CONFIG_RGBLED_WIDGET_BRIGHTNESS=128
```

#### Pin Configuration Examples

**Seeeduino Xiao BLE:**
```dts
&spi1 {
    compatible = "nordic,nrf-spim";
    status = "okay";
    sck-pin = <8>;   /* Not used but required */
    mosi-pin = <10>; /* Connect to WS2812 DI */
    miso-pin = <9>;  /* Not used but required */
};
```

**Nice!Nano:**
```dts
&spi1 {
    compatible = "nordic,nrf-spim";
    status = "okay";
    sck-pin = <20>;  /* Not used but required */
    mosi-pin = <6>;  /* Connect to WS2812 DI */
    miso-pin = <8>;  /* Not used but required */
};
```

#### Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No LED output | Wrong alias | Ensure `status-ws2812` alias exists |
| Build errors | Missing drivers | Add all required CONFIG options |
| Flickering | Wrong timing | Check `spi-max-frequency` (try 4MHz) |
| Wrong colors | Incorrect mapping | Adjust `color-mapping` property |
| Dim output | Power/brightness | Check power supply and brightness setting |

#### Power Considerations

- **Current**: ~60mA per LED at full white brightness
- **Voltage**: 5V recommended (brighter), 3.3V compatible (dimmer)
- **Brightness Control**: Use `CONFIG_RGBLED_WIDGET_BRIGHTNESS` to reduce power
- **Strip Length**: 1-4 LEDs recommended for keyboard use
