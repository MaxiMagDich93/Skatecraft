#pragma once
#include "coords.h"
#include "link.h"
#include <array>
#include <cstdint>
#include <deque>
#include <optional>

// Where the Minecraft player is this frame, and the camera skate. draws from.
//
// Minecraft runs its physics 20 times a second; skate. draws maybe 144. SkyCraft's answer, taken
// over here: keep Minecraft's last few ticks (each with its start and end position and the
// QueryPerformanceCounter time it ran), draw one tick behind, and go from each tick's start to its
// end exactly as Minecraft's renderer does with its partial tick. No judder from the two games'
// frames running at different rates.
namespace dingosdk::skatecraft {

struct PlayerFrame {
    DVec3 feet{};       // Minecraft coordinates
    double eye_height{};
    bool on_ground{}, sneaking{}, swimming{}, flying{}, dead{}, screen_open{};
    std::uint32_t camera_mode{}; // 0 first person, 1 behind, 2 in front looking back (F5)
    float camera_distance{};
    float fov{70};
};

class PlayerDrive {
public:
    // A new McState read this frame (the same one again is fine).
    void observe(const proto::McState& state);
    void forget() { ticks_.clear(); }
    // The player at `now_qpc` (QueryPerformanceCounter), `qpc_per_ms` its rate. Empty until
    // Minecraft has sent a tick.
    [[nodiscard]] std::optional<PlayerFrame> at(std::int64_t now_qpc, double qpc_per_ms) const;

    // The camera for that frame and look, in skate.'s world: its pose (coords.h camera_pose) and
    // vertical field of view.
    struct Camera {
        std::array<float, 16> pose{};
        float fov{};
    };
    static Camera camera(const PlayerFrame& frame, const Look& look, const Mapping& mapping);

private:
    struct Tick {
        proto::McState s{};
        std::int64_t at{};
    };
    std::deque<Tick> ticks_;
    proto::McState latest_{};
};
}
