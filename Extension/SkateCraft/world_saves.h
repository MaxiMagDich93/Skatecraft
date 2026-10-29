#pragma once
#include <filesystem>
#include <optional>
#include <string>

// Which Minecraft world SkateCraft plays in.
//
// SkyCraft's mod always opens the world called "SkyCraft" in Minecraft's saves folder, the moment it
// links up (MirrorWorld.java), and makes a new one when there is none. So the choice is made on disk,
// just before skate. opens the shared memory for the first time in a run (when F8 is first pressed):
//
// - Normally the last run's world is moved aside to "Letzte Sitzung" and Minecraft starts a new one.
// - Ctrl+1 to Ctrl+9 keeps the world in place 1 to 9: copied now (as Minecraft last wrote it), and
//   again at the next start, when Minecraft has saved and closed it.
// - Ctrl+Shift+1 to 9 loads place 1 to 9 instead of a new world; Ctrl+Shift+0 the last run's world.
//   Before the first F8 of a run it happens at that F8; later, at the next start of skate.
//
// The places are folders in %LOCALAPPDATA%\SkateCraft\Welten, next to Minecraft's own install.
namespace dingosdk::skatecraft::world_saves {
struct Paths {
    std::filesystem::path world; // Minecraft's saves\SkyCraft
    std::filesystem::path store; // the places: "Platz 1" ... "Platz 9", "Letzte Sitzung"
};
// Where SetupMinecraft put them (%LOCALAPPDATA%\SkateCraft); empty paths when unknown.
Paths default_paths();

// "Platz 3", or "Letzte Sitzung" for 0.
std::string place_name(int place);
bool place_exists(const Paths& paths, int place);

// Start of a run, before Minecraft has opened a world: what was asked for last run is done, and the
// world is new, or `load_now`, or the place asked for last run. What happened, for the player.
std::string prepare(const Paths& paths, std::optional<int> load_now);
// Ctrl+place: copy the world to the place now, and again at the next start.
std::string keep(const Paths& paths, int place);
// Ctrl+Shift+place once the world is open: load it at the next start.
std::string load_next_start(const Paths& paths, int place);
}
