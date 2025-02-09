#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/keymap.h>
#include <zmk/split/bluetooth/peripheral.h>

#include <zephyr/logging/log.h>

#include <zmk_rgbled_widget/widget.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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

// map from color values to names, for logging
static const char *color_names[] = {"black", "red",     "green", "yellow",
                                    "blue",  "magenta", "cyan",  "white"};

#if SHOW_LAYER_COLORS
static const uint8_t layer_color_idx[] = {
  CONFIG_RGBLED_WIDGET_LAYER_0_COLOR, CONFIG_RGBLED_WIDGET_LAYER_1_COLOR,
  CONFIG_RGBLED_WIDGET_LAYER_2_COLOR, CONFIG_RGBLED_WIDGET_LAYER_3_COLOR,
  CONFIG_RGBLED_WIDGET_LAYER_4_COLOR, CONFIG_RGBLED_WIDGET_LAYER_5_COLOR,
  CONFIG_RGBLED_WIDGET_LAYER_6_COLOR, CONFIG_RGBLED_WIDGET_LAYER_7_COLOR,
  CONFIG_RGBLED_WIDGET_LAYER_8_COLOR, CONFIG_RGBLED_WIDGET_LAYER_9_COLOR,
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
    bool first_item;
    uint16_t sleep_ms;
};

// flag to indicate whether the initial boot up sequence is complete
static bool initialized = false;

// define message queue of blink work items, that will be processed by a
// separate thread
K_MSGQ_DEFINE(led_msgq, sizeof(struct blink_item), 16, 1);

#if IS_ENABLED(CONFIG_ZMK_BLE)
void indicate_connectivity(void) {
    struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_CONN_BLINK_MS};

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
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
#else
    if (zmk_split_bt_peripheral_is_connected()) {
        LOG_CONN_PERIPHERAL("connected", CONNECTED);
        blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED;
    } else {
        LOG_CONN_PERIPHERAL("not connected", DISCONNECTED);
        blink.color = CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED;
    }
#endif

    k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
}

static int led_output_listener_cb(const zmk_event_t *eh) {
    if (initialized) {
        indicate_connectivity();
    }
    return 0;
}

ZMK_LISTENER(led_output_listener, led_output_listener_cb);
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
// run led_output_listener_cb on BLE profile change (on central)
ZMK_SUBSCRIPTION(led_output_listener, zmk_ble_active_profile_changed);
#else
// run led_output_listener_cb on peripheral status change event
ZMK_SUBSCRIPTION(led_output_listener, zmk_split_peripheral_status_changed);
#endif
#endif // IS_ENABLED(CONFIG_ZMK_BLE)

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
void indicate_battery(void) {
    struct blink_item blink = {.duration_ms = CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS,
                               .first_item = true};
    uint8_t battery_level = zmk_battery_state_of_charge();
    int retry = 0;
    while (battery_level == 0 && retry++ < 10) {
        k_sleep(K_MSEC(100));
        battery_level = zmk_battery_state_of_charge();
    };

    if (battery_level == 0) {
        LOG_INF("Battery level undetermined (zero), blinking magenta");
        blink.color = 5;
    } else if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH) {
        LOG_BATTERY(battery_level, HIGH);
        blink.color = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH;
    } else if (battery_level >= CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW) {
        LOG_BATTERY(battery_level, MEDIUM);
        blink.color = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM;
    } else {
        LOG_BATTERY(battery_level, LOW);
        blink.color = CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW;
    }

    k_msgq_put(&led_msgq, &blink, K_NO_WAIT);
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

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#if SHOW_LAYER_COLORS
uint8_t led_current_layer_color = 0;

void update_layer_color(void) {
    uint8_t index = zmk_keymap_highest_layer_active();

    if (led_current_layer_color != layer_color_idx[index]) {
        led_current_layer_color = layer_color_idx[index];
        struct blink_item color = {.duration_ms = 5, .color = led_current_layer_color};
        LOG_INF("Setting layer color to %s for layer %d", color_names[led_current_layer_color], index);
        k_msgq_put(&led_msgq, &color, K_NO_WAIT);
    }
}

static int led_layer_color_listener_cb(const zmk_event_t *eh) {
    ARG_UNUSED(eh);

    if (!initialized) {
      return 0;
    }

    update_layer_color();

    return 0;
}

// run layer_color_listener_cb on layer status change event
ZMK_LISTENER(led_layer_color_listener, led_layer_color_listener_cb);
ZMK_SUBSCRIPTION(led_layer_color_listener, zmk_layer_state_changed);
#endif // SHOW_LAYER_COLORS

void indicate_layer(void) {
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

static void set_rgb_leds(uint8_t from_color, uint8_t to_color) {
    for (uint8_t pos = 0; pos < 3; pos++) {
        uint8_t bit = BIT(pos);
        if ((bit & from_color) != (bit & to_color)) {
            // bits are different, so we need to change one
            if (bit & to_color) {
                led_on(led_dev, rgb_idx[pos]);
            } else {
                led_off(led_dev, rgb_idx[pos]);
            }
        }
    }
}

extern void led_process_thread(void *d0, void *d1, void *d2) {
    ARG_UNUSED(d0);
    ARG_UNUSED(d1);
    ARG_UNUSED(d2);

#if SHOW_LAYER_CHANGE
    k_work_init_delayable(&layer_indicate_work, indicate_layer_cb);
#endif

    uint8_t current_color = 0;

    while (true) {
        // wait until a blink item is received and process it
        struct blink_item blink;
        k_msgq_get(&led_msgq, &blink, K_FOREVER);
        LOG_DBG("Got a blink item from msgq, color %d, duration %d", blink.color,
                blink.duration_ms);

        // turn appropriate LEDs on
        set_rgb_leds(current_color, blink.color);

        // wait for blink duration
        k_sleep(K_MSEC(blink.duration_ms));

#if SHOW_LAYER_COLORS
        current_color = led_current_layer_color;
#endif
        // turn appropriate LEDs off
        set_rgb_leds(blink.color, current_color);

        // wait interval before processing another blink
        if (blink.sleep_ms > 0) {
            k_sleep(K_MSEC(blink.sleep_ms));
        } else {
            k_sleep(K_MSEC(CONFIG_RGBLED_WIDGET_INTERVAL_MS));
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

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    // check and indicate battery level on thread start
    LOG_INF("Indicating initial battery status");

    indicate_battery();

    // wait until blink should be displayed for further checks
    k_sleep(K_MSEC(CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS + CONFIG_RGBLED_WIDGET_INTERVAL_MS));
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)

#if IS_ENABLED(CONFIG_ZMK_BLE)
    // check and indicate current profile or peripheral connectivity status
    LOG_INF("Indicating initial connectivity status");
    indicate_connectivity();
#endif // IS_ENABLED(CONFIG_ZMK_BLE)

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
