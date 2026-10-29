#pragma once
#include <cstdint>
#include <vector>

// Minecraft's own picture of everything on top of the world: the hotbar, hearts, crosshair, the
// hand and held item, the inventory and every other Minecraft screen, chat. SkyCraft's Fabric mod
// draws these into the overlay slots of the shared memory (FrameExporter.java) with a clear
// background; SkateCraft lays that picture over skate.'s frame.
//
// The client tick takes the newest frame from the link (publish); the overlay's render thread
// takes it from here (take) and uploads it to a texture it draws over the whole screen
// (PATCHES.md, overlay_render.cpp). Only the newest frame is kept.
namespace dingosdk::skatecraft::overlay_feed {
struct Frame {
    std::uint32_t width{}, height{};
    std::uint64_t id{};
    // Top row first, straight alpha. Minecraft sends premultiplied alpha (SkyCraft's Overlay.cpp
    // blends it ONE / INV_SRC_ALPHA); ReSkate's ImGui draws straight alpha, so publish divides it out.
    std::vector<std::uint8_t> rgba;
};
// Client thread. `bottom_up`: the slot's rows run bottom to top (OverlaySlotHdr flags bit 0).
void publish(const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height, bool bottom_up, std::uint64_t id);
// Nothing to draw any more (SkateCraft off, Minecraft gone).
void clear();
// Render thread: the newest frame since the last take, into `out` (its buffer reused). False when
// there is none newer.
bool take(Frame& out);
// Render thread: whether anything should be drawn at all.
bool shown();
}
