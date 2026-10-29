#include "world_saves.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace dingosdk::skatecraft::world_saves {
namespace fs = std::filesystem;
namespace {
// What the next start should do (written now, done then): "keep=N" and "load=N" lines.
struct Plan {
    std::optional<int> keep, load;
};
fs::path plan_file(const Paths& p) { return p.store / "naechster-start.txt"; }

Plan read_plan(const Paths& p) {
    Plan plan;
    std::ifstream in(plan_file(p));
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() != 6 || line[5] < '0' || line[5] > '9') continue;
        if (line.starts_with("keep=")) plan.keep = line[5] - '0';
        if (line.starts_with("load=")) plan.load = line[5] - '0';
    }
    return plan;
}

void write_plan(const Paths& p, const Plan& plan) {
    std::error_code ignored;
    fs::create_directories(p.store, ignored);
    std::ofstream out(plan_file(p), std::ios::trunc);
    if (plan.keep) out << "keep=" << *plan.keep << '\n';
    if (plan.load) out << "load=" << *plan.load << '\n';
}

fs::path place_path(const Paths& p, int place) { return p.store / place_name(place); }

// A world copied file by file. session.lock is left out: Minecraft holds it locked while the world is
// open, and makes a new one when it opens the copy. False when a file could not be copied.
bool copy_world(const fs::path& from, const fs::path& to) {
    std::error_code e;
    fs::remove_all(to, e);
    if (!fs::create_directories(to, e) && e) return false;
    bool all = true;
    for (auto it = fs::recursive_directory_iterator(from, e); !e && it != fs::recursive_directory_iterator(); it.increment(e)) {
        const auto relative = fs::relative(it->path(), from, e);
        if (e) return false;
        const auto target = to / relative;
        if (it->is_directory(e)) {
            fs::create_directories(target, e);
        } else if (it->path().filename() != "session.lock") {
            fs::copy_file(it->path(), target, fs::copy_options::overwrite_existing, e);
        }
        if (e) {
            all = false;
            e.clear();
        }
    }
    return all && !e;
}

// The world moved to `to` (whatever was there goes): a rename, or a copy when a rename cannot be done.
bool move_world(const fs::path& from, const fs::path& to) {
    std::error_code e;
    fs::remove_all(to, e);
    fs::create_directories(to.parent_path(), e);
    fs::rename(from, to, e);
    if (!e) return true;
    if (!copy_world(from, to)) return false;
    fs::remove_all(from, e);
    return !e;
}
}

std::string place_name(int place) { return place == 0 ? std::string("Letzte Sitzung") : "Platz " + std::to_string(place); }

bool place_exists(const Paths& p, int place) {
    std::error_code e;
    return !p.store.empty() && fs::exists(place_path(p, place) / "level.dat", e);
}

Paths default_paths() {
#ifdef _WIN32
    wchar_t local[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (!length || length >= MAX_PATH) return {};
    const fs::path root = fs::path(local) / L"SkateCraft";
    return {root / L"Prism" / L"instances" / L"SkateCraft" / L".minecraft" / L"saves" / L"SkyCraft", root / L"Welten"};
#else
    return {};
#endif
}

std::string prepare(const Paths& p, std::optional<int> load_now) {
    if (p.world.empty() || p.store.empty()) return {};
    std::error_code e;
    const auto plan = read_plan(p);
    const auto load = load_now ? load_now : plan.load;
    std::string said;
    // Loading the last run's world: it is taken out of its place first, which the world being put
    // aside now goes into.
    const fs::path taken = p.store / "Letzte Sitzung (wird geladen)";
    if (load == 0 && place_exists(p, 0) && !move_world(place_path(p, 0), taken)) return "Die letzte Welt konnte nicht geladen werden.";
    if (fs::exists(p.world, e)) {
        if (plan.keep && !copy_world(p.world, place_path(p, *plan.keep)))
            said += "Die letzte Welt konnte nicht ganz in " + place_name(*plan.keep) + " gespeichert werden. ";
        if (!move_world(p.world, place_path(p, 0)))
            return "Die Minecraft-Welt ist noch offen (l\xc3\xa4" "uft ein altes Minecraft?). Sie bleibt diesmal, wie sie ist.";
    }
    write_plan(p, {});
    if (load) {
        const auto from = *load == 0 ? taken : place_path(p, *load);
        const bool there = fs::exists(from / "level.dat", e);
        if (there && (*load == 0 ? move_world(from, p.world) : copy_world(from, p.world))) return said + place_name(*load) + " geladen.";
        said += place_name(*load) + " gibt es nicht. ";
    }
    return said + "Neue Minecraft-Welt. Strg+1 bis 9 speichert sie, Strg+Umschalt+1 bis 9 l\xc3\xa4" "dt eine gespeicherte.";
}

std::string keep(const Paths& p, int place) {
    if (p.world.empty() || place < 1 || place > 9) return {};
    auto plan = read_plan(p);
    plan.keep = place;
    write_plan(p, plan);
    std::error_code e;
    if (!fs::exists(p.world, e)) return "Noch keine Welt zum Speichern: dr\xc3\xbc" "ck erst F8.";
    const bool now = copy_world(p.world, place_path(p, place));
    return "Welt in " + place_name(place) + " gespeichert" + (now ? "" : " (noch nicht ganz)") +
           ". Beim n\xc3\xa4" "chsten Start kommt der letzte Stand dazu.";
}

std::string load_next_start(const Paths& p, int place) {
    if (p.store.empty() || place < 0 || place > 9) return {};
    if (!place_exists(p, place)) return place_name(place) + " ist leer.";
    auto plan = read_plan(p);
    plan.load = place;
    write_plan(p, plan);
    return place_name(place) + " wird beim n\xc3\xa4" "chsten Start von skate. geladen.";
}
}
