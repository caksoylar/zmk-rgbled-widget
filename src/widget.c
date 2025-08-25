#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <math.h>

// Define M_PI if not available in embedded environment
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/split/bluetooth/peripheral.h>

#if __has_include(<zmk/split/central.h>)
#include <zmk/split/central.h>
#else
#include <zmk/split/bluetooth/central.h>
#endif

#include <zephyr/logging/log.h>

#include <zmk_rgbled_widget/widget.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Forward declarations for WS2812 functions
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
#include <zephyr/drivers/led_strip.h>

static void color_index_to_rgb(uint8_t color_idx, struct led_rgb *rgb);
static int ws2812_set_led(uint8_t led_index, uint8_t color_idx);
static int ws2812_update_strip(void);
static void ws2812_clear_strip(void);
static int set_led_with_sharing(uint8_t led_index, uint8_t color_idx, uint8_t priority, 
                               bool persistent, uint32_t share_timeout_ms);
static void return_shared_led(uint8_t led_index);
static int set_led_pattern(uint8_t led_index, struct animation_state *pattern);
static void apply_brightness(struct led_rgb *color, uint8_t brightness);
static void rgb_interpolate(struct led_rgb *start, struct led_rgb *end, 
                           float factor, struct led_rgb *result);
static void update_led_animation(uint8_t led_index);

// Enhanced priority system
enum status_priority {
    PRIORITY_CRITICAL_BATTERY = 0,  // Highest - never shareable
    PRIORITY_CONNECTION_CHANGE = 1, // High - can interrupt sharing
    PRIORITY_LAYER_CHANGE = 2,      // Medium - can use shared LEDs
    PRIORITY_MANUAL_TRIGGER = 3,    // Normal
    PRIORITY_AMBIENT = 4            // Lowest - always shareable
};

enum led_sharing_mode {
    SHARE_NONE,           // Dedicated LED only
    SHARE_OVERFLOW,       // Use shared LED when primary unavailable  
    SHARE_ALWAYS,         // Always use shared LED
    SHARE_TEMPORARY       // Temporarily borrow shared LED
};

// LED status mapping structure
struct led_status_mapping {
    uint8_t led_index;
    uint8_t status_type;
    uint8_t priority;
    bool persistent;
    bool shareable;
    uint32_t share_timeout_ms;
    uint8_t fallback_led;
};

// LED state tracking
struct led_state {
    uint8_t current_color;
    uint8_t base_color;           // Original color before sharing
    uint8_t status_type;          // Current status type assigned
    uint8_t priority;             // Current priority level
    bool is_shared;               // Whether LED is currently shared
    bool is_persistent;           // Whether status should persist
    uint32_t share_end_time;      // When to return to base status
    struct animation_state anim;  // Current animation state
};
#endif


#if LED
#define LED_GPIO_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led_red)),
             "An alias for a red LED is not found for RGBLED_WIDGET");
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led_green)),
             "An alias for a green LED is not found for RGBLED_WIDGET");
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led_blue)),
             "An alias for a blue LED is not found for RGBLED_WIDGET");
// GPIO-based LED device and indices of red/green/blue LEDs inside its DT node
static const struct device *led_dev = DEVICE_DT_GET(LED_GPIO_NODE_ID);
static const uint8_t rgb_idx[] = {DT_NODE_CHILD_IDX(DT_ALIAS(led_red)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led_green)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led_blue))};
#endif



BUILD_ASSERT(!(SHOW_LAYER_CHANGE && SHOW_LAYER_COLORS),
             "CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE and CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS "
             "are mutually exclusive");



// map from color values to names, for logging
static const char *color_names[] = {"black", "red",     "green", "yellow",
                                    "blue",  "magenta", "cyan",  "white"};

#if SHOW_LAYER_COLORS
static const uint8_t layer_color_idx[] = {
    CONFIG_RGBLED_WIDGET_LAYER_0_COLOR,  CONFIG_RGBLED_WIDGET_LAYER_1_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_2_COLOR,  CONFIG_RGBLED_WIDGET_LAYER_3_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_4_COLOR,  CONFIG_RGBLED_WIDGET_LAYER_5_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_6_COLOR,  CONFIG_RGBLED_WIDGET_LAYER_7_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_8_COLOR,  CONFIG_RGBLED_WIDGET_LAYER_9_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_10_COLOR, CONFIG_RGBLED_WIDGET_LAYER_11_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_12_COLOR, CONFIG_RGBLED_WIDGET_LAYER_13_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_14_COLOR, CONFIG_RGBLED_WIDGET_LAYER_15_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_16_COLOR, CONFIG_RGBLED_WIDGET_LAYER_17_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_18_COLOR, CONFIG_RGBLED_WIDGET_LAYER_19_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_20_COLOR, CONFIG_RGBLED_WIDGET_LAYER_21_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_22_COLOR, CONFIG_RGBLED_WIDGET_LAYER_23_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_24_COLOR, CONFIG_RGBLED_WIDGET_LAYER_25_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_26_COLOR, CONFIG_RGBLED_WIDGET_LAYER_27_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_28_COLOR, CONFIG_RGBLED_WIDGET_LAYER_29_COLOR,
    CONFIG_RGBLED_WIDGET_LAYER_30_COLOR, CONFIG_RGBLED_WIDGET_LAYER_31_COLOR,
};
#endif

