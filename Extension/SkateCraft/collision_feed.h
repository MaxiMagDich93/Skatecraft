#pragma once
#include "coords.h"
#include "link.h"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <utility>
#include <vector>

// skate.'s ground, as Minecraft's player collides with it.
//
// SkyCraft's first collision stage was a grid of downward rays around the player, turned into
// triangles; its final stage reads Havok's shapes. SkateCraft starts with the same first stage,
// because ReSkate already has a ray into the game's physics world (local_surface_hit, the one
// reskate.world.raycast uses) and nothing that reads its shapes yet.
//
// The ground around the player is cut into columns of 8 x 8 blocks (SkyCraft's collision region,
// SkyCollision.REGION_SIZE). Each column is sampled every `step` blocks by a vertical ray from a
// little above the player's head down to well below the feet, so a bridge overhead is not ground
// and the deck you stand on is. Neighbouring samples make two triangles per cell, sent to
// Minecraft as kColTris messages, one per 8-block region they touch. Minecraft's TriCollider walks
// on them with its own physics; steep triangles (a curb, a ledge, a wall) are walls to it.
//
// Limits of this stage, to be lifted by reading the physics shapes later: no overhangs inside one
// column (the underside of a ledge you stand beside), walls are as thick as one sample cell, and
// thin things a ray can miss (a rail, a fence post) may be missing.
namespace dingosdk::skatecraft {

// A ray into skate.'s world: from, to (game metres); where it first meets collision, or nothing.
using RayFn = std::function<std::optional<Vec3>(const Vec3& from, const Vec3& to)>;

struct CollisionSettings {
    float step = 0.25f;         // blocks between samples; 8 / step must be a whole number
    int radius_columns = 2;     // columns each way around the player: 2 is 5 x 5 columns, 40 x 40 blocks
    float above = 3.0f;         // blocks above the feet each ray starts
    float below = 48.0f;        // blocks below the feet each ray ends
    int rays_per_tick = 600;    // budget per client tick
    double refresh_seconds = 5; // a scanned column is scanned again after this long
    float exclude_radius = 1.2f; // blocks around the (hidden) skater whose hits are ignored: its body and board
};

class CollisionFeed {
public:
    explicit CollisionFeed(CollisionSettings settings = {});
    // A new world (a level loaded, a teleport far away): Minecraft forgets everything it had.
    void clear(LinkView& link, std::uint32_t epoch);
    // Scans what is due near the player and sends what is finished. `skater` is where the hidden
    // skater is (game metres), whose own body the rays must not stand the player on.
    void tick(LinkView& link, const Mapping& mapping, const DVec3& player_mc, const std::optional<Vec3>& skater, const RayFn& ray, double now_seconds);

    [[nodiscard]] std::uint32_t epoch() const { return epoch_; }
    [[nodiscard]] std::size_t columns_known() const { return sent_.size(); }
    [[nodiscard]] std::uint64_t rays_cast() const { return rays_; }

    // The triangles of one column from its heights, bucketed by the Minecraft region (ry) they
    // touch. `heights` is (n + 1)^2 Minecraft y values, row by row along z; NaN where a ray met
    // nothing. Public for the tests.
    static std::map<int, std::vector<proto::ColTri>> triangulate(int cx, int cz, float step, const std::vector<float>& heights);

private:
    struct Scan {
        int cx{}, cz{};
        float top{}, bottom{}; // Minecraft y the rays run between
        std::vector<float> heights;
        std::size_t next{};
    };
    bool send_column(LinkView& link, int cx, int cz, const std::map<int, std::vector<proto::ColTri>>& regions);
    std::optional<std::pair<int, int>> next_due(int pcx, int pcz, double now) const;

    CollisionSettings s_;
    std::uint32_t epoch_{1};
    std::optional<Scan> scan_;
    std::map<std::pair<int, int>, double> scanned_at_;      // column -> when its scan finished
    std::map<std::pair<int, int>, std::vector<int>> sent_;  // column -> the regions (ry) it last filled
    std::vector<std::pair<std::pair<int, int>, std::map<int, std::vector<proto::ColTri>>>> waiting_; // ring was full
    std::uint64_t rays_{};
};
}
