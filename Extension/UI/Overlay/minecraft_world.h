#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi.h>

// SkateCraft's Minecraft blocks drawn in skate.'s world (Extension/SkateCraft/world_mesh.h): each
// section's mesh with Minecraft's block atlas, from the game camera as it is this frame
// (plugin_camera.h camera_now), over the game's finished frame and under the viewmodel, the screen
// look and ImGui. Then the outline of the block the crosshair is on.
//
// It has a depth buffer of its own, so blocks hide each other but not skate.'s world: a block behind
// a wall shows through it. Using the game's own depth (depth_probe.cpp finds it) is the next step.
namespace dingosdk::minecraft_world {
// Render thread: whether there is anything to draw this frame.
bool pending() noexcept;
struct Target {
    ID3D12Device* device{};
    ID3D12GraphicsCommandList* commands{};
    ID3D12Resource* buffer{};     // the back buffer, in the PRESENT state; left in it
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    ID3D12DescriptorHeap* srvs{}; // the overlay's shader-visible heap (already set on the list)
    unsigned slot{};              // the heap slot the atlas uses
    DXGI_FORMAT format{};
    int transfer{};               // the overlay's output transfer: 0 SDR, 1 scRGB, 2 HDR10
    float white_nits{80};
    std::uint64_t fence{};        // what the overlay's fence reaches when this frame's commands are done
    std::uint64_t (*gpu_done)(){}; // the fence's value the GPU has reached
};
// Records the blocks on the frame's command list. False when nothing was drawn.
bool record(const Target& target) noexcept;
// The overlay is tearing its graphics down (a resize, a lost device, stop).
void release() noexcept;
}