// log shorthands
#define LOG_CONN_CENTRAL(index, status, color_label)                                               \
    LOG_INF("Profile %d %s, blinking %s", index, status,                                           \
            color_names[CONFIG_RGBLED_WIDGET_CONN_COLOR_##color_label])
#define LOG_CONN_PERIPHERAL(status, color_label)                                                   \
    LOG_INF("Peripheral %s, blinking %s", status,                                                  \
            color_names[CONFIG_RGBLED_WIDGET_CONN_COLOR_##color_label])
#define LOG_BATTERY(battery_level, color_label)                                                    \
    LOG_INF("Battery level %d, blinking %s", battery_level,                                        \
            color_names[CONFIG_RGBLED_WIDGET_BATTERY_COLOR_##color_label])

// a blink work item as specified by the color and duration
struct blink_item {
    uint8_t color;
    uint16_t duration_ms;
    uint16_t sleep_ms;
};

// flag to indicate whether the initial boot up sequence is complete
static bool initialized = false;

// track current color for persistent indicators (layer color)
uint8_t led_current_color = 0;

// low-level method to control the LED
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)

#   define STRIP_CHOSEN DT_CHOSEN(widget_rgb)
#   define WS2812_NODE DT_ALIAS(status_ws2812)
#   define RGB_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(worldsemi_ws2812_spi)
/*
Example Device Tree overlay for WS2812 LED strip:

//show use led strip

Replace <YOUR_GPIO_PIN> with the actual GPIO pin number connected to the WS2812 data line.
*/
BUILD_ASSERT(DT_NODE_EXISTS(WS2812_NODE),
             "An alias status-ws2812 for a ws2812 node is not found for RGBLED_WIDGET");

BUILD_ASSERT(DT_NODE_HAS_STATUS(WS2812_NODE, okay),
             "An alias status-ws2812 for a ws2812 node is not found for RGBLED_WIDGET");

static const struct device *ws2812_dev = DEVICE_DT_GET(WS2812_NODE);

// LED count configuration
#ifndef CONFIG_RGBLED_WIDGET_LED_COUNT
#define CONFIG_RGBLED_WIDGET_LED_COUNT 2
#endif

// LED mapping indices
#ifndef CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX
#define CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX 0
#endif
#ifndef CONFIG_RGBLED_WIDGET_CONN_LED_INDEX
#define CONFIG_RGBLED_WIDGET_CONN_LED_INDEX 1
#endif
#ifndef CONFIG_RGBLED_WIDGET_LAYER_LED_INDEX
#define CONFIG_RGBLED_WIDGET_LAYER_LED_INDEX 2
#endif

// LED sharing configuration
#ifndef CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS
#define CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS 500
#endif
#ifndef CONFIG_RGBLED_WIDGET_BRIGHTNESS
#define CONFIG_RGBLED_WIDGET_BRIGHTNESS 128
#endif

// Global LED state array
static struct led_state led_states[CONFIG_RGBLED_WIDGET_LED_COUNT] = {0};
static struct led_rgb led_colors[CONFIG_RGBLED_WIDGET_LED_COUNT] = {0};

// Status Mapper Functions
static uint8_t get_primary_led_for_status(enum status_type status_type) {
    switch (status_type) {
    case STATUS_BATTERY:
        return CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX;
    case STATUS_CONNECTIVITY:
        return CONFIG_RGBLED_WIDGET_CONN_LED_INDEX;
    case STATUS_LAYER:
        return CONFIG_RGBLED_WIDGET_LAYER_LED_INDEX;
    case STATUS_CUSTOM:
    default:
        return CONFIG_RGBLED_WIDGET_LED_COUNT; // Invalid index to trigger sharing
    }
}

static uint8_t get_fallback_led_for_status(enum status_type status_type) {
    switch (status_type) {
    case STATUS_LAYER:
        // Layer can fallback to battery or connectivity LEDs
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_LAYER_SHARE_BATTERY)
        if (CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX < CONFIG_RGBLED_WIDGET_LED_COUNT) {
            return CONFIG_RGBLED_WIDGET_BATTERY_LED_INDEX;
        }
#endif
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_LAYER_SHARE_CONN)
        if (CONFIG_RGBLED_WIDGET_CONN_LED_INDEX < CONFIG_RGBLED_WIDGET_LED_COUNT) {
            return CONFIG_RGBLED_WIDGET_CONN_LED_INDEX;
        }
#endif
        break;
    default:
        break;
    }
    return CONFIG_RGBLED_WIDGET_LED_COUNT; // No fallback available
}

static enum status_priority get_priority_for_status(enum status_type status_type, uint8_t color_idx) {
    switch (status_type) {
    case STATUS_BATTERY:
        // Critical battery gets highest priority
        if (color_idx == CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL) {
            return PRIORITY_CRITICAL_BATTERY;
        }
        return PRIORITY_CONNECTION_CHANGE;
    case STATUS_CONNECTIVITY:
        return PRIORITY_CONNECTION_CHANGE;
    case STATUS_LAYER:
        return PRIORITY_LAYER_CHANGE;
    case STATUS_CUSTOM:
    default:
        return PRIORITY_MANUAL_TRIGGER;
    }
}

static int set_status_led(enum status_type status_type, uint8_t color_idx, 
                         uint16_t duration_ms, bool persistent) {
    uint8_t primary_led = get_primary_led_for_status(status_type);
    uint8_t priority = get_priority_for_status(status_type, color_idx);
    
    // Try primary LED first
    if (primary_led < CONFIG_RGBLED_WIDGET_LED_COUNT) {
        int ret = set_led_with_sharing(primary_led, color_idx, priority, 
                                      persistent, duration_ms);
        if (ret == 0) {
            led_states[primary_led].status_type = status_type;
            LOG_DBG("Set status %d on primary LED %d", status_type, primary_led);
            return 0;
        }
    }
    
    // Try fallback LED if primary failed
    uint8_t fallback_led = get_fallback_led_for_status(status_type);
    if (fallback_led < CONFIG_RGBLED_WIDGET_LED_COUNT) {
        int ret = set_led_with_sharing(fallback_led, color_idx, priority, 
                                      false, CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS);
        if (ret == 0) {
            led_states[fallback_led].status_type = status_type;
            LOG_DBG("Set status %d on fallback LED %d (shared)", status_type, fallback_led);
            return 0;
        }
    }
    
    LOG_WRN("Failed to assign LED for status type %d", status_type);
    return -EBUSY;
}

int ws2812_clear_status_led(enum status_type status_type) {
    // Find LEDs with this status type and clear them
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        if (led_states[i].status_type == status_type) {
            if (led_states[i].is_shared) {
                return_shared_led(i);
            } else {
                ws2812_set_led(i, 0); // Turn off
                led_states[i].status_type = STATUS_CUSTOM;
                led_states[i].priority = PRIORITY_AMBIENT;
            }
        }
    }
    return 0;
}

// Pattern Engine Functions
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_ANIMATIONS)

static void rgb_interpolate(struct led_rgb *start, struct led_rgb *end, 
                           float factor, struct led_rgb *result) {
    result->r = (uint8_t)(start->r + (end->r - start->r) * factor);
    result->g = (uint8_t)(start->g + (end->g - start->g) * factor);
    result->b = (uint8_t)(start->b + (end->b - start->b) * factor);
}

static void apply_brightness(struct led_rgb *color, uint8_t brightness) {
    color->r = (uint8_t)((color->r * brightness) / 255);
    color->g = (uint8_t)((color->g * brightness) / 255);
    color->b = (uint8_t)((color->b * brightness) / 255);
}

static int set_led_pattern(uint8_t led_index, struct animation_state *pattern) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT || !pattern) {
        return -EINVAL;
    }
    
    led_states[led_index].anim = *pattern;
    
    // Apply initial state based on pattern type
    switch (pattern->type) {
    case ANIM_STATIC:
        ws2812_set_led(led_index, pattern->start_color);
        break;
        
    case ANIM_BLINK:
        // Start with the start color
        ws2812_set_led(led_index, pattern->start_color);
        break;
        
    case ANIM_PULSE:
    case ANIM_FADE:
        // Start with dimmed version of start color
        {
            struct led_rgb start_rgb;
            color_index_to_rgb(pattern->start_color, &start_rgb);
            apply_brightness(&start_rgb, CONFIG_RGBLED_WIDGET_BRIGHTNESS / 4);
            led_colors[led_index] = start_rgb;
            ws2812_update_strip();
        }
        break;
        
    case ANIM_WAVE:
    case ANIM_RAINBOW:
        // Not implemented yet - fallback to static
        ws2812_set_led(led_index, pattern->start_color);
        break;
    }
    
    return 0;
}

