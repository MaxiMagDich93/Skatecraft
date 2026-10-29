#include "player_drive.h"
#include <algorithm>
#include <cmath>

namespace dingosdk::skatecraft {

void PlayerDrive::observe(const proto::McState& state) {
    latest_ = state;
    if (state.tickQpc == 0 || state.tickMs <= 0) return;
    if (!ticks_.empty() && state.tickQpc < ticks_.back().at) ticks_.clear(); // Minecraft restarted
    if (ticks_.empty() || ticks_.back().at != state.tickQpc) ticks_.push_back({state, state.tickQpc});
    while (ticks_.size() > 6) ticks_.pop_front();
}

std::optional<PlayerFrame> PlayerDrive::at(std::int64_t now_qpc, double qpc_per_ms) const {
    if (ticks_.empty() || qpc_per_ms <= 0) return std::nullopt;
    const auto& last = ticks_.back().s;
    const double period = last.tickMs * qpc_per_ms;
    // One tick behind, so there is nearly always a tick to go through.
    const auto render = now_qpc - static_cast<std::int64_t>(std::llround(period));
    std::size_t i = 0;
    for (std::size_t k = ticks_.size(); k-- > 0;) {
        if (ticks_[k].at <= render) {
            i = k;
            break;
        }
    }
    const auto& tick = ticks_[i];
    const auto* next = i + 1 < ticks_.size() ? &ticks_[i + 1] : nullptr;
    const double ticks = static_cast<double>(render - tick.at) / period;
    const double t = std::clamp(ticks, 0.0, 1.0);
    const auto& s = tick.s;
    PlayerFrame f;
    f.feet = {s.prevX + (s.curX - s.prevX) * t, s.prevY + (s.curY - s.prevY) * t, s.prevZ + (s.curZ - s.prevZ) * t};
    f.eye_height = s.tickEyeO + (s.tickEye - s.tickEyeO) * t;
    if (ticks > 1.0 && next) {
        // Past this tick's end and the next one we have starts later: Minecraft ran a tick we never
        // saw. Carry on from this tick's end to the next one's start.
        const auto& n = next->s;
        const double gap = static_cast<double>(next->at) - (static_cast<double>(tick.at) + period);
        const double u = gap > 0 ? std::clamp((static_cast<double>(render) - (static_cast<double>(tick.at) + period)) / gap, 0.0, 1.0) : 1.0;
        f.feet = {s.curX + (n.prevX - s.curX) * u, s.curY + (n.prevY - s.curY) * u, s.curZ + (n.prevZ - s.curZ) * u};
        f.eye_height = s.tickEye + (n.tickEyeO - s.tickEye) * u;
    }
    const auto flags = latest_.flags;
    f.on_ground = (flags & proto::kMcOnGround) != 0;
    f.sneaking = (flags & proto::kMcSneaking) != 0;
    f.swimming = (flags & proto::kMcSwimming) != 0;
    f.flying = (flags & proto::kMcFlying) != 0;
    f.dead = (flags & proto::kMcDead) != 0;
    f.screen_open = (flags & proto::kMcScreenOpen) != 0;
    f.camera_mode = latest_.cameraMode;
    f.camera_distance = latest_.cameraDistance;
    f.fov = latest_.fovDeg > 1 ? latest_.fovDeg : 70.0f;
    return f;
}

PlayerDrive::Camera PlayerDrive::camera(const PlayerFrame& frame, const Look& look, const Mapping& mapping) {
    // Minecraft's eye; its third-person cameras sit back along the look (or in front, looking
    // back), as far as Minecraft's own zoom collision let them (cameraDistance).
    const DVec3 eye_mc{frame.feet.x, frame.feet.y + frame.eye_height, frame.feet.z};
    auto forward = mapping.forward_game(look.yaw, look.pitch);
    auto eye = mapping.to_game(eye_mc);
    if (frame.camera_mode != 0 && frame.camera_distance > 0) {
        const float d = frame.camera_distance;
        if (frame.camera_mode == 2) {
            for (int k = 0; k < 3; ++k) eye[k] += forward[k] * d;
            for (auto& v : forward) v = -v;
        } else {
            for (int k = 0; k < 3; ++k) eye[k] -= forward[k] * d;
        }
    }
    return {camera_pose(eye, forward), frame.fov};
}
}
