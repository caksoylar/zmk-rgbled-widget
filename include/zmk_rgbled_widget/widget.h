#define SHOW_LAYER_CHANGE                                                                          \
    (IS_ENABLED(CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE)) &&                                        \
        (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

// color values as specified by an RGB bitfield
enum led_color_t {
    LED_BLACK,   // 0b000
    LED_RED,     // 0b001
    LED_GREEN,   // 0b010
    LED_YELLOW,  // 0b011
    LED_BLUE,    // 0b100
    LED_MAGENTA, // 0b101
    LED_CYAN,    // 0b110
    LED_WHITE    // 0b111
};

// a blink work item as specified by the color and duration
struct blink_item {
    enum led_color_t color;
    uint16_t duration_ms;
    bool first_item;
    uint16_t sleep_ms;
};

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
void indicate_battery(void);
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
void indicate_connectivity(void);
#endif

#if SHOW_LAYER_CHANGE
void indicate_layer(void);
#endif
