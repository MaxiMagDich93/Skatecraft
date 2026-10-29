#include "collision_feed.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>

namespace dingosdk::skatecraft {
namespace {
constexpr int region_size = 8; // SkyCraft's SkyCollision.REGION_SIZE
constexpr float nothing = std::numeric_limits<float>::quiet_NaN();

int floor_div(double value, int by) { return static_cast<int>(std::floor(value / by)); }
// Where sample i of a column starts along one axis, in blocks.
double sample_at(int column, int i, float step) { return static_cast<double>(column) * region_size + static_cast<double>(i) * step; }

// The triangle with its normal pointing up (or level): skate.'s ground is walked on from above,
// and mirroring the X axis turns every winding round.
proto::ColTri upward(const std::array<float, 3>& a, const std::array<float, 3>& b, const std::array<float, 3>& c) {
    const float ux = b[0] - a[0], uz = b[2] - a[2];
    const float vx = c[0] - a[0], vz = c[2] - a[2];
    const float ny = uz * vx - ux * vz; // y of (b - a) x (c - a)
    proto::ColTri tri{};
    const auto& second = ny >= 0 ? b : c;
    const auto& third = ny >= 0 ? c : b;
    for (int k = 0; k < 3; ++k) {
        tri.v[k] = a[k];
        tri.v[3 + k] = second[k];
        tri.v[6 + k] = third[k];
    }
    tri.flags = 0;
    return tri;
}
}

CollisionFeed::CollisionFeed(CollisionSettings settings) : s_(settings) {}

void CollisionFeed::clear(LinkView& link, std::uint32_t epoch) {
    epoch_ = epoch;
    scan_.reset();
    scanned_at_.clear();
    sent_.clear();
    waiting_.clear();
    link.write_collision(proto::kColClear, &epoch_, sizeof(epoch_));
}

std::map<int, std::vector<proto::ColTri>> CollisionFeed::triangulate(int cx, int cz, float step, const std::vector<float>& heights) {
    std::map<int, std::vector<proto::ColTri>> regions;
    const int n = static_cast<int>(std::lround(region_size / step));
    if (static_cast<int>(heights.size()) != (n + 1) * (n + 1)) return regions;
    const auto h = [&](int i, int j) { return heights[static_cast<std::size_t>(j * (n + 1) + i)]; };
    const auto point = [&](int i, int j) {
        return std::array<float, 3>{static_cast<float>(sample_at(cx, i, step)), h(i, j), static_cast<float>(sample_at(cz, j, step))};
    };
    const auto add = [&](const proto::ColTri& tri) {
        const float low = std::min({tri.v[1], tri.v[4], tri.v[7]}), high = std::max({tri.v[1], tri.v[4], tri.v[7]});
        for (int ry = floor_div(low, region_size); ry <= floor_div(high, region_size); ++ry) regions[ry].push_back(tri);
    };
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            if (std::isnan(h(i, j)) || std::isnan(h(i + 1, j)) || std::isnan(h(i, j + 1)) || std::isnan(h(i + 1, j + 1))) continue;
            const auto p00 = point(i, j), p10 = point(i + 1, j), p01 = point(i, j + 1), p11 = point(i + 1, j + 1);
            add(upward(p00, p10, p11));
            add(upward(p00, p11, p01));
        }
    }
    return regions;
}

bool CollisionFeed::send_column(LinkView& link, int cx, int cz, const std::map<int, std::vector<proto::ColTri>>& regions) {
    // Every region the column filled before is sent again, empty if need be, so what is gone goes.
    auto& before = sent_[{cx, cz}];
    std::set<int> rys(before.begin(), before.end());
    for (const auto& [ry, tris] : regions) rys.insert(ry);
    std::uint64_t bytes = 0;
    for (int ry : rys) {
        const auto found = regions.find(ry);
        const std::size_t count = found == regions.end() ? 0 : found->second.size();
        bytes += 2 * (sizeof(proto::ColMsgHeader) + sizeof(proto::ColRegion) + 8) + count * sizeof(proto::ColTri);
    }
    if (link.collision_room() < bytes + 4096) return false;
    std::vector<std::uint8_t> payload;
    for (int ry : rys) {
        const auto found = regions.find(ry);
        const std::size_t count = found == regions.end() ? 0 : found->second.size();
        proto::ColRegion header{};
        header.minX = cx * region_size;
        header.minY = ry * region_size;
        header.minZ = cz * region_size;
        header.maxX = header.minX + region_size - 1;
        header.maxY = header.minY + region_size - 1;
        header.maxZ = header.minZ + region_size - 1;
        header.epoch = epoch_;
        header.count = static_cast<std::uint32_t>(count);
        payload.resize(sizeof(header) + count * sizeof(proto::ColTri));
        std::memcpy(payload.data(), &header, sizeof(header));
        if (count) std::memcpy(payload.data() + sizeof(header), found->second.data(), count * sizeof(proto::ColTri));
        // SkyCraft sends a region's triangles first, then the region itself (no voxel blocks here:
        // skate.'s ground is only triangles), which marks it as known to Minecraft.
        link.write_collision(proto::kColTris, payload.data(), static_cast<std::uint32_t>(payload.size()));
        header.count = 0;
        link.write_collision(proto::kColRegion, &header, sizeof(header));
    }
    before.assign(rys.begin(), rys.end());
    return true;
}