static void update_led_animation(uint8_t led_index) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        return;
    }
    
    struct led_state *state = &led_states[led_index];
    struct animation_state *anim = &state->anim;
    
    if (anim->type == ANIM_STATIC) {
        return; // Nothing to animate
    }
    
    uint32_t current_time = k_uptime_get_32();
    static uint32_t last_update = 0;
    
    if (last_update == 0) {
        last_update = current_time;
        return;
    }
    
    switch (anim->type) {
    case ANIM_BLINK:
        {
            uint32_t cycle_time = current_time % anim->period_ms;
            uint8_t color = (cycle_time < anim->period_ms / 2) ? 
                           anim->start_color : anim->end_color;
            ws2812_set_led(led_index, color);
        }
        break;
        
    case ANIM_PULSE:
        {
            uint32_t cycle_time = current_time % anim->period_ms;
            float phase = (float)cycle_time / anim->period_ms;
            
            // Create sine wave for smooth pulsing
            float intensity = (sinf(phase * 2 * M_PI) + 1.0f) / 2.0f;
            
            struct led_rgb start_rgb, result_rgb;
            color_index_to_rgb(anim->start_color, &start_rgb);
            
            uint8_t brightness = (uint8_t)(CONFIG_RGBLED_WIDGET_BRIGHTNESS * intensity);
            result_rgb = start_rgb;
            apply_brightness(&result_rgb, brightness);
            
            led_colors[led_index] = result_rgb;
            ws2812_update_strip();
        }
        break;
        
    case ANIM_FADE:
        {
            uint32_t cycle_time = current_time % anim->period_ms;
            float factor = (float)cycle_time / anim->period_ms;
            
            struct led_rgb start_rgb, end_rgb, result_rgb;
            color_index_to_rgb(anim->start_color, &start_rgb);
            color_index_to_rgb(anim->end_color, &end_rgb);
            
            rgb_interpolate(&start_rgb, &end_rgb, factor, &result_rgb);
            apply_brightness(&result_rgb, CONFIG_RGBLED_WIDGET_BRIGHTNESS);
            
            led_colors[led_index] = result_rgb;
            ws2812_update_strip();
        }
        break;
        
    default:
        // Unsupported animation types
        break;
    }
    
    last_update = current_time;
}

