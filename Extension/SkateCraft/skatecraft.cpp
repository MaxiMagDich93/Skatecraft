#include "skatecraft.h"
#include "collision_feed.h"
#include "coords.h"
#include "input_map.h"
#include "link.h"
#include "overlay_feed.h"
#include "player_drive.h"
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
    s.edges.release_all();
    notice("Minecraft has the player. F8 gives skate. the board back.");
}

void stop_driving(plugins::Game& g, State& s, LinkView& link, bool put_skater_there) {
    if (!s.driving) return;
    g.release_camera(self);
    g.game_keys(self, {});
    g.hide_skater(self, false);
    g.freeze(self, false);
    link.push_input(proto::kInReleaseAll, 0);
    s.edges.release_all();
    s.released = true;
    overlay_feed::clear();
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
    s.edges.poll([&](int vk) { return g.key_down(vk); }, dik_from_vk, toggle_dik, s.changes, s.swallow);
    for (const auto& change : s.changes)
        link.push_input(change.kind == InputEdge::mouse ? proto::kInMouseButton : proto::kInKey, change.code, change.down ? 1 : 0);
    g.game_keys(self, s.swallow);
    // O opens Minecraft's pause and options menu, Esc being skate.'s (as SkyCraft does with Skyrim).
    const bool o = g.key_down('O');
    if (o && !s.o_was_down) link.push_input(proto::kInOpenMenu, 0);
    s.o_was_down = o;

    const bool screen = (mc.flags & proto::kMcScreenOpen) != 0;
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
    // Blocks, the player's body and other entities as Minecraft draws them (the render ring) are
    // for the world renderer of phase 3 (README). Until then the ring is emptied so Minecraft never
    // waits on it.
    link.drain_render([](std::uint32_t, const std::uint8_t*, std::uint32_t) {}, 64ull << 20);
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

void tick(bool ready) noexcept {
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
                notice("Waiting for Minecraft. Start it with SkyCraft's mod and -Dskycraft.link=Local\\SkateCraft_v1 (README, phase 1).");
                s.waiting_told = true;
            }
            s.drive.forget();
            s.arrive = true;
        } else {
            s.waiting_told = false;
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

        const bool arrived = have && in_game && (mc.flags & proto::kMcInWorld) && mc.teleportAck == s.teleport_seq;
        if (!arrived) return;
        if (!s.driving) start_driving(*g, s);

        forward_input(*g, s, view, mc, menu);

        LARGE_INTEGER now{}, rate{};
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&rate);
        const auto frame = s.drive.at(now.QuadPart, static_cast<double>(rate.QuadPart) / 1000.0);
        if (!frame) return;
        s.last_frame = frame;

        const auto camera = PlayerDrive::camera(*frame, s.look, s.mapping);
        if (const auto why = g->set_camera(self, camera.pose, std::clamp(camera.fov, 10.0f, 150.0f)); !why.empty()) {
            static std::string said;
            if (said != why) log_info("camera: " + (said = why));
        }

        // skate.'s ground, as rays straight down through its physics world.
        const RayFn ray = [](const Vec3& from, const Vec3& to) -> std::optional<Vec3> {
            const auto ground = local_ground_height(from[0], from[2], from[1], to[1]);
            if (!ground) return std::nullopt;
            return Vec3{from[0], *ground, from[2]};
        };
        s.collision.tick(view, s.mapping, frame->feet, skater.position, ray, seconds_now());

        // The hidden skater walks along at a distance, so skate. keeps streaming the world in
        // (and its collision) where Minecraft's player is.
        const auto feet = s.mapping.to_game(frame->feet);
        const double dx = feet[0] - skater.position[0], dz = feet[2] - skater.position[2];
        if (std::hypot(dx, dz) > skater_follow_metres && seconds_now() - s.skater_moved_at > skater_follow_seconds &&
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
    text += std::format("driving {}; mirror_x {}; collision epoch {}, {} columns, {} rays", s.driving ? "yes" : "no", s.mapping.mirror_x ? 1 : 0,
                        s.collision.epoch(), s.collision.columns_known(), s.collision.rays_cast());
    if (s.last_frame)
        text += std::format("; Minecraft feet {:.2f} {:.2f} {:.2f}", s.last_frame->feet.x, s.last_frame->feet.y, s.last_frame->feet.z);
    return text;
}

std::string set_mirror_x(bool on) {
    auto& s = state();
    s.mapping.mirror_x = on;
    save_settings(s);
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