std::optional<std::pair<int, int>> CollisionFeed::next_due(int pcx, int pcz, double now) const {
    std::optional<std::pair<int, int>> best;
    int best_distance = std::numeric_limits<int>::max();
    for (int dz = -s_.radius_columns; dz <= s_.radius_columns; ++dz) {
        for (int dx = -s_.radius_columns; dx <= s_.radius_columns; ++dx) {
            const std::pair<int, int> column{pcx + dx, pcz + dz};
            const auto found = scanned_at_.find(column);
            if (found != scanned_at_.end() && now - found->second < s_.refresh_seconds) continue;
            // Never-scanned columns come before stale ones, nearest first.
            const int distance = dx * dx + dz * dz + (found == scanned_at_.end() ? 0 : 1000);
            if (distance < best_distance) {
                best_distance = distance;
                best = column;
            }
        }
    }
    return best;
}

void CollisionFeed::tick(LinkView& link, const Mapping& mapping, const DVec3& player, const std::optional<Vec3>& skater, const RayFn& ray, double now) {
    if (!link.valid() || !ray) return;
    while (!waiting_.empty()) {
        auto& [column, regions] = waiting_.front();
        if (!send_column(link, column.first, column.second, regions)) return; // Minecraft is behind: wait
        waiting_.erase(waiting_.begin());
    }
    const int pcx = floor_div(player.x, region_size), pcz = floor_div(player.z, region_size);
    const int n = static_cast<int>(std::lround(region_size / s_.step));
    const std::size_t samples = static_cast<std::size_t>((n + 1) * (n + 1));
    std::optional<DVec3> skater_mc;
    if (skater) skater_mc = mapping.to_mc(*skater);
    int budget = s_.rays_per_tick;
    while (budget > 0) {
        if (scan_ && (std::abs(scan_->cx - pcx) > s_.radius_columns + 1 || std::abs(scan_->cz - pcz) > s_.radius_columns + 1)) scan_.reset();
        if (!scan_) {
            const auto due = next_due(pcx, pcz, now);
            if (!due) return;
            scan_ = Scan{due->first, due->second, static_cast<float>(player.y + s_.above), static_cast<float>(player.y - s_.below),
                         std::vector<float>(samples, nothing), 0};
        }
        auto& scan = *scan_;
        while (budget > 0 && scan.next < samples) {
            const int i = static_cast<int>(scan.next % static_cast<std::size_t>(n + 1)), j = static_cast<int>(scan.next / static_cast<std::size_t>(n + 1));
            const double x = sample_at(scan.cx, i, s_.step), z = sample_at(scan.cz, j, s_.step);
            ++scan.next;
            if (skater_mc && std::hypot(x - skater_mc->x, z - skater_mc->z) < s_.exclude_radius) continue; // filled in below
            --budget;
            ++rays_;
            if (const auto hit = ray(mapping.to_game({x, scan.top, z}), mapping.to_game({x, scan.bottom, z})))
                scan.heights[scan.next - 1] = static_cast<float>(mapping.to_mc(*hit).y);
        }
        if (scan.next < samples) return; // the rest next tick
        // Samples skipped beside the skater take their neighbours' heights, a few cells at most.
        if (skater_mc) {
            for (int pass = 0; pass < 8; ++pass) {
                bool changed = false;
                auto filled = scan.heights;
                for (int j = 0; j <= n; ++j) {
                    for (int i = 0; i <= n; ++i) {
                        const double x = sample_at(scan.cx, i, s_.step), z = sample_at(scan.cz, j, s_.step);
                        auto& here = filled[static_cast<std::size_t>(j * (n + 1) + i)];
                        if (!std::isnan(here) || std::hypot(x - skater_mc->x, z - skater_mc->z) >= s_.exclude_radius) continue;
                        float sum = 0;
                        int count = 0;
                        const int di[] = {1, -1, 0, 0}, dj[] = {0, 0, 1, -1};
                        for (int k = 0; k < 4; ++k) {
                            const int a = i + di[k], b = j + dj[k];
                            if (a < 0 || b < 0 || a > n || b > n) continue;
                            const float v = scan.heights[static_cast<std::size_t>(b * (n + 1) + a)];
                            if (std::isnan(v)) continue;
                            sum += v;
                            ++count;
                        }
                        if (count) {
                            here = sum / static_cast<float>(count);
                            changed = true;
                        }
                    }
                }
                scan.heights = std::move(filled);
                if (!changed) break;
            }
        }
        const std::pair<int, int> column{scan.cx, scan.cz};
        auto regions = triangulate(scan.cx, scan.cz, s_.step, scan.heights);
        // Every region the rays went through is sent, the empty ones too: Minecraft holds a player
        // still until the regions at and below their feet are known (SkyClient.holdUntilReady), and
        // the air above the ground is known to be empty only when it is sent.
        for (int ry = floor_div(scan.bottom, region_size); ry <= floor_div(scan.top, region_size); ++ry) regions[ry];
        scanned_at_[column] = now;
        scan_.reset();
        if (!send_column(link, column.first, column.second, regions)) {
            // Minecraft is not keeping up: hold a few, and scan the rest again later.
            if (waiting_.size() >= 32) {
                scanned_at_.erase(waiting_.front().first);
                waiting_.erase(waiting_.begin());
            }
            waiting_.emplace_back(column, std::move(regions));
            return;
        }
    }
}
}