static void update_all_animations(void) {
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        update_led_animation(i);
    }
}

// Enhanced status indication with patterns
static int indicate_battery_enhanced(void) {
    uint8_t battery_level = zmk_battery_state_of_charge();
    uint8_t color_idx = 0;
    struct animation_state pattern = {0};
    
    if (battery_level == 0) {
        color_idx = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MISSING;
        pattern.type = ANIM_BLINK;
        pattern.period_ms = 1000;
        pattern.start_color = color_idx;
        pattern.end_color = 0; // Black
    } else if (battery_level <= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL) {
        color_idx = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL;
        pattern.type = ANIM_PULSE;
        pattern.period_ms = 2000;
        pattern.start_color = color_idx;
    } else if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH) {
        color_idx = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH;
        pattern.type = ANIM_STATIC;
        pattern.start_color = color_idx;
    } else if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW) {
        color_idx = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM;
        pattern.type = ANIM_STATIC;
        pattern.start_color = color_idx;
    } else {
        color_idx = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW;
        pattern.type = ANIM_STATIC;
        pattern.start_color = color_idx;
    }
    
    LOG_INF("Enhanced battery indication: level %d%%, color %s, pattern %d", 
            battery_level, color_names[color_idx], pattern.type);
    
    int ret = set_status_led(STATUS_BATTERY, color_idx, 0, true);
    
    // Apply pattern if using spatial mapping
    uint8_t battery_led = get_primary_led_for_status(STATUS_BATTERY);
    if (battery_led < CONFIG_RGBLED_WIDGET_LED_COUNT && pattern.type != ANIM_STATIC) {
        set_led_pattern(battery_led, &pattern);
    }
    
    return ret;
}

static int indicate_connectivity_enhanced(void) {
    uint8_t color_idx = 0;
    struct animation_state pattern = {0};
    pattern.type = ANIM_STATIC;
    
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (zmk_endpoints_selected().transport) {
    case ZMK_TRANSPORT_USB:
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_CONN_SHOW_USB)
        color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_USB;
        LOG_INF("Enhanced USB connection indication");
        break;
#endif
    default: // ZMK_TRANSPORT_BLE
#if IS_ENABLED(CONFIG_ZMK_BLE)
        if (zmk_ble_active_profile_is_connected()) {
            color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED;
            LOG_INF("Enhanced BLE connected indication");
        } else if (zmk_ble_active_profile_is_open()) {
            color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_ADVERTISING;
            pattern.type = ANIM_PULSE;
            pattern.period_ms = 2000;
            pattern.start_color = color_idx;
            LOG_INF("Enhanced BLE advertising indication");
        } else {
            color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED;
            pattern.type = ANIM_BLINK;
            pattern.period_ms = 1000;
            pattern.start_color = color_idx;
            pattern.end_color = 0;
            LOG_INF("Enhanced BLE disconnected indication");
        }
#endif
        break;
    }
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_BLE)
    if (zmk_split_bt_peripheral_is_connected()) {
        color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED;
        LOG_INF("Enhanced peripheral connected indication");
    } else {
        color_idx = CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED;
        pattern.type = ANIM_BLINK;
        pattern.period_ms = 1000;
        pattern.start_color = color_idx;
        pattern.end_color = 0;
        LOG_INF("Enhanced peripheral disconnected indication");
    }
#endif
    
    int ret = set_status_led(STATUS_CONNECTIVITY, color_idx, 0, true);
    
    // Apply pattern if using spatial mapping
    uint8_t conn_led = get_primary_led_for_status(STATUS_CONNECTIVITY);
    if (conn_led < CONFIG_RGBLED_WIDGET_LED_COUNT && pattern.type != ANIM_STATIC) {
        set_led_pattern(conn_led, &pattern);
    }
    
    return ret;
}

static int indicate_layer_enhanced(bool use_shared) {
    uint8_t layer_index = zmk_keymap_highest_layer_active();
    uint8_t color_idx = 0;
    struct animation_state pattern = {0};
    
#if SHOW_LAYER_COLORS
    color_idx = layer_color_idx[layer_index];
    pattern.type = ANIM_FADE;
    pattern.period_ms = 1000;
    pattern.start_color = 0; // Black
    pattern.end_color = color_idx;
#else
    color_idx = CONFIG_RGBLED_WIDGET_LAYER_COLOR;
    pattern.type = ANIM_BLINK;
    pattern.period_ms = CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS * 2;
    pattern.start_color = color_idx;
    pattern.end_color = 0;
#endif
    
    LOG_INF("Enhanced layer indication: layer %d, color %s, use_shared %s", 
            layer_index, color_names[color_idx], use_shared ? "yes" : "no");
    
    // Use shared timeout for layer indication
    uint32_t timeout_ms = use_shared ? CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS : 0;
    
    int ret = set_status_led(STATUS_LAYER, color_idx, timeout_ms, !use_shared);
    
    // Apply pattern for layer transitions
    if (ret == 0 && pattern.type != ANIM_STATIC) {
        // Find which LED was used for layer indication
        for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
            if (led_states[i].status_type == STATUS_LAYER) {
                set_led_pattern(i, &pattern);
                break;
            }
        }
    }
    
    return ret;
}

#else // !CONFIG_RGBLED_WIDGET_ANIMATIONS

// Simplified versions without animations
static int indicate_battery_enhanced(void) {
    uint8_t battery_level = zmk_battery_state_of_charge();
    uint8_t color_idx = get_battery_color(battery_level);
    return set_status_led(STATUS_BATTERY, color_idx, 0, true);
}

