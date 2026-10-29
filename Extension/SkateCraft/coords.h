#pragma once
#include <array>
#include <cmath>
#include <numbers>

// Where a point of skate.'s world is in Minecraft's, and back.
//
// skate. (Frostbite): metres, +Y up. Minecraft: blocks, +Y up, +Z south, yaw 0 facing +Z with
// forward (-sin yaw, 0, cos yaw), pitch positive looking down. One block is one metre, so the
// Minecraft player (1.8 blocks) stands as tall as a skater.
//
// What is not known yet, and is measured in phase 1 (README): whether Frostbite's +X is
// Minecraft's +X or its mirror image. The default is no mirror. If strafing left (A) in Minecraft
// moves the camera to the right in skate., set mirror_x. Everything else follows from that flag,
// including the winding of collision triangles (collision_feed.cpp fixes it per triangle).
namespace dingosdk::skatecraft {
using Vec3 = std::array<float, 3>;
struct DVec3 {
    double x{}, y{}, z{};
};

struct Mapping {
    // The skate. point that becomes Minecraft's mc_origin. Minecraft's mirror world (SkyCraft's
    // dimension) reaches from y -2032 to 2032, so skate.'s heights fit as they are.
    DVec3 origin{};
    DVec3 mc_origin{};
    bool mirror_x{};

    [[nodiscard]] DVec3 to_mc(const Vec3& game) const {
        const double x = game[0] - origin.x, y = game[1] - origin.y, z = game[2] - origin.z;
        return {mc_origin.x + (mirror_x ? -x : x), mc_origin.y + y, mc_origin.z + z};
    }
    [[nodiscard]] Vec3 to_game(const DVec3& mc) const {
        const double x = mc.x - mc_origin.x, y = mc.y - mc_origin.y, z = mc.z - mc_origin.z;
        return {static_cast<float>(origin.x + (mirror_x ? -x : x)), static_cast<float>(origin.y + y), static_cast<float>(origin.z + z)};
    }
    // Minecraft's look (degrees) as a direction in skate.'s world.
    [[nodiscard]] Vec3 forward_game(double yaw_deg, double pitch_deg) const {
        const double yaw = yaw_deg * std::numbers::pi / 180, pitch = pitch_deg * std::numbers::pi / 180;
        double x = -std::sin(yaw) * std::cos(pitch);
        const double y = -std::sin(pitch), z = std::cos(yaw) * std::cos(pitch);
        if (mirror_x) x = -x;
        return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    }
    // A skate. heading (radians, 0 faces +Z, forward (sin, 0, cos)) as Minecraft's yaw (degrees).
    [[nodiscard]] double yaw_from_heading(double heading) const {
        double fx = std::sin(heading);
        const double fz = std::cos(heading);
        if (mirror_x) fx = -fx;
        // Minecraft: forward x = -sin(yaw), z = cos(yaw).
        return std::atan2(-fx, fz) * 180 / std::numbers::pi;
    }
};

// Minecraft's mouse look: how many degrees one count of mouse movement turns the view, from its
// sensitivity option (0..1), as Minecraft's MouseHandler.turnPlayer works it out.
inline double degrees_per_count(double sensitivity) {
    const double f = sensitivity * 0.6 + 0.2;
    return f * f * f * 8.0 * 0.15;
}

struct Look {
    double yaw{}, pitch{};
    void turn(double dx, double dy, double sensitivity, bool invert_y = false) {
        const double k = degrees_per_count(sensitivity);
        yaw += dx * k;
        pitch += (invert_y ? -dy : dy) * k;
        if (pitch > 90) pitch = 90;
        if (pitch < -90) pitch = -90;
        yaw = std::remainder(yaw, 360.0);
    }
};

// A camera pose for ReSkate's camera (plugin_game.h set_camera): rows right, up, back and the
// position, as reskate.camera.set builds it.
inline std::array<float, 16> camera_pose(const Vec3& eye, const Vec3& forward) {
    const auto length = [](const Vec3& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    const auto cross = [](const Vec3& a, const Vec3& b) {
        return Vec3{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    };
    Vec3 back{-forward[0], -forward[1], -forward[2]};
    const float lb = length(back);
    for (auto& v : back) v /= (lb > 1e-6f ? lb : 1.0f);
    // Straight up or down: any right will do, and the world's +Z keeps it steady.
    const Vec3 hint = std::abs(back[1]) > 0.999f ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
    auto right = cross(hint, back);
    const float lr = length(right);
    for (auto& v : right) v /= (lr > 1e-6f ? lr : 1.0f);
    const auto up = cross(back, right);
    return {right[0], right[1], right[2], 0, up[0], up[1], up[2], 0, back[0], back[1], back[2], 0, eye[0], eye[1], eye[2], 1};
}
}
