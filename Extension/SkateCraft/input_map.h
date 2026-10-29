#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Keyboard and mouse for Minecraft. ReSkate reads keys as Windows virtual-key codes
// (plugin_game.h key_down); SkyCraft's Fabric mod takes SDL scancodes (USB HID usages, the
// physical key) for keys and SDL button numbers for the mouse (InputBridge.java), and applies the
// keyboard layout itself. So a virtual key goes to its physical scan code first (Windows'
// MapVirtualKey, the same codes DirectInput reports), then through SkyCraft's own table to SDL.
// That way a German keyboard's Z and Y, and its ^ key, reach Minecraft as the keys they are.
namespace dingosdk::skatecraft {

// SDL scancode for a DirectInput / set-1 scan code (0x80 set for the E0-prefixed keys), or 0.
// The table is SkyCraft's (skse/src/Input.cpp, MIT).
std::uint16_t sdl_from_dik(std::uint32_t dik);
// The set-1 scan code of a virtual key, as above, or 0. Windows only (0 elsewhere).
std::uint32_t dik_from_vk(int virtual_key);
// SDL mouse button (1 left, 2 middle, 3 right, 4 and 5 the side buttons) for a virtual key, or 0.
std::uint16_t sdl_mouse_button(int virtual_key);

// Physical keys skate. and ReSkate keep while Minecraft drives the player, as SkyCraft keeps
// Skyrim's: Esc (skate.'s pause menu), the console key left of 1, Insert (ReSkate's menu) and the
// key that turns SkateCraft on and off. O opens Minecraft's own pause menu instead (InputBridge).
inline constexpr std::uint32_t dik_escape = 0x01, dik_console = 0x29, dik_insert = 0xD2, dik_o = 0x18;
bool reserved_dik(std::uint32_t dik, std::uint32_t toggle_dik);

struct InputEdge {
    enum Kind : std::uint8_t { key, mouse } kind{};
    std::uint16_t code{}; // SDL scancode or button
    bool down{};
};

// Remembers which keys were down and reports what changed.
class InputEdges {
public:
    // `down(vk)` says whether a virtual key is down now, `to_dik(vk)` its scan code. Fills `out`
    // with the changes since the last call, and `swallow` with the virtual keys skate. should not
    // see this frame (every forwarded key that is down).
    template<class IsDown, class ToDik>
    void poll(IsDown&& down, ToDik&& to_dik, std::uint32_t toggle_dik, std::vector<InputEdge>& out, std::vector<std::uint8_t>& swallow) {
        out.clear();
        swallow.clear();
        // Several virtual keys can be one physical key (Num Lock turns the number pad's keys into
        // arrows), so what is down is gathered per SDL code before it is compared.
        std::array<bool, 256> keys{};
        std::array<bool, 8> buttons{};
        for (int vk = 1; vk < 256; ++vk) {
            // The generic Shift, Ctrl and Alt keys repeat their left and right versions.
            if (vk == 0x10 || vk == 0x11 || vk == 0x12) continue;
            const std::uint16_t button = sdl_mouse_button(vk);
            std::uint16_t key = 0;
            if (!button) {
                const std::uint32_t dik = to_dik(vk);
                if (!dik || reserved_dik(dik, toggle_dik)) continue;
                key = sdl_from_dik(dik);
                if (!key) continue;
            }
            if (!down(vk)) continue;
            swallow.push_back(static_cast<std::uint8_t>(vk));
            if (button) buttons[button] = true;
            else keys[key] = true;
        }
        for (std::size_t code = 1; code < keys.size(); ++code)
            if (keys[code] != held_keys_[code]) out.push_back({InputEdge::key, static_cast<std::uint16_t>(code), held_keys_[code] = keys[code]});
        for (std::size_t code = 1; code < buttons.size(); ++code)
            if (buttons[code] != held_buttons_[code]) out.push_back({InputEdge::mouse, static_cast<std::uint16_t>(code), held_buttons_[code] = buttons[code]});
    }
    // Forgets what was held (SkateCraft turned off, the window lost focus, a menu opened); the
    // caller also sends Minecraft kInReleaseAll.
    void release_all() {
        held_keys_.fill(false);
        held_buttons_.fill(false);
    }

private:
    std::array<bool, 256> held_keys_{};
    std::array<bool, 8> held_buttons_{};
};
}
