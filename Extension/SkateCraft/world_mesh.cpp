#include "world_mesh.h"
#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <tuple>

namespace dingosdk::skatecraft::world_mesh {
namespace {
// Render-ring message types (skycraft_protocol.h RenType), repeated so this file needs no protocol.
enum : std::uint32_t { ren_atlas = 1, ren_section = 2, ren_clear_all = 3 };
// The largest picture and mesh taken: a section holds 4096 blocks of 6 faces of 2 triangles.
constexpr std::uint32_t most_atlas_side = 16384;
constexpr std::uint32_t most_section_vertices = 4096 * 6 * 6;

using Key = std::tuple<std::int32_t, std::int32_t, std::int32_t>;
struct Shared {
    std::mutex mutex;
    std::map<Key, std::shared_ptr<const Section>> sections;
    std::shared_ptr<const std::vector<std::shared_ptr<const Section>>> list; // rebuilt when sections change
    std::shared_ptr<const Atlas> atlas;
    std::optional<Box> selection;
    bool mirror_x{};
    std::atomic<bool> shown{};
};
Shared& shared() {
    static auto* value = new Shared;
    return *value;
}

void take_section(Shared& s, const std::uint8_t* p, std::uint32_t bytes) {
    if (bytes < 16) return;
    std::int32_t sx{}, sy{}, sz{};
    std::uint32_t count{};
    std::memcpy(&sx, p, 4);
    std::memcpy(&sy, p + 4, 4);
    std::memcpy(&sz, p + 8, 4);
    std::memcpy(&count, p + 12, 4);
    const Key key{sx, sy, sz};
    if (count == 0) {
        std::lock_guard lock(s.mutex);
        if (s.sections.erase(key)) s.list.reset();
        return;
    }
    if (count > most_section_vertices || count % 3 || bytes < 16 + std::uint64_t{count} * sizeof(Vertex)) return;
    auto section = std::make_shared<Section>();
    section->sx = sx;
    section->sy = sy;
    section->sz = sz;
    const auto* vertices = p + 16;
    for (std::uint32_t t = 0; t < count; t += 3) {
        Vertex tri[3];
        std::memcpy(tri, vertices + std::size_t{t} * sizeof(Vertex), sizeof tri);
        auto& to = (tri[0].flags & 2u) ? section->translucent : section->solid;
        to.insert(to.end(), tri, tri + 3);
    }
    std::lock_guard lock(s.mutex);
    s.sections[key] = std::move(section);
    s.list.reset();
}

void take_atlas(Shared& s, const std::uint8_t* p, std::uint32_t bytes) {
    if (bytes < 8) return;
    std::uint32_t width{}, height{};
    std::memcpy(&width, p, 4);
    std::memcpy(&height, p + 4, 4);
    if (!width || !height || width > most_atlas_side || height > most_atlas_side) return;
    const std::uint64_t size = std::uint64_t{width} * height * 4;
    if (bytes < 8 + size) return;
    auto atlas = std::make_shared<Atlas>();
    atlas->width = width;
    atlas->height = height;
    atlas->rgba.assign(p + 8, p + 8 + size);
    std::lock_guard lock(s.mutex);
    s.atlas = std::move(atlas);
}
}

void receive(std::uint32_t type, const std::uint8_t* payload, std::uint32_t bytes) {
    auto& s = shared();
    switch (type) {
    case ren_section: take_section(s, payload, bytes); break;
    case ren_atlas: take_atlas(s, payload, bytes); break;
    case ren_clear_all: {
        std::lock_guard lock(s.mutex);
        s.sections.clear();
        s.list.reset();
        break;
    }
    default: break; // animated sprites, the player's body, other entities, lights: not drawn yet
    }
}

void clear() {
    auto& s = shared();
    std::lock_guard lock(s.mutex);
    s.sections.clear();
    s.list.reset();
    s.selection.reset();
}

void set_shown(bool shown) { shared().shown.store(shown, std::memory_order_release); }

void set_mirror(bool mirror_x) {
    auto& s = shared();
    std::lock_guard lock(s.mutex);
    s.mirror_x = mirror_x;
}

void set_selection(std::optional<Box> selection) {
    auto& s = shared();
    std::lock_guard lock(s.mutex);
    s.selection = selection;
}

std::size_t section_count() {
    auto& s = shared();
    std::lock_guard lock(s.mutex);
    return s.sections.size();
}

Scene snapshot() {
    auto& s = shared();
    std::lock_guard lock(s.mutex);
    if (!s.list) {
        auto list = std::make_shared<std::vector<std::shared_ptr<const Section>>>();
        list->reserve(s.sections.size());
        for (const auto& [key, section] : s.sections) list->push_back(section);
        s.list = std::move(list);
    }
    return {s.shown.load(std::memory_order_acquire), s.mirror_x, s.atlas, s.list, s.selection};
}

bool shown() { return shared().shown.load(std::memory_order_acquire); }
}
