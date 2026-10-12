#include <chrono>
#include "zelda_config.h"
#include <array>

#include "librecomp/helpers.hpp"
#include "recomp_input.h"
#include "ultramodern/ultramodern.hpp"

// Arrays that hold the mappings for every input for keyboard and controller respectively.
using input_mapping = std::array<recomp::InputField, recomp::bindings_per_input>;
using input_mapping_array = std::array<input_mapping, static_cast<size_t>(recomp::GameInput::COUNT)>;
static input_mapping_array keyboard_input_mappings{};
static input_mapping_array controller_input_mappings{};

// Make the button value array, which maps a button index to its bit field.
#define DEFINE_INPUT(name, value, readable) uint16_t(value##u),
static const std::array n64_button_values = {
    DEFINE_N64_BUTTON_INPUTS()
};
#undef DEFINE_INPUT

// Make the input name array.
#define DEFINE_INPUT(name, value, readable) readable,
static const std::vector<std::string> input_names = {
    DEFINE_ALL_INPUTS()
};
#undef DEFINE_INPUT

// Make the input enum name array.
#define DEFINE_INPUT(name, value, readable) #name,
static const std::vector<std::string> input_enum_names = {
    DEFINE_ALL_INPUTS()
};
#undef DEFINE_INPUT

size_t recomp::get_num_inputs() {
    return (size_t)GameInput::COUNT;
}

const std::string& recomp::get_input_name(GameInput input) {
    return input_names.at(static_cast<size_t>(input));
}

const std::string& recomp::get_input_enum_name(GameInput input) {
    return input_enum_names.at(static_cast<size_t>(input));
}

recomp::GameInput recomp::get_input_from_enum_name(const std::string_view enum_name) {
    auto find_it = std::find(input_enum_names.begin(), input_enum_names.end(), enum_name);
    if (find_it == input_enum_names.end()) {
        return recomp::GameInput::COUNT;
    }

    return static_cast<recomp::GameInput>(find_it - input_enum_names.begin());
}

// Due to an RmlUi limitation this can't be const. Ideally it would return a const reference or even just a straight up copy.
recomp::InputField& recomp::get_input_binding(GameInput input, size_t binding_index, recomp::InputDevice device) {
    input_mapping_array& device_mappings = (device == recomp::InputDevice::Controller) ?  controller_input_mappings : keyboard_input_mappings;
    input_mapping& cur_input_mapping = device_mappings.at(static_cast<size_t>(input));

    if (binding_index < cur_input_mapping.size()) {
        return cur_input_mapping[binding_index];
    }
    else {
        static recomp::InputField dummy_field = {};
        return dummy_field;
    }
}

void recomp::set_input_binding(recomp::GameInput input, size_t binding_index, recomp::InputDevice device, recomp::InputField value) {
    input_mapping_array& device_mappings = (device == recomp::InputDevice::Controller) ?  controller_input_mappings : keyboard_input_mappings;
    input_mapping& cur_input_mapping = device_mappings.at(static_cast<size_t>(input));

    if (binding_index < cur_input_mapping.size()) {
        cur_input_mapping[binding_index] = value;
    }
}

// Dual-Stick: left stick (deadzoned) read from the configured controller X/Y axis mappings.
static void get_left_stick_for_dual(float* x_out, float* y_out) {
    float lx = recomp::get_input_analog(controller_input_mappings[(size_t)recomp::GameInput::X_AXIS_POS])
             - recomp::get_input_analog(controller_input_mappings[(size_t)recomp::GameInput::X_AXIS_NEG]);
    float ly = recomp::get_input_analog(controller_input_mappings[(size_t)recomp::GameInput::Y_AXIS_POS])
             - recomp::get_input_analog(controller_input_mappings[(size_t)recomp::GameInput::Y_AXIS_NEG]);
    recomp::apply_joystick_deadzone(lx, ly, &lx, &ly);
    *x_out = lx;
    *y_out = ly;
}
bool recomp_dual_stick_gameplay_recent();
int recomp_dual_stick_control_style();

namespace {
    // Control styles reported by the game: 4 = 2.1, 5 = 2.2, 6 = 2.3, 7 = 2.4.
    // 2.1/2.3 are right-handed layouts, 2.2/2.4 are the left-handed mirrors.
    // 2.1/2.2: aim = second pad Z, fire = primary pad Z. 2.3/2.4: the other way round.
    struct DualStyle {
        bool is_2x;
        bool left_handed;
        bool aim_on_second_pad;
    };

