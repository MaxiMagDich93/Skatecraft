# SkateCraft

**Walk skate.'s city as your real Minecraft player – and build in it.**
Press **F8** in skate. and your skater hands over to your Minecraft character: walk, jump, break and
place blocks, open your inventory and chat – right on skate.'s streets, ramps and rooftops. Press F8
again and skate on from where you walked to.

🇩🇪 **Deutsch:** [README.de.md](README.de.md) · [Installationsanleitung](SkateCraft/docs/INSTALL.de.md)

## What is SkateCraft?

SkateCraft puts **Minecraft inside skate.** Not a copy of Minecraft and not Minecraft blocks pasted into
the game: the real Minecraft: Java Edition runs in the background, with *your* account and
a real Minecraft world – and skate. shows it to you.

- **Become your Minecraft player in skate.** – one key (**F8**) switches from your skater to your
  Minecraft character and back.
- **skate.'s city is your Minecraft world** – the streets, stairs, ramps, ledges and rooftops are solid
  ground for Minecraft. You walk up the stairs of a plaza or jump off a ledge.
- **Build in the city** – break and place real Minecraft blocks. They appear right in skate.'s world,
  with Minecraft's textures and lighting.
- **Everything Minecraft has** – hotbar, inventory, crafting, chat, the survival HUD, sprint, sneak,
  F5 camera modes.
- **Your worlds** – every session starts in a fresh world; save up to 9 worlds with **Ctrl+1…9** and
  load them again with **Ctrl+Shift+1…9**.
- **Skate on where you walked to** – press F8 and your skater appears where your Minecraft player
  stands.
- **One installer** – sets up its own ReSkate (your normal ReSkate stays untouched) and Minecraft with
  Prism Launcher. F8 even starts Minecraft for you.

## How does it work? (simple version)

Two games run at the same time and talk to each other many times per second:

1. **skate.** (with ReSkate and SkateCraft built in) shoots thousands of invisible rays down onto the
   city around you and tells Minecraft where the ground, walls and kerbs are. Minecraft turns that into
   invisible collision, so its player can walk on skate.'s world.
2. Your **keyboard and mouse** go to Minecraft while you are in Minecraft mode. skate. hides your
   skater and board.
3. **Minecraft** runs your player as usual – walking, jumping, gravity, breaking and placing blocks,
   the inventory – in its own hidden window.
4. Minecraft sends back **where your player is and where it looks**, the **blocks you built** (as 3D
   meshes with its texture atlas) and a picture of its **HUD** (hotbar, hearts, inventory, chat).
5. skate. puts its **camera at your Minecraft player's eyes**, draws the **blocks into the city** and the
   **HUD on top** – so what you see is skate.'s world, played as Minecraft.

The two talk through a block of shared memory on your PC (`Local\SkateCraft_v1`), using SkyCraft's
protocol. Nothing goes over the internet.

## Download

