#include <river-libinput-config-v1-client-protocol.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "libinput.h"

struct river_libinput_config_v1 *river_libinput_config;

const struct river_libinput_config_v1_listener river_libinput_config_listener =
    {
        .finished = river_libinput_config_handle_finished,
        .libinput_device = river_libinput_config_handle_libinput_device,
};

const struct river_libinput_device_v1_listener river_libinput_device_listener =
    {
        .removed = river_libinput_device_handle_removed,
        .input_device = river_libinput_device_handle_input_device,
        .send_events_support = river_libinput_device_handle_send_events_support,
        .send_events_default = river_libinput_device_handle_send_events_default,
        .send_events_current = river_libinput_device_handle_send_events_current,
        .tap_support = river_libinput_device_handle_tap_support,
        .tap_default = river_libinput_device_handle_tap_default,
        .tap_current = river_libinput_device_handle_tap_current,
        .tap_button_map_default =
            river_libinput_device_handle_tap_button_map_default,
        .tap_button_map_current =
            river_libinput_device_handle_tap_button_map_current,
        .drag_default = river_libinput_device_handle_drag_default,
        .drag_current = river_libinput_device_handle_drag_current,
        .drag_lock_default = river_libinput_device_handle_drag_lock_default,
        .drag_lock_current = river_libinput_device_handle_drag_lock_current,
        .three_finger_drag_support =
            river_libinput_device_handle_three_finger_drag_support,
        .three_finger_drag_default =
            river_libinput_device_handle_three_finger_drag_default,
        .three_finger_drag_current =
            river_libinput_device_handle_three_finger_drag_current,
        .calibration_matrix_support =
            river_libinput_device_handle_calibration_matrix_support,
        .calibration_matrix_default =
            river_libinput_device_handle_calibration_matrix_default,
        .calibration_matrix_current =
            river_libinput_device_handle_calibration_matrix_current,
        .accel_profiles_support =
            river_libinput_device_handle_accel_profiles_support,
        .accel_profile_default =
            river_libinput_device_handle_accel_profile_default,
        .accel_profile_current =
            river_libinput_device_handle_accel_profile_current,
        .accel_speed_default = river_libinput_device_handle_accel_speed_default,
        .accel_speed_current = river_libinput_device_handle_accel_speed_current,
        .natural_scroll_support =
            river_libinput_device_handle_natural_scroll_support,
        .natural_scroll_default =
            river_libinput_device_handle_natural_scroll_default,
        .natural_scroll_current =
            river_libinput_device_handle_natural_scroll_current,
        .left_handed_support = river_libinput_device_handle_left_handed_support,
        .left_handed_default = river_libinput_device_handle_left_handed_default,
        .left_handed_current = river_libinput_device_handle_left_handed_current,
        .click_method_support =
            river_libinput_device_handle_click_method_support,
        .click_method_default =
            river_libinput_device_handle_click_method_default,
        .click_method_current =
            river_libinput_device_handle_click_method_current,
        .clickfinger_button_map_default =
            river_libinput_device_handle_clickfinger_button_map_default,
        .clickfinger_button_map_current =
            river_libinput_device_handle_clickfinger_button_map_current,
        .middle_emulation_support =
            river_libinput_device_handle_middle_emulation_support,
        .middle_emulation_default =
            river_libinput_device_handle_middle_emulation_default,
        .middle_emulation_current =
            river_libinput_device_handle_middle_emulation_current,
        .scroll_method_support =
            river_libinput_device_handle_scroll_method_support,
        .scroll_method_default =
            river_libinput_device_handle_scroll_method_default,
        .scroll_method_current =
            river_libinput_device_handle_scroll_method_current,
        .scroll_button_default =
            river_libinput_device_handle_scroll_button_default,
        .scroll_button_current =
            river_libinput_device_handle_scroll_button_current,
        .scroll_button_lock_default =
            river_libinput_device_handle_scroll_button_lock_default,
        .scroll_button_lock_current =
            river_libinput_device_handle_scroll_button_lock_current,
        .dwt_support = river_libinput_device_handle_dwt_support,
        .dwt_default = river_libinput_device_handle_dwt_default,
        .dwt_current = river_libinput_device_handle_dwt_current,
        .dwtp_support = river_libinput_device_handle_dwtp_support,
        .dwtp_default = river_libinput_device_handle_dwtp_default,
        .dwtp_current = river_libinput_device_handle_dwtp_current,
        .rotation_support = river_libinput_device_handle_rotation_support,
        .rotation_default = river_libinput_device_handle_rotation_default,
        .rotation_current = river_libinput_device_handle_rotation_current,
        .done = river_libinput_device_handle_done,
};