    DualStyle get_dual_style() {
        const int s = recomp_dual_stick_control_style();
        DualStyle d{};
        d.is_2x = (s >= 4 && s <= 7);
        d.left_handed = (s == 5 || s == 7);
        d.aim_on_second_pad = (s == 4 || s == 5);
        return d;
    }

    // Tuning: how long a D-pad weapon-cycle button press is held, and how hard the D-pad pushes
    // the second pad's stick Y for crouch/stand/zoom (1.0 = about 2/3 of full stick).
    constexpr int64_t dual_pulse_ms = 120;
    constexpr float dual_dpad_stick_value = 1.0f;

    int64_t dual_pulse_fwd_until = 0;
    int64_t dual_pulse_back_until = 0;
    bool dual_prev_dpad_left = false;
    bool dual_prev_dpad_right = false;

    int64_t dual_now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void dual_update_dpad_pulses(bool active, bool left, bool right) {
        if (!active) {
            dual_pulse_fwd_until = 0;
            dual_pulse_back_until = 0;
            dual_prev_dpad_left = false;
            dual_prev_dpad_right = false;
            return;
        }
        const int64_t now = dual_now_ms();
        if (right && !dual_prev_dpad_right) { dual_pulse_fwd_until = now + dual_pulse_ms; }
        if (left && !dual_prev_dpad_left) { dual_pulse_back_until = now + dual_pulse_ms; }
        dual_prev_dpad_left = left;
        dual_prev_dpad_right = right;
    }
}

