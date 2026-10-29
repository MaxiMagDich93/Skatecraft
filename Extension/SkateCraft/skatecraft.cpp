#include "skatecraft.h"
#include "collision_feed.h"
#include "coords.h"
#include "input_map.h"
#include "link.h"
#include "overlay_feed.h"
#include "player_drive.h"
#include "world_mesh.h"
#include "Engine/Core/Log/logging.h"
#include "Extension/Plugins/plugin_game_runtime.h"
#include "Extension/Plugins/plugin_ui.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Trainer/trainer.h"
#include "Extension/UI/Overlay/overlay.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <numbers>
#include <string>

namespace dingosdk::skatecraft {
namespace {
// SkateCraft asks the game for things the way a plugin would, under this name: the camera, the
// hidden and frozen skater and the keys it keeps from the game are all held as "skatecraft".
constexpr std::string_view self = "skatecraft";
constexpr int toggle_vk = VK_F8;
constexpr std::uint32_t toggle_dik = 0x42; // F8
constexpr double skater_follow_metres = 25; // collision only streams in near the skater
constexpr double skater_follow_seconds = 3;
constexpr double skater_follow_most_metres = 150;

void log_info(std::string_view text) { logging::log(logging::Level::info, logging::Channel::runtime, "SkateCraft: {}", text); }
void notice(std::string_view text, bool error = false) {
    overlay::notify(error ? overlay::NoticeLevel::error : overlay::NoticeLevel::info, "SkateCraft", std::string(text));
    log_info(text);
}

double seconds_now() { return static_cast<double>(GetTickCount64()) / 1000.0; }

struct State {
    bool enabled{}, driving{}, toggle_was_down{}, o_was_down{}, waiting_told{}, released{true}, arrive{true};
    Mapping mapping;
    Look look;
    PlayerDrive drive;
    CollisionFeed collision;
    InputEdges edges;
    std::vector<InputEdge> changes;
    std::vector<std::uint8_t> swallow;
    std::uint32_t world{}, epoch{1}, teleport_seq{1};
    DVec3 teleport_to{};
    double cursor_x{}, cursor_y{};
    bool screen_was_open{};
    double skater_moved_at{-100};
    bool arrived_before{}; // Minecraft had acknowledged the current teleport last tick
    // The skater is held still by noclip with no input (plugin_game_runtime freeze). Noclip starts only
    // with the game's own camera, so the camera waits for it, asking again now and then.
    double driving_since{}, freeze_asked_at{};
    std::uint32_t mc_flags{}; // Minecraft's last state flags
    std::optional<PlayerFrame> last_frame;
};
State& state() {
    static State s;
    return s;
}

// ---- settings, kept between runs in %LOCALAPPDATA%\ReSkate\SkateCraft.txt ----------------------------
std::filesystem::path settings_path() {
    wchar_t local[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (!length || length >= MAX_PATH) return {};
    return std::filesystem::path(local) / L"ReSkate" / L"SkateCraft.txt";
}
void load_settings(State& s) {
    const auto path = settings_path();
    if (path.empty()) return;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line == "mirror_x=1") s.mapping.mirror_x = true;
        if (line == "mirror_x=0") s.mapping.mirror_x = false;
    }
}
void save_settings(const State& s) {
    const auto path = settings_path();
    if (path.empty()) return;
    std::error_code ignored;
    std::filesystem::create_directories(path.parent_path(), ignored);
    std::ofstream out(path, std::ios::trunc);
    out << "mirror_x=" << (s.mapping.mirror_x ? 1 : 0) << '\n';
}

std::uint32_t world_id(const std::string& map) {
    const auto h = std::hash<std::string>{}(map);
    return static_cast<std::uint32_t>(h ^ (h >> 32)) | 1u;
}

plugins::Game* game() { return plugins::existing_game(); }

// ---- driving: the skater hidden and still, the camera and keys Minecraft's ------------------------
void start_driving(plugins::Game& g, State& s) {
    if (const auto why = g.hide_skater(self, true); !why.empty()) log_info("hiding the skater: " + why);
    if (const auto why = g.freeze(self, true); !why.empty()) log_info("holding the skater still: " + why);
    s.driving = true;
    s.driving_since = s.freeze_asked_at = seconds_now();
    s.edges.release_all();
    world_mesh::set_mirror(s.mapping.mirror_x);
    world_mesh::set_shown(true);
    notice("Minecraft has the player. F8 gives skate. the board back.");
}

void stop_driving(plugins::Game& g, State& s, LinkView& link, bool put_skater_there) {
    if (!s.driving) return;
    // A Minecraft screen left open (the chat, the inventory) would still be open when Minecraft has
    // the player again, and take every key: Esc closes it.
    if (s.mc_flags & proto::kMcScreenOpen) {
        link.push_input(proto::kInKey, 41, 1);
        link.push_input(proto::kInKey, 41, 0);
    }
    g.release_camera(self);
    g.game_keys(self, {});
    g.hide_skater(self, false);
    g.freeze(self, false);
    link.push_input(proto::kInReleaseAll, 0);
    s.edges.release_all();
    s.released = true;
    overlay_feed::clear();
    world_mesh::set_shown(false);
    world_mesh::set_selection(std::nullopt);
    // Skate on from where you walked to.
    if (put_skater_there && s.last_frame) {
        const auto feet = s.mapping.to_game(s.last_frame->feet);
        const double yaw = s.look.yaw * std::numbers::pi / 180;
        // Minecraft's forward (-sin yaw, cos yaw) as skate.'s heading (0 = +Z, forward (sin, cos)).
        double fx = -std::sin(yaw);
        if (s.mapping.mirror_x) fx = -fx;
        teleport_local_skater_heading(feet, static_cast<float>(std::atan2(fx, std::cos(yaw))));
    }
    s.driving = false;
}

// The characters a key types now, with Shift, Caps Lock and AltGr as they are (a German keyboard's
// @ is AltGr+Q), or none (a dead key such as ^, or a key that types nothing).
template<class IsDown>
std::u32string typed(int vk, IsDown&& down) {
    BYTE keys[256]{};
    const auto set = [&](int key) { keys[key] = 0x80; };
    if (down(VK_LSHIFT) || down(VK_RSHIFT)) set(VK_SHIFT), set(VK_LSHIFT);
    if (down(VK_LCONTROL) || down(VK_RCONTROL)) set(VK_CONTROL), set(VK_LCONTROL);
    if (down(VK_RMENU)) set(VK_MENU), set(VK_RMENU), set(VK_CONTROL), set(VK_LCONTROL); // AltGr is Ctrl+Alt to Windows
    else if (down(VK_LMENU)) set(VK_MENU), set(VK_LMENU);
    if (GetKeyState(VK_CAPITAL) & 1) keys[VK_CAPITAL] = 0x01;
    const UINT scan = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    wchar_t text[8]{};
    // Flag 4: the keyboard's dead-key state is left as it is, so skate. typing elsewhere is not disturbed.
    const int n = ToUnicodeEx(static_cast<UINT>(vk), scan, keys, text, 8, 4, GetKeyboardLayout(0));
    std::u32string out;
    for (int i = 0; i < n; ++i) {
        char32_t c = text[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < n) c = 0x10000 + ((c - 0xD800) << 10) + (static_cast<char32_t>(text[++i]) - 0xDC00);
        if (c >= 0x20 && c != 0x7F) out.push_back(c);
    }
    return out;
}

void forward_input(plugins::Game& g, State& s, LinkView& link, const proto::McState& mc, bool menu) {
    if (menu) {
        // skate.'s pause menu, ReSkate's menu or console, or the window in the background: Minecraft
        // lets go of everything and skate. gets its keys back.
        if (!s.released) {
            link.push_input(proto::kInReleaseAll, 0);
            s.edges.release_all();
            s.released = true;
        }
        g.game_keys(self, {});
        return;
    }
    s.released = false;
    const bool screen = (mc.flags & proto::kMcScreenOpen) != 0;
    const auto down = [&](int vk) { return g.key_down(vk); };
    s.edges.poll(down, dik_from_vk, toggle_dik, screen, s.changes, s.swallow);
    for (const auto& change : s.changes) {
        link.push_input(change.kind == InputEdge::mouse ? proto::kInMouseButton : proto::kInKey, change.code, change.down ? 1 : 0);
        // In a Minecraft screen (chat, a sign, a book, the search box) a key also types what it
        // says, with the keyboard's own layout, as Windows would turn it into a character.
        if (screen && change.down && change.kind == InputEdge::key)
            for (const char32_t c : typed(change.vk, down)) link.push_input(proto::kInText, 0, static_cast<std::int32_t>(c));
    }
    g.game_keys(self, s.swallow);
    // O opens Minecraft's pause and options menu, Esc being skate.'s (as SkyCraft does with Skyrim).
    // In a Minecraft screen both are Minecraft's own keys instead (input_map.h reserved_dik).
    const bool o = !screen && g.key_down('O');
    if (o && !s.o_was_down) link.push_input(proto::kInOpenMenu, 0);
    s.o_was_down = o;

    const auto display = plugin_ui::display();
    if (screen && !s.screen_was_open) {
        s.cursor_x = display[0] / 2;
        s.cursor_y = display[1] / 2;
    }
    s.screen_was_open = screen;
    if (const auto motion = g.mouse_motion(self)) {
        if (screen) {
            // A Minecraft screen (inventory, chest, crafting): the mouse is a cursor over its picture.
            s.cursor_x = std::clamp(s.cursor_x + motion->x, 0.0, static_cast<double>(display[0] - 1));
            s.cursor_y = std::clamp(s.cursor_y + motion->y, 0.0, static_cast<double>(display[1] - 1));
            link.push_input(proto::kInCursor, 0, static_cast<std::int32_t>(s.cursor_x), static_cast<std::int32_t>(s.cursor_y));
        } else {
            s.look.turn(motion->x, motion->y, mc.sensitivity);
        }
        if (motion->wheel != 0) link.push_input(proto::kInScroll, 0, static_cast<std::int32_t>(std::lround(motion->wheel * 120)));
    }
}

void drain_minecraft(LinkView& link, bool driving) {
    // Minecraft's block meshes and atlas (the render ring), kept for drawing them in skate.'s world
    // (world_mesh.h). Taken while it is not shown too, so nothing built is missing when it is.
    link.drain_render(world_mesh::receive, 64ull << 20);
    LinkView::Selection selection;
    if (driving && link.read_selection(selection))
        world_mesh::set_selection(selection.has ? std::optional<world_mesh::Box>(world_mesh::Box{
                                                      {selection.min[0], selection.min[1], selection.min[2]},
                                                      {selection.max[0], selection.max[1], selection.max[2]}})
                                                : std::nullopt);
    proto::McEvent event{};
    while (link.pop_event(event))
        if (event.type == proto::kEvPlayerDied) notice("Your Minecraft player died. Respawn in Minecraft (the overlay shows its screen).");
    // Minecraft's screen is shown only while it has the player; before that the frames are taken
    // (so its triple buffer keeps turning) and dropped.
    if (link.acquire_overlay_frame() && driving) {
        const auto* header = link.front_header();
        if (header->width && header->height && header->width <= proto::kMaxOverlayW && header->height <= proto::kMaxOverlayH)
            overlay_feed::publish(link.front_pixels(), header->width, header->height, (header->flags & 1u) != 0, header->frameId);
    }
}
}