void river_libinput_config_handle_libinput_device(
    void *data, struct river_libinput_config_v1 *config,
    struct river_libinput_device_v1 *device_id) {
    (void)config;

    fprintf(stdout, "river_libinput_config_handle_libinput_device\n");

    river_libinput_device_v1_add_listener(
        device_id, &river_libinput_device_listener, data);
}

void river_libinput_device_handle_accel_profiles_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t profiles) {
    (void)data;
    (void)profiles;

    fprintf(stdout, "river_libinput_device_handle_accel_profiles_support\n");

    if (strcmp(input_config.accel_profile, "flat") == 0) {
        river_libinput_device_v1_set_accel_profile(
            device, RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_FLAT);

    } else if (strcmp(input_config.accel_profile, "adaptive") == 0) {
        river_libinput_device_v1_set_accel_profile(
            device, RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_ADAPTIVE);

    } else if (strcmp(input_config.accel_profile, "none") == 0) {
        river_libinput_device_v1_set_accel_profile(
            device, RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_NONE);

    } else {
        fprintf(stderr, "err: invalid accel profile\n");
        river_libinput_device_v1_set_accel_profile(
            device, RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_NONE);
    }
}
void river_libinput_device_handle_tap_support(
    void *data, struct river_libinput_device_v1 *device, int32_t finger_count) {
    (void)data;

    fprintf(stdout, "river_libinput_device_handle_tap_support\n");

    if (input_config.tap_to_click) {
        river_libinput_device_v1_set_tap(device, 1);
    } else {
        river_libinput_device_v1_set_tap(device, 0);
    }
}

void river_libinput_config_handle_finished(
    void *data, struct river_libinput_config_v1 *config) {}
void river_libinput_device_handle_removed(
    void *data, struct river_libinput_device_v1 *device) {}
void river_libinput_device_handle_input_device(
    void *data, struct river_libinput_device_v1 *device,
    struct river_input_device_v1 *input_device) {}
void river_libinput_device_handle_send_events_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t modes) {}
void river_libinput_device_handle_send_events_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t mode) {}
void river_libinput_device_handle_send_events_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t mode) {}
void river_libinput_device_handle_tap_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_tap_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_tap_button_map_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map) {}
void river_libinput_device_handle_tap_button_map_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map) {}
void river_libinput_device_handle_drag_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_drag_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_drag_lock_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_drag_lock_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_three_finger_drag_support(
    void *data, struct river_libinput_device_v1 *device, int32_t finger_count) {
}
void river_libinput_device_handle_three_finger_drag_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_three_finger_drag_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_calibration_matrix_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_calibration_matrix_default(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *matrix) {}
void river_libinput_device_handle_calibration_matrix_current(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *matrix) {}
void river_libinput_device_handle_accel_profile_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t profile) {}
void river_libinput_device_handle_accel_profile_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t profile) {}
void river_libinput_device_handle_accel_speed_default(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *speed) {}
void river_libinput_device_handle_accel_speed_current(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *speed) {}
void river_libinput_device_handle_natural_scroll_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_natural_scroll_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_natural_scroll_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_left_handed_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_left_handed_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_left_handed_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_click_method_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t methods) {}
void river_libinput_device_handle_click_method_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t method) {}
void river_libinput_device_handle_click_method_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t method) {}
void river_libinput_device_handle_clickfinger_button_map_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map) {}
void river_libinput_device_handle_clickfinger_button_map_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map) {}
void river_libinput_device_handle_middle_emulation_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_middle_emulation_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_middle_emulation_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_scroll_method_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t methods) {}
void river_libinput_device_handle_scroll_method_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t method) {}
void river_libinput_device_handle_scroll_method_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t method) {}
void river_libinput_device_handle_scroll_button_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button) {}
void river_libinput_device_handle_scroll_button_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button) {}
void river_libinput_device_handle_scroll_button_lock_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_scroll_button_lock_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_dwt_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_dwt_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_dwt_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_dwtp_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_dwtp_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_dwtp_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state) {}
void river_libinput_device_handle_rotation_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported) {}
void river_libinput_device_handle_rotation_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t angle) {}
void river_libinput_device_handle_rotation_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t angle) {}
void river_libinput_device_handle_done(
    void *data, struct river_libinput_device_v1 *device) {}