➡️ **[Latest release](https://github.com/MaxiMagDich93/Skatecraft/releases/latest)** – download
`SkateCraft-Setup-<version>.exe` and run it.

📖 **[Step-by-step installation guide (English)](SkateCraft/docs/INSTALL.en.md)**

You need: **skate.** on Steam, **Minecraft: Java Edition**, Windows 10/11 (64-bit), about 16 GB of
free space, keyboard and mouse. The installer sets up everything else:

- a **ReSkate of its own** with SkateCraft built in (your normal ReSkate stays untouched, and the
  installer can reuse its game files instead of downloading skate. again),
- **Minecraft** through a portable **Prism Launcher** with SkyCraft's mod, ready to go – skate. starts
  it for you when you press F8.

## Shoutouts ❤️

SkateCraft stands on the shoulders of two amazing projects. Please go and star them:

- **[ReSkate](https://github.com/Dingo-Shenanigans/ReSkate)** by **Dingo-Shenanigans** – offline play,
  multiplayer, mods and so much more for skate. SkateCraft is a fork of ReSkate; everything except
  SkateCraft itself is their work. ([README of ReSkate](README.ReSkate.md))
- **[SkyCraft](https://github.com/chasmlol/SkyCraft)** by **chasmlol** – the brilliant idea of playing
  Minecraft inside another game (Skyrim). SkateCraft uses SkyCraft's Minecraft mod unchanged and talks
  to it over SkyCraft's shared-memory protocol.

Thanks also to [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher),
[Fabric](https://fabricmc.net/) and [e4mc](https://github.com/vgskye/e4mc-minecraft-architectury).

## How it works (technical)

```
┌──────── Skate.exe + ReSkate.dll (with SkateCraft) ────────┐      ┌── Minecraft (Prism) + SkyCraft mod ──┐
│ sends skate.'s ground as collision, the camera size,       │ ───▶ │ Minecraft runs its own player,       │
│ your keys and mouse; hides the skater, holds it still      │      │ physics, blocks, inventory, chat     │
│ draws Minecraft's blocks in the world, its HUD on screen,  │ ◀─── │ sends the player, block meshes, the  │
│ and puts the camera at your Minecraft player's eyes        │      │ texture atlas and its HUD picture    │
└────────────────────────────────────────────────────────────┘      └──────────────────────────────────────┘
                         shared memory  Local\SkateCraft_v1  (SkyCraft protocol v11)
```

The code is in [`Extension/SkateCraft/`](Extension/SkateCraft) and the block renderer in
[`Extension/UI/Overlay/minecraft_world.cpp`](Extension/UI/Overlay/minecraft_world.cpp). The SkyCraft
protocol header is vendored in [`External/skycraft_protocol/`](External/skycraft_protocol) (MIT,
pinned to SkyCraft v0.1.2).

Known limits for now: blocks show through skate.'s walls (no shared depth yet), no third-person
avatar in F5, no animated block textures, and only keyboard and mouse reach Minecraft.

## Building from source

Windows, Visual Studio 2022 (C++ desktop, v143), CMake 3.24+, from a Developer PowerShell:

```powershell
cmake --preset vs2022-x64 -DDINGOSDK_BUILD_SKATECRAFT_TESTS=ON
cmake --build build/vs2022-x64 --config Release --target dingosdk_runtime dingosdk_launcher dingosdk_skatecraft_tests
build\vs2022-x64\Release\dingosdk_skatecraft_tests.exe
```

The installer is made by [`.github/workflows/build.yml`](.github/workflows/build.yml): it builds the
above, packs SkyCraft's Minecraft with
[`SkateCraft/installer/make_minecraft_bundle.ps1`](SkateCraft/installer/make_minecraft_bundle.ps1) and
compiles [`SkateCraft/installer/SkateCraft.iss`](SkateCraft/installer/SkateCraft.iss) with Inno
Setup 6. A push to `main` with a new version in [`SkateCraft/VERSION`](SkateCraft/VERSION) publishes
a release.

## License

SkateCraft is a fork of ReSkate and, like ReSkate, licensed under the **GNU GPL v3.0** ([LICENSE](LICENSE)).
The SkyCraft protocol header is MIT ([External/skycraft_protocol/LICENSE](External/skycraft_protocol/LICENSE)).
The Minecraft package in the installer keeps the licenses of its parts (Prism Launcher GPL-3.0,
Fabric API Apache-2.0, e4mc MIT, SkyCraft MIT) and carries their notices.

No game files are included: not from EA (skate.), nor from Mojang/Microsoft (Minecraft). You need your
own copies. SkateCraft is a fan project and is not affiliated with or endorsed by Electronic Arts, Full
Circle, Mojang or Microsoft – nor by the ReSkate or SkyCraft developers, so please report SkateCraft
problems [here](https://github.com/MaxiMagDich93/Skatecraft/issues), not to them.
