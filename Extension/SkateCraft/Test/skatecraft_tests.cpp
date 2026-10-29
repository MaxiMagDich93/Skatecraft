// SkateCraft apart from the game: the shared memory speaks SkyCraft's protocol byte for byte,
// skate.'s ground reaches Minecraft as upward triangles in the right regions, keys turn into
// press and release events once each, and the player is interpolated between Minecraft's ticks.
#include "Extension/SkateCraft/collision_feed.h"
#include "Extension/SkateCraft/coords.h"
#include "Extension/SkateCraft/input_map.h"
#include "Extension/SkateCraft/link.h"
#include "Extension/SkateCraft/overlay_feed.h"
#include "Extension/SkateCraft/player_drive.h"
#include "Extension/SkateCraft/world_mesh.h"
#include "Extension/SkateCraft/world_saves.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace dingosdk::skatecraft;
namespace {
void check(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
bool near(double a, double b, double within = 1e-4) { return std::abs(a - b) <= within; }

struct Memory {
    std::uint8_t* bytes = static_cast<std::uint8_t*>(std::calloc(proto::kMappingBytes, 1));
    ~Memory() { std::free(bytes); }
};

// The collision ring read the way SkyCraft's SkyCollision.java reads it.
struct Message {
    std::uint32_t type{};
    std::vector<std::uint8_t> payload;
};
std::vector<Message> read_collision(std::uint8_t* base) {
    auto* ring = base + proto::kOffCollisionRing;
    auto& head = *reinterpret_cast<std::uint64_t*>(ring + proto::kColRingHeadOff);
    auto& tail = *reinterpret_cast<std::uint64_t*>(ring + proto::kColRingTailOff);
    auto* data = ring + proto::kColRingDataOff;
    std::vector<Message> out;
    while (tail < head) {
        const auto pos = tail % proto::kColRingDataBytes;
        const auto* header = reinterpret_cast<const proto::ColMsgHeader*>(data + pos);
        if (header->type == proto::kColPad) {
            tail += proto::kColRingDataBytes - pos;
            continue;
        }
        Message m{header->type, std::vector<std::uint8_t>(data + pos + 8, data + pos + 8 + header->payloadBytes)};
        out.push_back(std::move(m));
        tail += (8 + header->payloadBytes + 7) & ~7ull;
    }
    return out;
}

void protocol_layout() {
    // SkyCraft's own static_asserts hold in this copy, and the offsets are the ones its Java mirrors.
    check(sizeof(proto::SkyState) == 0x40 && sizeof(proto::McState) == 0xC8, "state sizes");
    check(sizeof(proto::ColTri) == 40 && sizeof(proto::ColRegion) == 32 && sizeof(proto::InputEvent) == 16, "message sizes");
    check(proto::kOffCollisionRing == 0x20000 && proto::kVersion == 11, "offsets and version this was written against");
}

void link_state_and_input() {
    Memory m;
    LinkView link(m.bytes);
    link.reset_own_regions(1234, 5000);
    const auto* header = reinterpret_cast<const proto::Header*>(m.bytes);
    check(header->magic == proto::kMagic && header->version == proto::kVersion && header->skyrimPid == 1234, "header stamped");
    const auto* water = reinterpret_cast<const proto::WaterGrid*>(m.bytes + proto::kOffWaterGrid);
    check(water->seq == 2 && water->surface[0] == proto::kNoWater && water->surface[255] == proto::kNoWater, "no water anywhere");

    proto::SkyState sky{};
    sky.flags = proto::kSkyInGame;
    sky.posX = 12.5;
    sky.teleportSeq = 7;
    link.write_sky_state(sky);
    const auto* shared = reinterpret_cast<const proto::SkyState*>(m.bytes + proto::kOffSkyState);
    check(shared->seq == 2 && shared->flags == proto::kSkyInGame && shared->posX == 12.5 && shared->teleportSeq == 7, "sky state written under its seqlock");

    proto::McState mc{};
    check(!link.read_mc_state(mc), "no Minecraft state before Minecraft wrote one");
    auto* mine = reinterpret_cast<proto::McState*>(m.bytes + proto::kOffMcState);
    mine->x = 3;
    mine->flags = proto::kMcInWorld;
    mine->seq = 2;
    check(link.read_mc_state(mc) && mc.x == 3 && mc.flags == proto::kMcInWorld, "Minecraft state read");
    reinterpret_cast<proto::Header*>(m.bytes)->mcHeartbeatMs = 4000;
    check(link.minecraft_alive(12000) && !link.minecraft_alive(15000), "Minecraft's heartbeat times out after 10 s");

    link.push_input(proto::kInKey, 26, 1);
    const auto* ring = m.bytes + proto::kOffInputRing;
    const auto head = *reinterpret_cast<const std::uint64_t*>(ring + proto::kInputRingHeadOff);
    const auto* event = reinterpret_cast<const proto::InputEvent*>(ring + proto::kInputRingDataOff);
    check(head == 1 && event->type == proto::kInKey && event->code == 26 && event->a == 1, "a key press in the input ring");
}

void coordinates() {
    for (const bool mirror : {false, true}) {
        Mapping map;
        map.mirror_x = mirror;
        map.origin = {100, 0, -50};
        map.mc_origin = {8, 64, 8};
        const Vec3 p{103.25f, 7.5f, -41.0f};
        const auto back = map.to_game(map.to_mc(p));
        check(near(back[0], p[0]) && near(back[1], p[1]) && near(back[2], p[2]), "a point goes to Minecraft and back");
        // A skate. heading becomes the Minecraft yaw that looks the same way.
        for (double heading = -3.0; heading <= 3.0; heading += 0.5) {
            const auto f = map.forward_game(map.yaw_from_heading(heading), 0);
            check(near(f[0], std::sin(heading)) && near(f[2], std::cos(heading)) && near(f[1], 0), "heading and yaw agree");
        }
    }
    Mapping plain;
    const auto down = plain.forward_game(0, 90);
    check(near(down[1], -1) && near(std::abs(down[0]) + std::abs(down[2]), 0), "pitch 90 looks straight down");
    const auto pose = camera_pose({1, 2, 3}, {0, 0, 1});
    check(near(pose[8], 0) && near(pose[10], -1) && near(pose[5], 1) && near(pose[12], 1) && near(pose[14], 3), "camera rows: back is -forward, up is up");
    Look look;
    look.turn(100, 1000, 0.5);
    check(near(look.pitch, 90) && look.yaw > 0, "the look turns and pitch stops at 90");
}

void collision_flat_ground_and_step() {
    // A floor at y 5, with a 1 m kerb from x 4 on: what SkyCraft's TriCollider would get.
    Memory m;
    LinkView link(m.bytes);
    link.reset_own_regions(1, 1);
    CollisionSettings settings;
    settings.radius_columns = 0;
    settings.rays_per_tick = 400;
    CollisionFeed feed(settings);
    feed.clear(link, 9);
    Mapping map;
    int rays = 0;
    const RayFn ray = [&](const Vec3& from, const Vec3& to) -> std::optional<Vec3> {
        ++rays;
        check(near(from[0], to[0]) && near(from[2], to[2]) && from[1] > to[1], "rays go straight down");
        const float ground = from[0] >= 4.0f ? 6.0f : 5.0f;
        if (from[1] < ground || to[1] > ground) return std::nullopt;
        return Vec3{from[0], ground, from[2]};
    };
    for (int t = 0; t < 4; ++t) feed.tick(link, map, {2, 5, 2}, std::nullopt, ray, t);
    check(rays == 33 * 33, "one column: 33 x 33 samples, each one ray");
    const auto messages = read_collision(m.bytes);
    check(!messages.empty() && messages[0].type == proto::kColClear, "the clear comes first");
    std::uint32_t epoch{};
    std::memcpy(&epoch, messages[0].payload.data(), 4);
    check(epoch == 9, "with its epoch");
    std::size_t tris = 0;
    bool region_after_tris = false;
    std::map<int, bool> known; // region y -> sent as a region
    for (std::size_t i = 1; i < messages.size(); ++i) {
        const auto& msg = messages[i];
        proto::ColRegion header{};
        std::memcpy(&header, msg.payload.data(), sizeof(header));
        check(header.epoch == 9 && header.minX == 0 && header.minZ == 0 && header.maxY == header.minY + 7, "regions of column 0,0, 8 blocks high");
        if (msg.type == proto::kColTris && header.minY != 0) check(header.count == 0, "nothing but air above and below the floor");
        if (msg.type == proto::kColTris) {
            check(msg.payload.size() == sizeof(header) + header.count * sizeof(proto::ColTri), "triangle message size");
            for (std::uint32_t k = 0; k < header.count; ++k) {
                proto::ColTri t{};
                std::memcpy(&t, msg.payload.data() + sizeof(header) + k * sizeof(t), sizeof(t));
                const float ux = t.v[3] - t.v[0], uz = t.v[5] - t.v[2], vx = t.v[6] - t.v[0], vz = t.v[8] - t.v[2];
                check(uz * vx - ux * vz > 0, "every triangle faces up");
                for (int c = 0; c < 3; ++c) check(t.v[c * 3 + 1] == 5.0f || t.v[c * 3 + 1] == 6.0f, "on the floor or the kerb");
            }
            tris += header.count;
        } else if (msg.type == proto::kColRegion) {
            check(header.count == 0, "regions carry no voxel blocks");
            region_after_tris = messages[i - 1].type == proto::kColTris;
            known[header.minY / 8] = true;
        }
    }
    check(tris == 32 * 32 * 2 && region_after_tris, "two triangles a cell, then the region");
    // Feet at y 5: the rays ran from 8 down to -43, regions -6 to 1. Minecraft wants the regions
    // at the feet, just above them and a region below known before it lets the player go.
    check(known[0] && known[1] && known[-1] && known[-6] && known.size() == 8, "every region the rays passed is known, the air too");
    check(feed.columns_known() == 1, "one column known");
}

void collision_skips_the_skater() {
    Memory m;
    LinkView link(m.bytes);
    CollisionSettings settings;
    settings.radius_columns = 0;
    settings.rays_per_tick = 2000;
    CollisionFeed feed(settings);
    feed.clear(link, 1);
    Mapping map;
    // The hidden skater stands at (4, 5, 4): a ray there would hit its head at 6.8.
    const RayFn ray = [](const Vec3& from, const Vec3&) -> std::optional<Vec3> {
        const bool on_skater = std::hypot(from[0] - 4, from[2] - 4) < 0.5;
        return Vec3{from[0], on_skater ? 6.8f : 5.0f, from[2]};
    };
    feed.tick(link, map, {4, 5, 4}, Vec3{4, 5, 4}, ray, 0);
    for (const auto& msg : read_collision(m.bytes)) {
        if (msg.type != proto::kColTris) continue;
        proto::ColRegion header{};
        std::memcpy(&header, msg.payload.data(), sizeof(header));
        for (std::uint32_t k = 0; k < header.count; ++k) {
            proto::ColTri t{};
            std::memcpy(&t, msg.payload.data() + sizeof(header) + k * sizeof(t), sizeof(t));
            for (int c = 0; c < 3; ++c) check(near(t.v[c * 3 + 1], 5.0), "no step where the hidden skater stands");
        }
    }
}

void triangles_span_regions() {
    // A cell rising from y 7 to 9 crosses from region 0 into region 1: it goes to both.
    const int n = 32;
    std::vector<float> heights(static_cast<std::size_t>((n + 1) * (n + 1)), 7.0f);
    heights[1] = 9.0f;
    const auto regions = CollisionFeed::triangulate(0, 0, 0.25f, heights);
    check(regions.count(0) && regions.count(1), "a triangle across a region boundary is in both");
    std::vector<float> holes(heights.size(), std::nanf(""));
    check(CollisionFeed::triangulate(0, 0, 0.25f, holes).empty(), "where rays met nothing there is nothing");
}

void keys_and_buttons() {
    InputEdges edges;
    std::map<int, bool> down;
    std::map<int, std::uint32_t> dik{{'W', 0x11}, {0x01, 0}, {0xA0, 0x2A}, {0x1B, 0x01}, {0x77, 0x42}, {'O', 0x18}};
    const auto is_down = [&](int vk) { return down[vk]; };
    const auto to_dik = [&](int vk) { return dik.count(vk) ? dik[vk] : 0u; };
    std::vector<InputEdge> out;
    std::vector<std::uint8_t> swallow;
    down['W'] = true;
    down[0x01] = true; // left mouse
    down[0x1B] = true; // Esc: skate.'s
    down[0x77] = true; // F8: SkateCraft's own switch
    edges.poll(is_down, to_dik, 0x42, false, out, swallow);
    check(out.size() == 2, "W and the left button change; Esc and F8 stay with skate.");
    bool w = false, click = false;
    for (const auto& e : out) {
        w |= e.kind == InputEdge::key && e.code == 26 && e.down;  // SDL_SCANCODE_W
        click |= e.kind == InputEdge::mouse && e.code == 1 && e.down;
    }
    check(w && click, "W is SDL 26, the left button is button 1");
    check(swallow.size() == 2, "skate. does not see W or the click");
    edges.poll(is_down, to_dik, 0x42, false, out, swallow);
    check(out.empty(), "held keys are not pressed again");
    down['W'] = false;
    edges.poll(is_down, to_dik, 0x42, false, out, swallow);
    check(out.size() == 1 && !out[0].down && out[0].code == 26, "and released once");
    // In a Minecraft screen Esc closes it, so it goes to Minecraft; outside one it stays with skate.
    edges.release_all();
    down.clear();
    down[0x1B] = true;
    edges.poll(is_down, to_dik, 0x42, true, out, swallow);
    check(out.size() == 1 && out[0].code == 41 && out[0].down && out[0].vk == 0x1B, "Esc reaches Minecraft's open screen, with its virtual key");
    check(sdl_from_dik(0x15) == 28 && sdl_from_dik(0x2C) == 29, "a German keyboard's Z and Y keys are the physical keys Minecraft expects");
}

void player_between_ticks() {
    PlayerDrive drive;
    proto::McState s{};
    s.tickMs = 50;
    s.tickQpc = 1000;
    s.prevX = 0;
    s.curX = 1;
    s.tickEyeO = s.tickEye = 1.62f;
    s.flags = proto::kMcInWorld | proto::kMcOnGround;
    s.fovDeg = 70;
    drive.observe(s);
    s.tickQpc = 1050;
    s.prevX = 1;
    s.curX = 2;
    drive.observe(s);
    // One tick behind: at 1075 the frame is drawn for 1025, halfway through the first tick.
    const auto f = drive.at(1075, 1.0);
    check(f && near(f->feet.x, 0.5) && near(f->eye_height, 1.62, 1e-6) && f->on_ground, "halfway between a tick's start and end");
    const auto later = drive.at(1125, 1.0);
    check(later && near(later->feet.x, 1.5), "and through the next tick");
}

void overlay_premultiplied() {
    const std::uint8_t pixels[] = {128, 64, 0, 128, 255, 255, 255, 255, 0, 0, 0, 0, 10, 20, 30, 40};
    overlay_feed::publish(pixels, 2, 2, true, 7);
    overlay_feed::Frame f;
    check(overlay_feed::take(f) && f.width == 2 && f.id == 7, "a frame is taken");
    // Bottom-up: the first row given is the bottom one.
    check(f.rgba[0] == 0 && f.rgba[3] == 0 && f.rgba[4] == 64 && f.rgba[7] == 40, "rows turned the right way up");
    check(f.rgba[8] == 255 && f.rgba[9] == 128 && f.rgba[10] == 0 && f.rgba[11] == 128, "premultiplied colour divided by its alpha");
    check(!overlay_feed::take(f), "only once");
}

void blocks_from_minecraft() {
    namespace wm = dingosdk::skatecraft::world_mesh;
    wm::clear();
    // An atlas of 2 x 1 pixels and a section with one solid and one glass triangle, as the ring carries them.
    std::vector<std::uint8_t> atlas{2, 0, 0, 0, 1, 0, 0, 0, 255, 0, 0, 255, 0, 255, 0, 255};
    wm::receive(proto::kRenAtlas, atlas.data(), static_cast<std::uint32_t>(atlas.size()));
    proto::RenSection header{3, 4, -2, 6};
    std::vector<std::uint8_t> section(sizeof header + 6 * sizeof(proto::RenVertex));
    std::memcpy(section.data(), &header, sizeof header);
    for (int k = 0; k < 6; ++k) {
        proto::RenVertex v{};
        v.x = static_cast<float>(k);
        v.flags = k < 3 ? 1u : 2u; // cutout, then translucent
        std::memcpy(section.data() + sizeof header + k * sizeof v, &v, sizeof v);
    }
    wm::receive(proto::kRenSection, section.data(), static_cast<std::uint32_t>(section.size()));
    wm::set_shown(true);
    auto scene = wm::snapshot();
    check(scene.shown && scene.atlas && scene.atlas->width == 2 && scene.atlas->rgba[4] == 0 && scene.atlas->rgba[5] == 255, "the atlas is kept");
    check(scene.sections && scene.sections->size() == 1, "one section");
    const auto& got = *scene.sections->front();
    check(got.sx == 3 && got.sy == 4 && got.sz == -2 && got.solid.size() == 3 && got.translucent.size() == 3, "solid and glass apart");
    check(got.translucent[0].x == 3.0f, "the vertices as sent");
    const auto before = scene.sections;
    check(wm::snapshot().sections == before, "the same list while nothing changed");
    // A section sent again with no vertices: its blocks were all broken.
    proto::RenSection empty{3, 4, -2, 0};
    wm::receive(proto::kRenSection, reinterpret_cast<const std::uint8_t*>(&empty), sizeof empty);
    check(wm::snapshot().sections->empty() && wm::section_count() == 0, "an empty section is gone");
    // A message cut short is ignored rather than read past its end.
    wm::receive(proto::kRenSection, section.data(), 40);
    check(wm::section_count() == 0, "a short section is ignored");
    wm::set_shown(false);
}

void worlds_kept_and_loaded() {
    namespace ws = dingosdk::skatecraft::world_saves;
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("skatecraft-worlds-" + std::to_string(std::rand()));
    fs::remove_all(root);
    const ws::Paths p{root / "saves" / "SkyCraft", root / "Welten"};
    const auto make_world = [&](const std::string& marker) {
        fs::create_directories(p.world / "region");
        std::ofstream(p.world / "level.dat") << marker;
        std::ofstream(p.world / "region" / "r.0.0.mca") << marker;
        std::ofstream(p.world / "session.lock") << "locked";
    };
    const auto marker = [](const fs::path& world) {
        std::ifstream in(world / "level.dat");
        std::string text;
        std::getline(in, text);
        return text;
    };
    // First run ever: nothing to move, a new world.
    check(ws::prepare(p, std::nullopt).starts_with("Neue"), "a new world the first time");
    make_world("A");
    // Ctrl+1 while playing: copied now, without the lock Minecraft holds.
    ws::keep(p, 1);
    check(marker(p.store / "Platz 1") == "A" && !fs::exists(p.store / "Platz 1" / "session.lock") &&
              fs::exists(p.store / "Platz 1" / "region" / "r.0.0.mca"), "Ctrl+1 copies the world, lock left out");
    // Built on after Ctrl+1, then quit: the next start keeps that last state, then starts anew.
    std::ofstream(p.world / "level.dat") << "A2";
    check(ws::prepare(p, std::nullopt).starts_with("Neue"), "a new world on the next start");
    check(!fs::exists(p.world) && marker(p.store / "Platz 1") == "A2" && marker(p.store / "Letzte Sitzung") == "A2",
          "the last state is in place 1 and in the last run's place");
    // Ctrl+Shift+1 before F8: place 1 is the world this run.
    make_world("B");
    check(ws::prepare(p, 1) == "Platz 1 geladen.", "place 1 loaded");
    check(marker(p.world) == "A2" && marker(p.store / "Letzte Sitzung") == "B", "place 1 is the world; B is the last run's");
    // Ctrl+Shift+0 once playing: the last run's world at the next start.
    check(ws::load_next_start(p, 0).find("n\xc3\xa4" "chsten Start") != std::string::npos, "loading waits for the next start");
    ws::prepare(p, std::nullopt);
    check(marker(p.world) == "B", "the last run's world back");
    check(ws::load_next_start(p, 7) == "Platz 7 ist leer.", "an empty place is said to be empty");
    fs::remove_all(root);
}
}

int main() {
    const std::pair<const char*, void (*)()> tests[] = {
        {"protocol layout", protocol_layout},     {"link state and input", link_state_and_input},
        {"coordinates", coordinates},             {"collision: flat ground and a kerb", collision_flat_ground_and_step},
        {"collision: the skater skipped", collision_skips_the_skater}, {"triangles across regions", triangles_span_regions},
        {"keys and buttons", keys_and_buttons},   {"player between ticks", player_between_ticks},
        {"overlay alpha", overlay_premultiplied}, {"blocks from Minecraft", blocks_from_minecraft},
        {"worlds kept and loaded", worlds_kept_and_loaded}};
    int failed = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "ok   " << name << '\n';
        } catch (const std::exception& e) {
            std::cout << "FAIL " << name << ": " << e.what() << '\n';
            ++failed;
        }
    }
    return failed ? 1 : 0;
}
