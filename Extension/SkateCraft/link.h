#pragma once
#include "skycraft_protocol.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// The skate. end of the SkyCraft shared-memory link (External/skycraft_protocol). SkateCraft plays
// the part SkyCraft's SKSE plugin plays for Skyrim: it creates the mapping, and Minecraft (running
// SkyCraft's unchanged Fabric mod) opens it. Adapted from SkyCraft's skse/src/Link.cpp (MIT,
// chasmlol); the ring and seqlock code is the same, so both mods keep speaking one protocol.
//
// The mapping's name is not SkyCraft's own (Local\SkyCraft_v1), so a Skyrim with SkyCraft and a
// skate. with SkateCraft never share one. Minecraft is pointed at it with the JVM option
// -Dskycraft.link=Local\SkateCraft_v1 (SkyCraft's Proto.MAPPING_NAME reads it).
namespace dingosdk::skatecraft {
namespace proto = skycraft::proto;
inline constexpr wchar_t mapping_name[] = L"Local\\SkateCraft_v1";

// The protocol over any block of kMappingBytes bytes. The tests give it plain memory; in the game
// it is the shared mapping (Link below).
class LinkView {
public:
    LinkView() = default;
    explicit LinkView(std::uint8_t* base) : base_(base) {}
    [[nodiscard]] bool valid() const { return base_ != nullptr; }
    [[nodiscard]] std::uint8_t* base() const { return base_; }

    // Puts everything this side owns back to its start (rings empty, overlay swap reset) and
    // stamps the header, as a fresh skate. run must after Minecraft kept an old mapping open.
    void reset_own_regions(std::uint32_t pid, std::uint64_t now_ms);
    void heartbeat(std::uint64_t now_ms);
    [[nodiscard]] bool minecraft_alive(std::uint64_t now_ms) const;
    [[nodiscard]] std::uint32_t minecraft_pid() const;

    // skate. -> Minecraft (seqlock, once a frame: Minecraft paces itself on its seq).
    void write_sky_state(const proto::SkyState& state);
    // Minecraft -> skate. (seqlock). False when no steady copy could be had.
    bool read_mc_state(proto::McState& out) const;
    // Input ring (producer). Drops the event when Minecraft is a whole ring behind.
    void push_input(proto::InputType type, std::uint16_t code, std::int32_t a = 0, std::int32_t b = 0, std::int32_t c = 0);
    // Collision ring (producer, one thread). False when the ring is full: send it again later.
    bool write_collision(proto::ColType type, const void* payload, std::uint32_t bytes);
    // Free bytes in the collision ring, for pacing big sends.
    [[nodiscard]] std::uint64_t collision_room() const;
    // Event ring (consumer). False when empty.
    bool pop_event(proto::McEvent& out);
    // Render ring (consumer): fn(type, payload, bytes) for each waiting message, about max_bytes.
    void drain_render(const std::function<void(std::uint32_t, const std::uint8_t*, std::uint32_t)>& fn, std::uint64_t max_bytes);
    // Overlay triple buffer (consumer): true when a newer frame became the front slot.
    bool acquire_overlay_frame();
    void reset_overlay();
    [[nodiscard]] const std::uint8_t* front_pixels() const;
    [[nodiscard]] const proto::OverlaySlotHdr* front_header() const;

private:
    template<class T> T* at(std::uint64_t offset) const { return reinterpret_cast<T*>(base_ + offset); }
    std::uint8_t* base_{};
    std::uint32_t overlay_front_{2};
};

// The shared mapping itself (Windows). One per process.
class Link {
public:
    static Link& get();
    // Creates (or reuses) the mapping and resets this side's regions. Empty when it worked, else why.
    std::string create();
    void close();
    [[nodiscard]] LinkView& view() { return view_; }
    [[nodiscard]] bool valid() const { return view_.valid(); }

private:
    void* mapping_{};
    LinkView view_;
};
}
