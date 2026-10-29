#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// Minecraft's blocks as Minecraft's own block renderer built them, for drawing in skate.'s world.
//
// SkyCraft's Fabric mod sends, through the render ring of the shared memory, its block atlas (every
// block texture in one picture) and a mesh for each 16-block section with blocks in it: triangles
// with atlas coordinates, tint and ambient occlusion already worked out. In the mirror world that
// is exactly the blocks the player placed. The client tick takes them in here (receive); the
// overlay's render thread draws them (Extension/UI/Overlay/minecraft_world.cpp).
//
// Positions stay in Minecraft's coordinates; the drawing turns them into skate.'s (mirror_x).
namespace dingosdk::skatecraft::world_mesh {
// SkyCraft's RenVertex, byte for byte (skycraft_protocol.h): what the GPU reads.
struct Vertex {
    float x, y, z;       // Minecraft blocks, from the section's corner
    float u, v;          // in the atlas
    std::uint32_t color; // RGBA8, R in the lowest byte: tint times ambient occlusion
    std::uint32_t light; // lowest byte block light 0-15, next byte sky light 0-15
    std::uint32_t flags; // bit 0 cutout, bit 1 translucent, bits 4-6 the face (Direction ordinal + 1)
};
static_assert(sizeof(Vertex) == 32);

struct Section {
    std::int32_t sx{}, sy{}, sz{};        // which 16-block cube
    std::vector<Vertex> solid, translucent; // triangle lists: glass and water go in the second
};
struct Atlas {
    std::uint32_t width{}, height{};
    std::vector<std::uint8_t> rgba; // top row first
};
struct Box {
    std::array<float, 3> min{}, max{}; // Minecraft coordinates
};
struct Scene {
    bool shown{};
    bool mirror_x{};
    std::shared_ptr<const Atlas> atlas;
    std::shared_ptr<const std::vector<std::shared_ptr<const Section>>> sections;
    std::optional<Box> selection; // the block the crosshair is on
};

// ---- client thread -------------------------------------------------------------------------------
// One render-ring message (its payload is in shared memory: what is kept is copied).
void receive(std::uint32_t type, const std::uint8_t* payload, std::uint32_t bytes);
void clear();
void set_shown(bool shown);
void set_mirror(bool mirror_x);
void set_selection(std::optional<Box> selection);
// How many sections are held, for `skatecraft status`.
std::size_t section_count();

// ---- render thread -------------------------------------------------------------------------------
// The scene now: pointers to what was received, nothing copied.
Scene snapshot();
bool shown();
}