static int indicate_connectivity_enhanced(void) {
    // Use existing connectivity logic to determine color
    // This is a simplified version - full implementation would need refactoring
    return set_status_led(STATUS_CONNECTIVITY, CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED, 0, true);
}

static int indicate_layer_enhanced(bool use_shared) {
    uint8_t layer_index = zmk_keymap_highest_layer_active();
    uint8_t color_idx = SHOW_LAYER_COLORS ? layer_color_idx[layer_index] : 
                        CONFIG_RGBLED_WIDGET_LAYER_COLOR;
    uint32_t timeout_ms = use_shared ? CONFIG_RGBLED_WIDGET_SHARE_TIMEOUT_MS : 0;
    return set_status_led(STATUS_LAYER, color_idx, timeout_ms, !use_shared);
}

#endif // CONFIG_RGBLED_WIDGET_ANIMATIONS

// LED Strip Manager Functions
static void ws2812_strip_init(void) {
    if (!device_is_ready(ws2812_dev)) {
        LOG_ERR("WS2812 device not ready");
        return;
    }
    
    // Initialize all LEDs to off
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        led_colors[i] = (struct led_rgb){0, 0, 0};
        led_states[i].current_color = 0;
        led_states[i].base_color = 0;
        led_states[i].status_type = 0;
        led_states[i].priority = PRIORITY_AMBIENT;
        led_states[i].is_shared = false;
        led_states[i].is_persistent = false;
        led_states[i].share_end_time = 0;
    }
    
    led_strip_update_rgb(ws2812_dev, led_colors, CONFIG_RGBLED_WIDGET_LED_COUNT);
    LOG_INF("WS2812 strip initialized with %d LEDs", CONFIG_RGBLED_WIDGET_LED_COUNT);
}

static void color_index_to_rgb(uint8_t color_idx, struct led_rgb *rgb) {
    uint8_t brightness = CONFIG_RGBLED_WIDGET_BRIGHTNESS;
    
    switch (color_idx) {
    case 0: *rgb = (struct led_rgb){0, 0, 0}; break;                                    // black
    case 1: *rgb = (struct led_rgb){brightness, 0, 0}; break;                          // red
    case 2: *rgb = (struct led_rgb){0, brightness, 0}; break;                          // green
    case 3: *rgb = (struct led_rgb){brightness, brightness, 0}; break;                 // yellow
    case 4: *rgb = (struct led_rgb){0, 0, brightness}; break;                          // blue
    case 5: *rgb = (struct led_rgb){brightness, 0, brightness}; break;                 // magenta
    case 6: *rgb = (struct led_rgb){0, brightness, brightness}; break;                 // cyan
    case 7: *rgb = (struct led_rgb){brightness, brightness, brightness}; break;        // white
    default: *rgb = (struct led_rgb){0, 0, 0}; break;
    }
}

static int ws2812_set_led(uint8_t led_index, uint8_t color_idx) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        LOG_ERR("LED index %d out of range (max %d)", led_index, CONFIG_RGBLED_WIDGET_LED_COUNT - 1);
        return -EINVAL;
    }
    
    color_index_to_rgb(color_idx, &led_colors[led_index]);
    led_states[led_index].current_color = color_idx;
    
    return led_strip_update_rgb(ws2812_dev, led_colors, CONFIG_RGBLED_WIDGET_LED_COUNT);
}

int ws2812_clear_led(uint8_t led_index) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        return -EINVAL;
    }
    
    led_colors[led_index] = (struct led_rgb){0, 0, 0};
    led_states[led_index].current_color = 0;
    led_states[led_index].status_type = STATUS_CUSTOM;
    led_states[led_index].priority = PRIORITY_AMBIENT;
    led_states[led_index].is_shared = false;
    led_states[led_index].is_persistent = false;
    led_states[led_index].share_end_time = 0;
    
    return led_strip_update_rgb(ws2812_dev, led_colors, CONFIG_RGBLED_WIDGET_LED_COUNT);
}

static int ws2812_update_strip(void) {
    return led_strip_update_rgb(ws2812_dev, led_colors, CONFIG_RGBLED_WIDGET_LED_COUNT);
}

static void ws2812_clear_strip(void) {
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        led_colors[i] = (struct led_rgb){0, 0, 0};
        led_states[i].current_color = 0;
    }
    ws2812_update_strip();
}

int ws2812_clear_all(void) {
    ws2812_clear_strip();
    // Reset all LED states
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        led_states[i].status_type = STATUS_CUSTOM;
        led_states[i].priority = PRIORITY_AMBIENT;
        led_states[i].is_shared = false;
        led_states[i].is_persistent = false;
        led_states[i].share_end_time = 0;
    }
    return 0;
}

// LED Sharing Management
static bool can_share_led(uint8_t led_index, uint8_t new_priority) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        return false;
    }
    
    struct led_state *state = &led_states[led_index];
    
    // Critical battery status can never be overridden
    if (state->priority == PRIORITY_CRITICAL_BATTERY) {
        return false;
    }
    
    // Higher priority can override lower priority
    if (new_priority < state->priority) {
        return true;
    }
    
    // Same priority can share if LED is marked as shareable
    if (new_priority == state->priority && state->is_shared) {
        return true;
    }
    
    return false;
}

