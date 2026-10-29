#include "input_map.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace dingosdk::skatecraft {
namespace {
// DirectInput scan code -> SDL scancode / USB HID usage. SkyCraft's table (skse/src/Input.cpp, MIT).
constexpr auto dik_to_sdl = [] {
    std::array<std::uint16_t, 256> t{};
    t[0x01] = 41; // Esc
    for (int i = 0; i < 9; ++i) t[0x02 + i] = static_cast<std::uint16_t>(30 + i); // 1-9
    t[0x0B] = 39;                                                                  // 0
    t[0x0C] = 45, t[0x0D] = 46, t[0x0E] = 42, t[0x0F] = 43;                        // - = Backspace Tab
    t[0x10] = 20, t[0x11] = 26, t[0x12] = 8, t[0x13] = 21, t[0x14] = 23;           // Q W E R T
    t[0x15] = 28, t[0x16] = 24, t[0x17] = 12, t[0x18] = 18, t[0x19] = 19;          // Y U I O P
    t[0x1A] = 47, t[0x1B] = 48, t[0x1C] = 40, t[0x1D] = 224;                       // [ ] Enter LCtrl
    t[0x1E] = 4, t[0x1F] = 22, t[0x20] = 7, t[0x21] = 9, t[0x22] = 10;             // A S D F G
    t[0x23] = 11, t[0x24] = 13, t[0x25] = 14, t[0x26] = 15;                        // H J K L
    t[0x27] = 51, t[0x28] = 52, t[0x29] = 53, t[0x2A] = 225, t[0x2B] = 49;         // ; ' ` LShift backslash
    t[0x2C] = 29, t[0x2D] = 27, t[0x2E] = 6, t[0x2F] = 25, t[0x30] = 5;            // Z X C V B
    t[0x31] = 17, t[0x32] = 16, t[0x33] = 54, t[0x34] = 55, t[0x35] = 56;          // N M , . /
    t[0x36] = 229, t[0x37] = 85, t[0x38] = 226, t[0x39] = 44, t[0x3A] = 57;        // RShift KP* LAlt Space Caps
    for (int i = 0; i < 10; ++i) t[0x3B + i] = static_cast<std::uint16_t>(58 + i); // F1-F10
    t[0x45] = 83, t[0x46] = 71;                                                    // NumLock ScrollLock
    t[0x47] = 95, t[0x48] = 96, t[0x49] = 97, t[0x4A] = 86;                        // KP7 KP8 KP9 KP-
    t[0x4B] = 92, t[0x4C] = 93, t[0x4D] = 94, t[0x4E] = 87;                        // KP4 KP5 KP6 KP+
    t[0x4F] = 89, t[0x50] = 90, t[0x51] = 91, t[0x52] = 98, t[0x53] = 99;          // KP1 KP2 KP3 KP0 KP.
    t[0x56] = 100, t[0x57] = 68, t[0x58] = 69;                                     // OEM102 F11 F12
    t[0x9C] = 88, t[0x9D] = 228, t[0xB5] = 84, t[0xB7] = 70, t[0xB8] = 230;        // KPEnter RCtrl KP/ PrtSc RAlt
    t[0xC5] = 72, t[0xC7] = 74, t[0xC8] = 82, t[0xC9] = 75, t[0xCB] = 80;          // Pause Home Up PgUp Left
    t[0xCD] = 79, t[0xCF] = 77, t[0xD0] = 81, t[0xD1] = 78, t[0xD2] = 73;          // Right End Down PgDn Insert
    t[0xD3] = 76, t[0xDB] = 227, t[0xDC] = 231, t[0xDD] = 101;                     // Delete LWin RWin Menu
    return t;
}();
}

std::uint16_t sdl_from_dik(std::uint32_t dik) { return dik < dik_to_sdl.size() ? dik_to_sdl[dik] : 0; }

std::uint32_t dik_from_vk(int vk) {
#ifdef _WIN32
    // MAPVK_VK_TO_VSC_EX gives 0xE0xx for the extended keys (arrows, right Ctrl/Alt, Insert, ...),
    // which DirectInput numbers 0x80 and up.
    const UINT code = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC_EX);
    if (!code) return 0;
    const std::uint32_t low = code & 0x7F;
    return (code & 0xFF00) == 0xE000 ? (low | 0x80) : low;
#else
    (void)vk;
    return 0;
#endif
}

std::uint16_t sdl_mouse_button(int vk) {
    switch (vk) {
    case 0x01: return 1; // VK_LBUTTON
    case 0x04: return 2; // VK_MBUTTON
    case 0x02: return 3; // VK_RBUTTON
    case 0x05: return 4; // VK_XBUTTON1
    case 0x06: return 5; // VK_XBUTTON2
    default: return 0;
    }
}

bool reserved_dik(std::uint32_t dik, std::uint32_t toggle_dik) {
    return dik == dik_escape || dik == dik_console || dik == dik_insert || dik == dik_o || dik == toggle_dik;
}
}