void tick(bool ready, bool noclip) noexcept {
    try {
        auto& s = state();
        auto* g = game();
        if (!g) return;
        static bool loaded = [] {
            load_settings(state());
            return true;
        }();
        (void)loaded;
        const bool toggle = g->key_down(toggle_vk) && g->input_allowed();
        if (toggle && !s.toggle_was_down) notice(set_enabled(!s.enabled));
        s.toggle_was_down = toggle;
        auto& link = Link::get();
        if (!s.enabled) return;
        if (!link.valid()) {
            if (const auto why = link.create(); !why.empty()) {
                s.enabled = false;
                notice("Could not open the shared memory: " + why, true);
                return;
            }
            log_info("shared memory open, waiting for Minecraft");
        }
        auto& view = link.view();
        const auto now_ms = GetTickCount64();
        view.heartbeat(now_ms);

        const auto skater = trainer::telemetry();
        const bool in_game = ready && skater.skater;
        if (const auto world = world_id(g->world_map()); world != s.world) {
            // A new map: Minecraft starts its collision from nothing and the player arrives anew.
            s.world = world;
            s.collision.clear(view, ++s.epoch);
            s.arrive = true;
        }

        proto::McState mc{};
        const bool have = view.read_mc_state(mc) && view.minecraft_alive(now_ms);
        if (!have) {
            if (s.driving) {
                stop_driving(*g, s, view, true);
                notice("Minecraft stopped answering; skate. has the player again.", true);
            }
            if (!s.waiting_told) {
                notice("Waiting for Minecraft. Start it with SkyCraft's mod and -Dskycraft.link=SkateCraft_v1 (README, phase 1).");
                s.waiting_told = true;
            }
            s.drive.forget();
            s.arrive = true;
        } else {
            s.waiting_told = false;
            s.mc_flags = mc.flags;
            s.drive.observe(mc);
            drain_minecraft(view, s.driving);
        }

        // Where Minecraft's player should appear: where the skater is, facing the way it faces.
        if (s.arrive && in_game && have) {
            s.teleport_to = s.mapping.to_mc(skater.position);
            s.look.yaw = s.mapping.yaw_from_heading(skater.heading * std::numbers::pi / 180);
            s.look.pitch = 0;
            ++s.teleport_seq;
            s.arrive = false;
            // Minecraft's ticks from before the move are of another place (with the X axis newly
            // mirrored, another place entirely): the camera, the ground and the skater must not
            // follow them.
            s.drive.forget();
            s.last_frame.reset();
            log_info(std::format("Minecraft's player goes to {:.1f} {:.1f} {:.1f}", s.teleport_to.x, s.teleport_to.y, s.teleport_to.z));
        }

        const auto focus = g->input_focus();
        const bool menu = !g->input_allowed() || !focus.gameplay || focus.interface_open;
        const auto display = plugin_ui::display();
        proto::SkyState sky{};
        sky.flags = (in_game ? proto::kSkyInGame : 0u) | (menu ? proto::kSkyMenuOpen : 0u) | (!ready ? proto::kSkyLoading : 0u);
        sky.worldId = s.world;
        sky.collisionEpoch = s.epoch;
        sky.posX = s.teleport_to.x;
        sky.posY = s.teleport_to.y;
        sky.posZ = s.teleport_to.z;
        sky.yaw = static_cast<float>(s.look.yaw);
        sky.pitch = static_cast<float>(s.look.pitch);
        sky.teleportSeq = s.teleport_seq;
        sky.viewportW = static_cast<std::uint32_t>(std::max(1.0f, display[0]));
        sky.viewportH = static_cast<std::uint32_t>(std::max(1.0f, display[1]));
        sky.gameHour = 12; // skate.'s time of day could drive Minecraft's sky later; it is not drawn yet
        view.write_sky_state(sky);

        // skate.'s ground, as rays straight down through its physics world. It is sent before the
        // player arrives too: Minecraft holds its player still where it is to appear until the
        // ground there is known, and only then says it has arrived.
        const RayFn ray = [](const Vec3& from, const Vec3& to) -> std::optional<Vec3> {
            const auto ground = local_ground_height(from[0], from[2], from[1], to[1]);
            if (!ground) return std::nullopt;
            return Vec3{from[0], *ground, from[2]};
        };
        if (have && in_game) {
            const DVec3 around = s.driving && s.last_frame ? s.last_frame->feet : s.teleport_to;
            s.collision.tick(view, s.mapping, around, skater.position, ray, seconds_now());
        }

        const bool arrived = have && in_game && (mc.flags & proto::kMcInWorld) && mc.teleportAck == s.teleport_seq;
        if (!arrived) {
            s.arrived_before = false;
            return;
        }
        if (!s.arrived_before) {
            // Just arrived: only ticks from here on, and the skater stays put for a moment.
            s.arrived_before = true;
            s.drive.forget();
            s.drive.observe(mc);
            s.skater_moved_at = seconds_now();
        }
        if (!s.driving) start_driving(*g, s);

        forward_input(*g, s, view, mc, menu);

        LARGE_INTEGER now{}, rate{};
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&rate);
        const auto frame = s.drive.at(now.QuadPart, static_cast<double>(rate.QuadPart) / 1000.0);
        if (!frame) return;
        s.last_frame = frame;

        // The skater must be in noclip before the camera is taken: noclip will not start without the
        // game's own camera, and a skater left out of it rolls off with the keys skate. still reads.
        // After a few seconds without it (a host that forbids noclip) the camera is taken anyway.
        if (!noclip) {
            const double now_s = seconds_now();
            if (now_s - s.driving_since < 4) {
                if (now_s - s.freeze_asked_at > 1) {
                    g->freeze(self, false);
                    if (const auto why = g->freeze(self, true); !why.empty()) log_info("holding the skater still: " + why);
                    s.freeze_asked_at = now_s;
                }
                g->release_camera(self);
                return;
            }
        }
        const auto camera = PlayerDrive::camera(*frame, s.look, s.mapping);
        if (const auto why = g->set_camera(self, camera.pose, std::clamp(camera.fov, 10.0f, 150.0f)); !why.empty()) {
            static std::string said;
            if (said != why) log_info("camera: " + (said = why));
        }

        // The hidden skater walks along at a distance, so skate. keeps streaming the world in
        // (and its collision) where Minecraft's player is.
        const auto feet = s.mapping.to_game(frame->feet);
        const double dx = feet[0] - skater.position[0], dz = feet[2] - skater.position[2];
        // Never further than a walk could take the player in a few seconds: a jump of hundreds of
        // metres means the frame is wrong, and the skater would land somewhere it should not be.
        const double away = std::hypot(dx, dz);
        if (away > skater_follow_metres && away < skater_follow_most_metres && seconds_now() - s.skater_moved_at > skater_follow_seconds &&
            !local_skater_teleport_pending()) {
            teleport_local_skater_heading(feet, skater.heading * std::numbers::pi_v<float> / 180);
            s.skater_moved_at = seconds_now();
        }
    } catch (...) {
        logging::write(logging::Level::error, logging::Channel::runtime, "SkateCraft: tick failed");
    }
}

