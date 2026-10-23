#include "libinput.hpp"

#include <string>

#include "config.hpp"
#include "util.hpp"

const river_libinput_device_v1_listener libinputDeviceListener = {
    .removed = noop,
    .input_device = noop,
    .send_events_support = noop,
    .send_events_default = noop,
    .send_events_current = noop,
    .tap_support =
        [](void *, river_libinput_device_v1 *device, int32_t) {
            river_libinput_device_v1_set_tap(
                device, config.input.tapToClick
                            ? RIVER_LIBINPUT_DEVICE_V1_TAP_STATE_ENABLED
                            : RIVER_LIBINPUT_DEVICE_V1_TAP_STATE_DISABLED);
        },
    .tap_default = noop,
    .tap_current = noop,
    .tap_button_map_default = noop,
    .tap_button_map_current = noop,
    .drag_default = noop,
    .drag_current = noop,
    .drag_lock_default = noop,
    .drag_lock_current = noop,
    .three_finger_drag_support = noop,
    .three_finger_drag_default = noop,
    .three_finger_drag_current = noop,
    .calibration_matrix_support = noop,
    .calibration_matrix_default = noop,
    .calibration_matrix_current = noop,
    .accel_profiles_support =
        [](void *, river_libinput_device_v1 *device, uint32_t) {
            const std::string &name = config.input.accelProfile;
            if (name.empty()) {
                return;
            }
            uint32_t profile = RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_NONE;
            if (name == "flat") {
                profile = RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_FLAT;
            } else if (name == "adaptive") {
                profile = RIVER_LIBINPUT_DEVICE_V1_ACCEL_PROFILES_ADAPTIVE;
            } else if (name != "none") {
                diag::error("invalid accel profile");
            }
            river_libinput_device_v1_set_accel_profile(device, profile);
        },
    .accel_profile_default = noop,
    .accel_profile_current = noop,
    .accel_speed_default = noop,
    .accel_speed_current = noop,
    .natural_scroll_support = noop,
    .natural_scroll_default = noop,
    .natural_scroll_current = noop,
    .left_handed_support = noop,
    .left_handed_default = noop,
    .left_handed_current = noop,
    .click_method_support = noop,
    .click_method_default = noop,
    .click_method_current = noop,
    .clickfinger_button_map_default = noop,
    .clickfinger_button_map_current = noop,
    .middle_emulation_support = noop,
    .middle_emulation_default = noop,
    .middle_emulation_current = noop,
    .scroll_method_support = noop,
    .scroll_method_default = noop,
    .scroll_method_current = noop,
    .scroll_button_default = noop,
    .scroll_button_current = noop,
    .scroll_button_lock_default = noop,
    .scroll_button_lock_current = noop,
    .dwt_support = noop,
    .dwt_default = noop,
    .dwt_current = noop,
    .dwtp_support = noop,
    .dwtp_default = noop,
    .dwtp_current = noop,
    .rotation_support = noop,
    .rotation_default = noop,
    .rotation_current = noop,
    .done = noop,
};

const river_libinput_config_v1_listener libinputConfigListener = {
    .finished = noop,
    .libinput_device =
        [](void *, river_libinput_config_v1 *,
           river_libinput_device_v1 *device) {
            river_libinput_device_v1_add_listener(
                device, &libinputDeviceListener, nullptr);
        },
};
