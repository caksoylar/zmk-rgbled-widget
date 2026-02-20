#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#define SHOW_LAYER_CHANGE                                                                          \
    (IS_ENABLED(CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE)) &&                                        \
        (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#define SHOW_LAYER_COLORS                                                                          \
    (IS_ENABLED(CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS)) &&                                        \
        (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

// Traditional indicator functions (backward compatible)
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
void indicate_battery(void);
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
void indicate_connectivity(void);
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
void indicate_layer(void);
#endif

// Enhanced WS2812 API (available when WS2812 is enabled)
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_WS2812)

// Status type definitions for enhanced API
enum status_type {
    STATUS_BATTERY = 0,
    STATUS_CONNECTIVITY = 1,
    STATUS_LAYER = 2,
    STATUS_CUSTOM = 3
};

// Animation type definitions
enum animation_type {
    ANIM_STATIC = 0,
    ANIM_BLINK = 1,
    ANIM_PULSE = 2,
    ANIM_FADE = 3,
    ANIM_WAVE = 4,
    ANIM_RAINBOW = 5
};

// Animation state structure
struct animation_state {
    enum animation_type type;
    uint32_t duration_ms;
    uint32_t period_ms;
    uint8_t start_color;
    uint8_t end_color;
    bool loop;
};

// Core WS2812 control functions
int ws2812_widget_init(void);
int ws2812_set_led_rgb(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b);
int ws2812_clear_led(uint8_t led_index);
int ws2812_clear_all(void);

// Status indication functions with LED assignment
int ws2812_set_status_led(enum status_type status_type, uint8_t color_idx, 
                         uint16_t duration_ms, bool persistent);
int ws2812_clear_status_led(enum status_type status_type);

// LED sharing functions
int ws2812_set_shared_led(uint8_t primary_led, uint8_t fallback_led, 
                         uint8_t color_idx, uint16_t duration_ms);
int ws2812_return_shared_led(uint8_t led_index);
bool ws2812_can_share_led(uint8_t led_index, uint8_t priority);

// Pattern and animation functions
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_ANIMATIONS)
int ws2812_set_pattern(uint8_t led_index, struct animation_state *pattern);
int ws2812_set_blink_pattern(uint8_t led_index, uint8_t color1, uint8_t color2, uint32_t period_ms);
int ws2812_set_pulse_pattern(uint8_t led_index, uint8_t color, uint32_t period_ms);
int ws2812_set_fade_pattern(uint8_t led_index, uint8_t start_color, uint8_t end_color, uint32_t period_ms);
void ws2812_update_animations(void);
#endif

// Enhanced status indication functions
int ws2812_indicate_battery_enhanced(void);
int ws2812_indicate_connectivity_enhanced(void);
int ws2812_indicate_layer_enhanced(bool use_shared);
int ws2812_indicate_custom_status(uint8_t led_index, uint8_t pattern_id);

// Configuration functions
int ws2812_set_brightness(uint8_t brightness);
int ws2812_get_led_count(void);
int ws2812_set_led_mapping(enum status_type status_type, uint8_t led_index);
int ws2812_get_led_mapping(enum status_type status_type);

// Utility functions
const char* ws2812_get_color_name(uint8_t color_idx);
int ws2812_color_name_to_index(const char* color_name);
bool ws2812_is_enhanced_mode(void);

// LED state query functions
bool ws2812_is_led_shared(uint8_t led_index);
uint8_t ws2812_get_led_color(uint8_t led_index);
enum status_type ws2812_get_led_status_type(uint8_t led_index);
uint8_t ws2812_get_led_priority(uint8_t led_index);

#endif // CONFIG_RGBLED_WIDGET_WS2812

// Color index constants for convenience
#define WS2812_COLOR_BLACK     0
#define WS2812_COLOR_RED       1
#define WS2812_COLOR_GREEN     2
#define WS2812_COLOR_YELLOW    3
#define WS2812_COLOR_BLUE      4
#define WS2812_COLOR_MAGENTA   5
#define WS2812_COLOR_CYAN      6
#define WS2812_COLOR_WHITE     7

// Priority constants
#define WS2812_PRIORITY_CRITICAL    0
#define WS2812_PRIORITY_HIGH        1
#define WS2812_PRIORITY_MEDIUM      2
#define WS2812_PRIORITY_NORMAL      3
#define WS2812_PRIORITY_LOW         4