std::string set_enabled(bool on) {
    auto& s = state();
    if (on == s.enabled) return on ? "SkateCraft is already on." : "SkateCraft is already off.";
    s.enabled = on;
    if (!on) {
        if (auto* g = game(); g && Link::get().valid()) {
            auto& view = Link::get().view();
            stop_driving(*g, s, view, true);
            // Minecraft keeps running; a menu-open state makes it drop every held key until we return.
            proto::SkyState sky{};
            sky.flags = proto::kSkyMenuOpen;
            sky.teleportSeq = s.teleport_seq;
            view.write_sky_state(sky);
        }
        s.arrive = true;
        return "SkateCraft off: skate. has the player.";
    }
    s.arrive = true;
    return "SkateCraft on.";
}

bool enabled() { return state().enabled; }

std::string status() {
    const auto& s = state();
    auto& link = Link::get();
    std::string text = std::format("SkateCraft {}; shared memory {}; ", s.enabled ? "on" : "off", link.valid() ? "open" : "closed");
    if (link.valid()) {
        const bool alive = link.view().minecraft_alive(GetTickCount64());
        text += std::format("Minecraft {} (pid {}); ", alive ? "connected" : "not connected", link.view().minecraft_pid());
    }
    text += std::format("driving {}; mirror_x {}; collision epoch {}, {} columns, {} rays; {} block sections", s.driving ? "yes" : "no",
                        s.mapping.mirror_x ? 1 : 0, s.collision.epoch(), s.collision.columns_known(), s.collision.rays_cast(),
                        world_mesh::section_count());
    if (s.last_frame)
        text += std::format("; Minecraft feet {:.2f} {:.2f} {:.2f}", s.last_frame->feet.x, s.last_frame->feet.y, s.last_frame->feet.z);
    return text;
}

std::string set_mirror_x(bool on) {
    auto& s = state();
    s.mapping.mirror_x = on;
    save_settings(s);
    world_mesh::set_mirror(on);
    if (Link::get().valid()) s.collision.clear(Link::get().view(), ++s.epoch);
    s.arrive = true;
    return std::format("mirror_x {}: the player arrives again where the skater is.", on ? 1 : 0);
}

std::string resync() {
    auto& s = state();
    if (Link::get().valid()) s.collision.clear(Link::get().view(), ++s.epoch);
    s.arrive = true;
    return "Collision sent again, and Minecraft's player put back where the skater is.";
}
}