bool recomp::get_n64_input(int controller_num, uint16_t* buttons_out, float* x_out, float* y_out) {
    uint16_t cur_buttons = 0;
    float cur_x = 0.0f;
    float cur_y = 0.0f;

    const bool dual_compatibility_enabled =
        zelda64::get_dual_controller_compatibility_mode() == zelda64::DualControllerCompatibilityMode::On;
    const bool gameplay_recent = dual_compatibility_enabled && recomp_dual_stick_gameplay_recent();
    const DualStyle ds = get_dual_style();
    // 2.x styles get the dual-stick layout; 1.x styles in gameplay behave like a normal single pad.
    const bool dual_swap_active = gameplay_recent && ds.is_2x;
    const bool dual_buttons_mode = dual_compatibility_enabled && !(gameplay_recent && !ds.is_2x);

    auto held = [](recomp::GameInput g) { return recomp::get_input_digital(controller_input_mappings[(size_t)g]); };
    auto n64_mask = [](recomp::GameInput g) -> uint16_t {
        return n64_button_values[(size_t)g - (size_t)recomp::GameInput::N64_BUTTON_START];
    };

    bool dpad_up = false, dpad_down = false;
    if (dual_swap_active && !recomp::game_input_disabled()) {
        dpad_up = held(recomp::GameInput::DPAD_UP);
        dpad_down = held(recomp::GameInput::DPAD_DOWN);
        dual_update_dpad_pulses(true, held(recomp::GameInput::DPAD_LEFT), held(recomp::GameInput::DPAD_RIGHT));
    } else {
        dual_update_dpad_pulses(false, false, false);
    }
    const int64_t now_ms = dual_now_ms();
    const bool pulse_fwd = dual_pulse_fwd_until > now_ms;
    const bool pulse_back = dual_pulse_back_until > now_ms;

    if (controller_num == 1) {
        if (!dual_compatibility_enabled) {
            return false;
        }
        if (!recomp::game_input_disabled()) {
            float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
            get_left_stick_for_dual(&lx, &ly);
            recomp::get_right_analog(&rx, &ry);
            const bool lb = held(recomp::GameInput::Z);
            const bool rb = held(recomp::GameInput::R);
            if (dual_swap_active) {
                // move stick: walk + strafe. look stick: turn + pitch + crosshair.
                const float mx = ds.left_handed ? rx : lx;
                const float my = ds.left_handed ? ry : ly;
                const float ky = ds.left_handed ? ly : ry;
                const bool aim_btn = ds.left_handed ? lb : rb;
                const bool fire_btn = ds.left_handed ? rb : lb;
                cur_x = mx; // strafe
                if (aim_btn) {
                    // While aiming this stick Y is crouch/stand and sniper zoom: drive it from the D-pad only.
                    cur_y = (dpad_up ? dual_dpad_stick_value : 0.0f) - (dpad_down ? dual_dpad_stick_value : 0.0f);
                } else {
                    // Right-handed: pitch (look stick). Left-handed: walk (move stick).
                    cur_y = ds.left_handed ? my : ky;
                }
                const bool z_pressed = ds.aim_on_second_pad ? aim_btn : (fire_btn || pulse_back);
                if (z_pressed) { cur_buttons |= n64_mask(recomp::GameInput::Z); }
            } else if (dual_buttons_mode) {
                if (lb) { cur_buttons |= n64_mask(recomp::GameInput::Z); }
            }
        }
        *buttons_out = cur_buttons;
        *x_out = std::clamp(cur_x * 0.65f, -1.0f, 1.0f);
        *y_out = std::clamp(cur_y * 0.65f, -1.0f, 1.0f);
        return true;
    }

    if (controller_num != 0) {
        return false;
    }

    if (!recomp::game_input_disabled()) {
        for (size_t i = 0; i < n64_button_values.size(); i++) {
            size_t input_index = (size_t)GameInput::N64_BUTTON_START + i;
            cur_buttons |= recomp::get_input_digital(keyboard_input_mappings[input_index]) ? n64_button_values[i] : 0;
            if (!dual_buttons_mode) {
                cur_buttons |= recomp::get_input_digital(controller_input_mappings[input_index]) ? n64_button_values[i] : 0;
            } else {
                const bool is_zr = (input_index == (size_t)GameInput::Z || input_index == (size_t)GameInput::R);
                const bool is_dpad = dual_swap_active &&
                    (input_index == (size_t)GameInput::DPAD_UP || input_index == (size_t)GameInput::DPAD_DOWN ||
                     input_index == (size_t)GameInput::DPAD_LEFT || input_index == (size_t)GameInput::DPAD_RIGHT);
                if (!is_zr && !is_dpad) {
                    cur_buttons |= recomp::get_input_digital(controller_input_mappings[input_index]) ? n64_button_values[i] : 0;
                }
                // Menus: RB acts as Z on pad 0.
                if (!dual_swap_active && input_index == (size_t)GameInput::R &&
                    recomp::get_input_digital(controller_input_mappings[input_index])) {
                    cur_buttons |= n64_button_values[(size_t)GameInput::Z - (size_t)GameInput::N64_BUTTON_START];
                }
            }
        }

        float joystick_x = recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::X_AXIS_POS])
                        - recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::X_AXIS_NEG]);

        float joystick_y = recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::Y_AXIS_POS])
                        - recomp::get_input_analog(controller_input_mappings[(size_t)GameInput::Y_AXIS_NEG]);

        recomp::apply_joystick_deadzone(joystick_x, joystick_y, &joystick_x, &joystick_y);
        if (dual_swap_active) {
            float rx = 0.0f, ry = 0.0f;
            recomp::get_right_analog(&rx, &ry);
            const float lx = joystick_x, ly = joystick_y;
            const bool lb = held(recomp::GameInput::Z);
            const bool rb = held(recomp::GameInput::R);
            const bool aim_btn = ds.left_handed ? lb : rb;
            const bool fire_btn = ds.left_handed ? rb : lb;
            const float kx = ds.left_handed ? lx : rx;
            const float ky = ds.left_handed ? ly : ry;
            const float my = ds.left_handed ? ry : ly;
            joystick_x = kx; // turn / crosshair X
            // Right-handed: walk on the move stick, crosshair Y while aiming. Left-handed: always pitch / crosshair Y.
            joystick_y = (!ds.left_handed && !aim_btn) ? my : ky;
            const bool z_pressed = ds.aim_on_second_pad ? (fire_btn || pulse_back) : aim_btn;
            if (z_pressed) { cur_buttons |= n64_mask(recomp::GameInput::Z); }
            // D-pad Right = next weapon (A press). D-pad Left = previous weapon (A + fire pressed together).
            if (pulse_fwd || pulse_back) { cur_buttons |= n64_mask(recomp::GameInput::A); }
        }

        cur_x = recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::X_AXIS_POS])
                - recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::X_AXIS_NEG]) + joystick_x;

        cur_y = recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::Y_AXIS_POS])
                - recomp::get_input_analog(keyboard_input_mappings[(size_t)GameInput::Y_AXIS_NEG]) + joystick_y;
    }

    *buttons_out = cur_buttons;
    *x_out = std::clamp(cur_x * 0.65f, -1.0f, 1.0f);
    *y_out = std::clamp(cur_y * 0.65f, -1.0f, 1.0f);

    return true;
}

