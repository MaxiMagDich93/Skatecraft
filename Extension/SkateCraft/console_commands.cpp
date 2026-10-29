#include "skatecraft.h"
#include "Extension/Console/commands.h"

// `skatecraft on|off`, `skatecraft status`, `skatecraft mirror 0|1`, `skatecraft resync`.
namespace dingosdk::console {
void register_skatecraft_commands(Commands& registry) {
    auto enabled = variable("skatecraft", "Walk as your Minecraft player (SkyCraft's Minecraft mod must be running); F8 switches it too",
                            Group::gameplay, argument("on|off", Type::boolean));
    enabled.inspect = [](const Model&) { return boolean_state(true, skatecraft::enabled()); };
    enabled.run = [](const Model&, const Values& args, const Output& out) { out(skatecraft::set_enabled(std::get<bool>(args[0]))); };
    registry.add(std::move(enabled));

    auto status = action("skatecraft status", "SkateCraft: Minecraft's connection, the player's position and the collision sent", Group::gameplay);
    status.run = [](const Model&, const Values&, const Output& out) { out(skatecraft::status()); };
    registry.add(std::move(status));

    auto mirror = action("skatecraft mirror", "Mirror skate.'s X axis in Minecraft (set it when strafing left moves you right)", Group::gameplay,
                         {argument("0|1", Type::boolean)});
    mirror.run = [](const Model&, const Values& args, const Output& out) { out(skatecraft::set_mirror_x(std::get<bool>(args[0]))); };
    registry.add(std::move(mirror));

    auto resync = action("skatecraft resync", "Send skate.'s ground to Minecraft again and put the player back at the skater", Group::gameplay);
    resync.run = [](const Model&, const Values&, const Output& out) { out(skatecraft::resync()); };
    registry.add(std::move(resync));
}
}