static int set_led_with_sharing(uint8_t led_index, uint8_t color_idx, uint8_t priority, 
                               bool persistent, uint32_t share_timeout_ms) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        return -EINVAL;
    }
    
    struct led_state *state = &led_states[led_index];
    
    // Check if we can use this LED
    if (!can_share_led(led_index, priority)) {
        LOG_DBG("Cannot share LED %d (current priority %d, requested %d)", 
                led_index, state->priority, priority);
        return -EBUSY;
    }
    
    // Save current state if this is the first share
    if (!state->is_shared) {
        state->base_color = state->current_color;
    }
    
    // Update LED state
    state->priority = priority;
    state->is_shared = (share_timeout_ms > 0);
    state->is_persistent = persistent;
    
    if (share_timeout_ms > 0) {
        state->share_end_time = k_uptime_get_32() + share_timeout_ms;
    }
    
    // Set the LED color
    ws2812_set_led(led_index, color_idx);
    
    LOG_DBG("Set LED %d to color %d (priority %d, shared %s)", 
            led_index, color_idx, priority, state->is_shared ? "yes" : "no");
    
    return 0;
}

// Return shared LED to base status
static void return_shared_led(uint8_t led_index) {
    if (led_index >= CONFIG_RGBLED_WIDGET_LED_COUNT) {
        return;
    }
    
    struct led_state *state = &led_states[led_index];
    
    if (state->is_shared) {
        ws2812_set_led(led_index, state->base_color);
        state->is_shared = false;
        state->priority = PRIORITY_AMBIENT;
        state->share_end_time = 0;
        
        LOG_DBG("Returned shared LED %d to base color %d", led_index, state->base_color);
    }
}

// Check for expired shared LEDs
static void check_shared_led_timeouts(void) {
    uint32_t current_time = k_uptime_get_32();
    
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        struct led_state *state = &led_states[i];
        
        if (state->is_shared && state->share_end_time > 0 && 
            current_time >= state->share_end_time) {
            return_shared_led(i);
        }
    }
}

// Enhanced set_rgb_leds function with multi-LED support
static void set_rgb_leds(uint8_t color, uint16_t duration_ms) {
    // For backward compatibility, set all LEDs to the same color when using simple interface
    for (int i = 0; i < CONFIG_RGBLED_WIDGET_LED_COUNT; i++) {
        ws2812_set_led(i, color);
    }    
    if (duration_ms > 0) {
        k_sleep(K_MSEC(duration_ms));
    }
    led_current_color = color;
}

int ws2812_set_status_led(enum status_type status_type, uint8_t color_idx, 
                         uint16_t duration_ms, bool persistent) {
    return set_status_led(status_type, color_idx, duration_ms, persistent);
}

// Public API wrappers for enhanced indication functions
int ws2812_indicate_battery_enhanced(void) {
    return indicate_battery_enhanced();
}

int ws2812_indicate_connectivity_enhanced(void) {
    return indicate_connectivity_enhanced();
}

int ws2812_indicate_layer_enhanced(bool use_shared) {
    return indicate_layer_enhanced(use_shared);
}

// Public API wrappers for LED sharing functions
int ws2812_return_shared_led(uint8_t led_index) {
    return_shared_led(led_index);
    return 0;
}

bool ws2812_can_share_led(uint8_t led_index, uint8_t priority) {
    return can_share_led(led_index, priority);
}

// Public API wrapper for animation updates
void ws2812_update_animations(void) {
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_ANIMATIONS)
    update_all_animations();
#endif
}

#else // !IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812) - use GPIO LEDs

// GPIO LED implementation (backward compatible)
static void set_rgb_leds(uint8_t color, uint16_t duration_ms) {
#if LED
    for (uint8_t pos = 0; pos < 3; pos++) {
        uint8_t bit = BIT(pos);
        if ((bit & led_current_color) != (bit & color)) {
            // bits are different, so we need to change one
            if (bit & color) {
                led_on(led_dev, rgb_idx[pos]);
            } else {
                led_off(led_dev, rgb_idx[pos]);
            }
        }
    }
#endif
    if (duration_ms > 0) {
        k_sleep(K_MSEC(duration_ms));
    }
    led_current_color = color;
}
#endif

// define message queue of blink work items, that will be processed by a
// separate thread
K_MSGQ_DEFINE(led_msgq, sizeof(struct blink_item), 16, 1);

static void indicate_connectivity_internal(void) {
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
    // Use enhanced connectivity indication for WS2812
    indicate_connectivity_enhanced();
    return;
#else
    // Original implementation for GPIO LEDs and simple WS2812
    struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_CONN_BLINK_MS};

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (zmk_endpoints_selected().transport) {
    case ZMK_TRANSPORT_USB:
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_CONN_SHOW_USB)
        LOG_INF("USB connected, blinking %s", color_names[CONFIG_RGBLED_WIDGET_CONN_COLOR_USB]);
        blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_USB;
        break;
#endif
    default: // ZMK_TRANSPORT_BLE
#if IS_ENABLED(CONFIG_ZMK_BLE)
        uint8_t profile_index = zmk_ble_active_profile_index();
        if (zmk_ble_active_profile_is_connected()) {
            LOG_CONN_CENTRAL(profile_index, "connected", CONNECTED);
            blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED;
        } else if (zmk_ble_active_profile_is_open()) {
            LOG_CONN_CENTRAL(profile_index, "open", ADVERTISING);
            blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_ADVERTISING;
        } else {
            LOG_CONN_CENTRAL(profile_index, "not connected", DISCONNECTED);
            blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED;
        }
