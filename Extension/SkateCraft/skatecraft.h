#pragma once
#include <cstdint>
#include <string>

// SkateCraft: walk skate.'s world as your Minecraft player, place and break blocks in it, and get
// back on the board whenever you like (README.md beside this folder).
//
// Minecraft runs beside skate. with SkyCraft's Fabric mod, unchanged, pointed at SkateCraft's
// shared memory (link.h). Minecraft is the player: its movement, physics, inventory, blocks and
// rendering of items are Minecraft's own. skate. is the world: SkateCraft tells Minecraft what the
// ground is shaped like (collision_feed.h) and draws the view from where Minecraft's player stands
// (player_drive.h). The keys and mouse go to Minecraft while SkateCraft is on (input_map.h).
//
// This is a mode, which ReSkate's AGENTS.md keeps out of the SDK itself ("The SDK offers general
// APIs, not modes"). It lives in a fork of ReSkate. The general pieces it needs (a shared-memory
// channel, drawing meshes in the world with the game's depth) are what a later version could
// offer as plugin APIs, with the mode itself as a Luau plugin.
namespace dingosdk::skatecraft {
// Each client tick, after the plugins' tick (Runtime/client_tick.cpp). `ready`: a level is playing
// (the client's named context is ready and no level request is loading). Not ready reaches
// Minecraft as a loading screen. `noclip`: the skater is held in noclip now (the debug model's), which
// is how it stays still while Minecraft has the player.
void tick(bool ready, bool noclip) noexcept;

// For the console (console_commands.cpp).
std::string set_enabled(bool on);
bool enabled();
std::string status();
std::string set_mirror_x(bool on);
std::string resync();
}