#endif
        break;
    }
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_BLE)
    if (zmk_split_bt_peripheral_is_connected()) {
        LOG_CONN_PERIPHERAL("connected", CONNECTED);
        blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED;
    } else {
        LOG_CONN_PERIPHERAL("not connected", DISCONNECTED);
        blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED;
    }
#endif

    k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
#endif // Enhanced vs original
}

static int led_output_listener_cb(const zmk_event_t *eh) {
    if (initialized) {
        indicate_connectivity();
    }
    return 0;
}

// debouncing to ignore all but last connectivity event, to prevent repeat blinks
static struct k_work_delayable indicate_connectivity_work;
static void indicate_connectivity_cb(struct k_work *work) { indicate_connectivity_internal(); }
void indicate_connectivity() { k_work_reschedule(&indicate_connectivity_work, K_MSEC(16)); }

ZMK_LISTENER(led_output_listener, led_output_listener_cb);

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
// run led_output_listener_cb on endpoint and BLE profile change (on central)
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_CONN_SHOW_USB)
ZMK_SUBSCRIPTION(led_output_listener, zmk_endpoint_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(led_output_listener, zmk_ble_active_profile_changed);
#endif // IS_ENABLED(CONFIG_ZMK_BLE)
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_BLE)
// run led_output_listener_cb on peripheral status change event
ZMK_SUBSCRIPTION(led_output_listener, zmk_split_peripheral_status_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
static inline uint8_t get_battery_color(uint8_t battery_level) {
    if (battery_level == 0) {
        LOG_INF("Battery level undetermined (zero), blinking %s",
                color_names[CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MISSING]);
        return CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MISSING;
    }
    if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH) {
        LOG_BATTERY(battery_level, HIGH);
        return CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH;
    }
    if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW) {
        LOG_BATTERY(battery_level, MEDIUM);
        return CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM;
    }
    LOG_BATTERY(battery_level, LOW);
    return CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW;
}

void indicate_battery(void) {
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
    // Use enhanced battery indication for WS2812
    indicate_battery_enhanced();
    return;
#else
    // Original implementation for GPIO LEDs and simple WS2812
    struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS};
    int retry = 0;

#if IS_ENABLED(CONFIG_RGBLED_WIDGET_BATTERY_SHOW_SELF) ||                                          \
    IS_ENABLED(CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS)
    uint8_t battery_level = zmk_battery_state_of_charge();
    while (battery_level == 0 && retry++ < 10) {
        k_sleep(K_MSEC(100));
        battery_level = zmk_battery_state_of_charge();
    };

    blink.color = get_battery_color(battery_level);
    k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
#endif

#if IS_ENABLED(CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS) ||                                   \
    IS_ENABLED(CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS)
    for (uint8_t i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        uint8_t peripheral_level;
#if __has_include(<zmk/split/central.h>)
        int ret = zmk_split_central_get_peripheral_battery_level(i, &peripheral_level);
#else
        int ret = zmk_split_get_peripheral_battery_level(i, &peripheral_level);
#endif
        if (ret == 0) {
            retry = 0;
            while (peripheral_level == 0 && retry++ < (CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS +
                                                       CONFIG_RGBLED_WIDGET_INTERVAL_MS) /
                                                          100) {
                k_sleep(K_MSEC(100));
#if __has_include(<zmk/split/central.h>)
                zmk_split_central_get_peripheral_battery_level(i, &peripheral_level);
#else
                zmk_split_get_peripheral_battery_level(i, &peripheral_level);
#endif
            }

            LOG_INF("Got battery level for peripheral %d:", i);
            blink.color = get_battery_color(peripheral_level);
            k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
        } else {
            LOG_ERR("Error looking up battery level for peripheral %d", i);
        }
    }
#endif
#endif // Enhanced vs original
}

static int led_battery_listener_cb(const zmk_event_t *eh) {
    if (!initialized) {
        return 0;
    }

    // check if we are in critical battery levels at state change, blink if we are
    uint8_t battery_level = as_zmk_battery_state_changed(eh)->state_of_charge;

    if (battery_level > 0 && battery_level <= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL) {
        LOG_BATTERY(battery_level, CRITICAL);

        struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS,
                                   .color = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL};
        k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
    }
    return 0;
}

// run led_battery_listener_cb on battery state change event
ZMK_LISTENER(led_battery_listener, led_battery_listener_cb);
ZMK_SUBSCRIPTION(led_battery_listener, zmk_battery_state_changed);
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)

uint8_t led_layer_color = 0;
#if SHOW_LAYER_COLORS
void update_layer_color(void) {
    uint8_t index = zmk_keymap_highest_layer_active();

    if (led_layer_color != layer_color_idx[index]) {
        led_layer_color = layer_color_idx[index];
        
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
        // Use enhanced layer color with persistent display
        set_status_led(STATUS_LAYER, led_layer_color, 0, true);
        LOG_INF("Setting enhanced layer color to %s for layer %d", color_names[led_layer_color], index);
#else
        // Original implementation using message queue
        struct blink_item color = {.color = led_layer_color};
        LOG_INF("Setting layer color to %s for layer %d", color_names[led_layer_color], index);
        k_msgq_put(&led_msgq, &color, K_NO_WAIT);
#endif
    }
}

static int led_layer_color_listener_cb(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);

    // check if this is indeed an activity state changed event
    if (ev != NULL) {
        switch (ev->state) {
        case ZMK_ACTIVITY_SLEEP:
            LOG_INF("Detected sleep activity state, turn off LED");
            set_rgb_leds(0, 0);
            break;
        default: // not handling IDLE and ACTIVE yet
            break;
        }
        return 0;
    }

    // it must be a layer change event instead
    if (initialized) {
        update_layer_color();
    }
    return 0;
}

// run layer_color_listener_cb on layer status change event and activity state event
ZMK_LISTENER(led_layer_color_listener, led_layer_color_listener_cb);
ZMK_SUBSCRIPTION(led_layer_color_listener, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(led_layer_color_listener, zmk_activity_state_changed);
#endif // SHOW_LAYER_COLORS

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
void indicate_layer(void) {
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
    // Use enhanced layer indication with sharing support
    indicate_layer_enhanced(true); // Enable sharing for backward compatibility
    return;
#else
    // Original implementation for GPIO LEDs and simple WS2812
    uint8_t index = zmk_keymap_highest_layer_active();
    static const struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS,
                                            .color = CONFIG_RGBLED_WIDGET_LAYER_COLOR,
                                            .sleep_ms = CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS};
    static const struct blink_item last_blink = {.duration_ms = CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS,
                                                 .color = CONFIG_RGBLED_WIDGET_LAYER_COLOR};
    LOG_INF("Blinking %d times %s for layer change", index,
            color_names[CONFIG_RGBLED_WIDGET_LAYER_COLOR]);

    for (int i = 0; i < index; i++) {
        if (i < index - 1) {
            k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
        } else {
            k_msgq_put(&led_msgq, &last_blink, K_NO_WAIT);
        }
    }
#endif // Enhanced vs original
}
#endif // !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#if SHOW_LAYER_CHANGE
static struct k_work_delayable layer_indicate_work;

static int led_layer_listener_cb(const zmk_event_t *eh) {
    // ignore if not initialized yet or layer off events
    if (initialized && as_zmk_layer_state_changed(eh)->state) {
        k_work_reschedule(&layer_indicate_work, K_MSEC(CONFIG_RGBLED_WIDGET_LAYER_DEBOUNCE_MS));
    }
    return 0;
}

static void indicate_layer_cb(struct k_work *work) { indicate_layer(); }

ZMK_LISTENER(led_layer_listener, led_layer_listener_cb);
ZMK_SUBSCRIPTION(led_layer_listener, zmk_layer_state_changed);
#endif // SHOW_LAYER_CHANGE

extern void led_process_thread(void *d0, void *d1, void *d2) {
    ARG_UNUSED(d0);
    ARG_UNUSED(d1);
    ARG_UNUSED(d2);

    k_work_init_delayable(&indicate_connectivity_work, indicate_connectivity_cb);

#if SHOW_LAYER_CHANGE
    k_work_init_delayable(&layer_indicate_work, indicate_layer_cb);
#endif

    while (true) {
        // wait until a blink item is received and process it
        struct blink_item blink;
        k_msgq_get(&led_msgq, &blink, K_MSEC(50)); // Non-blocking with timeout for animations
        
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
        // Check for expired shared LED timeouts
        check_shared_led_timeouts();
        
        // Update animations
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_ANIMATIONS)
        update_all_animations();
#endif
#endif
        
        if (blink.duration_ms > 0) {
            LOG_DBG("Got a blink item from msgq, color %d, duration %d", blink.color,
                    blink.duration_ms);

            // Blink the leds, using a separation blink if necessary
            if (blink.color == led_current_color && blink.color > 0) {
                set_rgb_leds(0, CONFIG_RGBLED_WIDGET_INTERVAL_MS);
            }
            set_rgb_leds(blink.color, blink.duration_ms);
            if (blink.color == led_layer_color && blink.color > 0) {
                set_rgb_leds(0, CONFIG_RGBLED_WIDGET_INTERVAL_MS);
            }
            // wait interval before processing another blink
            set_rgb_leds(led_layer_color,
                         blink.sleep_ms > 0 ? blink.sleep_ms : CONFIG_RGBLED_WIDGET_INTERVAL_MS);

        } else {
            LOG_DBG("Got a layer color item from msgq, color %d", blink.color);
            set_rgb_leds(blink.color, 0);
        }
    }
}

// define led_process_thread with stack size 1024, start running it 100 ms after
// boot
K_THREAD_DEFINE(led_process_tid, 1024, led_process_thread, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 100);

extern void led_init_thread(void *d0, void *d1, void *d2) {
    ARG_UNUSED(d0);
    ARG_UNUSED(d1);
    ARG_UNUSED(d2);

#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)
    // Initialize WS2812 strip for enhanced mode
    ws2812_strip_init();
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    // check and indicate battery level on thread start
    LOG_INF("Indicating initial battery status");

    indicate_battery();

    // wait until blink should be displayed for further checks
    k_sleep(K_MSEC(CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS + CONFIG_RGBLED_WIDGET_INTERVAL_MS));
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)

    // check and indicate current profile or peripheral connectivity status
    LOG_INF("Indicating initial connectivity status");
    indicate_connectivity();

#if SHOW_LAYER_COLORS
    LOG_INF("Setting initial layer color");
    update_layer_color();
#endif // SHOW_LAYER_COLORS

    initialized = true;
    LOG_INF("Finished initializing LED widget");
}

// run init thread on boot for initial battery+output checks
K_THREAD_DEFINE(led_init_tid, 1024, led_init_thread, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 200);
