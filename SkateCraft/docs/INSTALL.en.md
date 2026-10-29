# SkateCraft – Installation guide (English)

This guide takes you from nothing to walking around skate.'s city as your Minecraft player. Do the
steps in order. You don't need any programming knowledge for it.

🇩🇪 Deutsche Anleitung: [INSTALL.de.md](INSTALL.de.md)

---

## 1. What you need

| | What | Why |
|---|---|---|
| ✅ | **skate.** on Steam (free to play, in your Steam library) | SkateCraft runs inside skate. |
| ✅ | **Minecraft: Java Edition** on your Microsoft account | You play as your own Minecraft player |
| ✅ | **Windows 10 or 11**, 64-bit | |
| ✅ | About **16 GB** of free disk space | ~14 GB for skate., ~1–2 GB for Minecraft |
| ✅ | Keyboard and mouse | In Minecraft mode only keyboard and mouse are passed on |

You do **not** need Skyrim, Visual Studio or anything else. You do not need to install ReSkate first
either – SkateCraft brings its own. If you already play with ReSkate, that's fine: your ReSkate is not
touched.

---

## 2. Download the installer

1. Open the releases page: **https://github.com/MaxiMagDich93/Skatecraft/releases**
2. At the top (the newest release), under **Assets**, click **`SkateCraft-Setup-<version>.exe`**.
3. Your browser saves it, usually in your **Downloads** folder.

> **"Windows protected your PC"?** The installer is not signed with a paid certificate, so Windows
> SmartScreen warns about it. Click **More info** and then **Run anyway**. (If you'd rather check
> first: the installer is built in public by GitHub from the source in this repository – see the
> *Actions* tab.)

---

## 3. Run the installer

Double-click `SkateCraft-Setup-<version>.exe`. It needs **no** administrator rights.

1. **Language** – pick *English* or *Deutsch*.
2. **License** – SkateCraft is free software under the GPL-3.0 (like ReSkate). Click *I accept*, then
   *Next*.
3. **Information** – a short list of what you need and what gets installed. *Next*.
4. **Folder** – where SkateCraft's ReSkate (and its copy of skate.) goes. The default is
   `C:\Games\SkateCraft`. Pick a drive with about 14 GB free. **Don't** pick Steam's skate. folder
   or the folder of your other ReSkate. *Next*.
5. **skate. game files** – choose one:
   - **Download skate. with ReSkate's launcher** – the normal choice. The game is downloaded the
     first time you start SkateCraft (Steam sign-in, ~14 GB).
   - **Take the game files from a ReSkate folder I already have** – only if you already play with
     ReSkate. Much faster. On the same drive it uses no extra space at all (the files are
     *hard-linked*); on another drive they are copied. On the next page pick the folder that contains
     `ReSkateLauncher.exe` and `Skate.exe` (for example `C:\Games\ReSkate`).
6. **Desktop shortcuts** – tick it if you want an icon on the desktop. *Next* → *Install*.
7. If you chose to take the game files, a blue PowerShell window shows the progress. Wait until it
   closes by itself.
8. **Finish** – leave **"Sign in to Minecraft now"** ticked and click *Finish*.

What was installed:

- `C:\Games\SkateCraft\` (or your folder): `ReSkateLauncher.exe` and `ReSkate.dll` with SkateCraft.
- `%LOCALAPPDATA%\SkateCraft\`: Prism Launcher with the **SkateCraft** Minecraft instance (Minecraft
  26.3 + Fabric + SkyCraft's mod). Your Minecraft worlds for SkateCraft are kept here too.
- Start menu entries: **SkateCraft (skate. with ReSkate)**, **SkateCraft Minecraft**, **SkateCraft
  guide**, **SkateCraft Minecraft folder**.

---

## 4. Sign in to Minecraft (once)

The installer opened **Prism Launcher** (a Minecraft launcher). You only do this once:

1. If Prism asks you to pick a language or theme, just click through with *Next* / *Finish*.
2. Top right, click **Accounts** → **Manage Accounts** → **Add Microsoft**.
3. Prism shows a **code** and a link. Open the link, enter the code and sign in with the Microsoft
   account that owns Minecraft: Java Edition.
4. When your name shows up in the account list, **close Prism**.

> You don't need to start Minecraft yourself – SkateCraft does it when you press **F8**. Prism
> downloads Minecraft and Java the first time that happens (that takes a minute or two).

Missed it? Open **SkateCraft Minecraft** from the Start menu and do the steps above.

---

## 5. First start of skate.

1. Start **SkateCraft (skate. with ReSkate)** from the Start menu (or the desktop).
2. ReSkate's launcher opens.
   - If you chose *Download*: it asks you to **sign in to Steam** (QR code with the Steam app, or
     name/password + Steam Guard) and then downloads skate. (~14 GB). Only the first time.
   - If you took the files from your ReSkate: it just checks them.
3. Click **PLAY**.

The launcher shows *"ReSkate development build"* – that's normal: SkateCraft's ReSkate never updates
itself, so an official ReSkate update can't overwrite SkateCraft.

---

## 6. Play

1. Wait until you can skate in the city.
2. Press **F8**.
   - SkateCraft says **"SkateCraft on"** and **"New Minecraft world …"**.
   - It starts Minecraft by itself (**"Starting Minecraft …"**). The first time it can take
     1–2 minutes. Minecraft's window stays hidden – that's on purpose.
   - Then: **"Minecraft has the player."** Now you are your Minecraft player in skate.'s world. 🎉
3. Press **F8** again to get your skater and board back. You continue skating from where you walked
   to.

### Controls in Minecraft mode

| Key | Does |
|---|---|
| **W A S D**, mouse | walk and look around |
| **Space** / **Shift** / **Ctrl** | jump / sneak / sprint |
| **Left mouse** / **Right mouse** | break / place blocks |
| **Mouse wheel**, **1–9** | choose the item in your hotbar |
| **E** | inventory |
| **T** | Minecraft chat (Esc closes it) |
| **O** | Minecraft's menu (options, …) |
| **F5** | Minecraft's camera modes |
| **Esc** | skate.'s menu (or closes an open Minecraft screen) |
| **F8** | back to skate. (and into Minecraft again) |
| **^** / **~** and **Insert** | ReSkate's console and menu, as always |

### Your Minecraft worlds

Every start of skate. begins with a **new** Minecraft world, so you always build on a clean city. To
keep a world:

| Keys | Does |
|---|---|
| **Ctrl + 1 … 9** | saves the current world in slot 1 … 9 |
| **Ctrl + Shift + 1 … 9** | loads slot 1 … 9 (before the first F8: right away; later: at the next start of skate.) |
| **Ctrl + Shift + 0** | loads the world of your last session |

The slots are folders in `%LOCALAPPDATA%\SkateCraft\Welten` ("Platz 1" … "Platz 9", "Letzte Sitzung").

### Console commands

Open the console with **^** (German keyboard) or **~** (US keyboard):

| Command | Does |
|---|---|
| `skatecraft on` / `skatecraft off` | same as F8 |
| `skatecraft status` | shows what SkateCraft is doing (handy when something doesn't work) |
| `skatecraft mirror 1` / `0` | mirrors Minecraft's X axis (try it if your world looks flipped) |
| `skatecraft resync` | sends skate.'s ground to Minecraft again |

---

## 7. If something doesn't work

| Problem | Try this |
|---|---|
| Windows won't start the installer | *More info* → *Run anyway* (see step 2). |
| **"Waiting for Minecraft …"** stays | Start **SkateCraft Minecraft** from the Start menu once. If Prism asks you to sign in, do step 4. Then press F8 twice. |
| Prism says no account / "not owned" | Sign in with the Microsoft account that **owns Minecraft: Java Edition** (step 4). |
| You can look around but not walk | Noclip must be **off** (ReSkate menu, *Insert* → Skater). SkateCraft says so if it's on. |
| The Minecraft HUD / inventory is missing | Press F8 twice. |
| Blocks show through walls | Known limit for now: SkateCraft draws the blocks over skate.'s picture. |
| The controller doesn't work in Minecraft mode | Known: only keyboard and mouse go to Minecraft. Use the controller for skating. |
| Anything else | Run `skatecraft status` in the console, take a screenshot, and open an issue with the two logs: `ReSkate.log` (in `logs\` of your SkateCraft folder) and `latest.log` (in `%LOCALAPPDATA%\SkateCraft\Prism\instances\SkateCraft\.minecraft\logs`). |

Issues: **https://github.com/MaxiMagDich93/Skatecraft/issues**

> Please don't report SkateCraft problems to ReSkate or SkyCraft – they didn't make SkateCraft.

---

## 8. Uninstall

Windows *Settings* → *Apps* → **SkateCraft** → *Uninstall*. This removes the files the installer put
there. The skate. game files the launcher downloaded and your Minecraft data in
`%LOCALAPPDATA%\SkateCraft` (worlds, Prism sign-in) stay – delete those folders by hand if you want them
gone. Your other ReSkate is not affected.

---

## 9. Without the installer (portable zip)

Each release also has `SkateCraft-<version>-portable.zip`:

1. Unpack it.
2. Put `ReSkate\ReSkateLauncher.exe` and `ReSkate\ReSkate.dll` into a new, empty folder (e.g.
   `C:\Games\SkateCraft`).
3. Copy everything inside `Minecraft\` into `%LOCALAPPDATA%\SkateCraft` (type that into the Explorer
   address bar). Copy `defaults\prismlauncher.cfg` into `%LOCALAPPDATA%\SkateCraft\Prism\`.
4. Continue with step 4 of this guide (sign in with `Prism\prismlauncher.exe`).

---

## Credits

SkateCraft only exists because of two great projects – go give them a ⭐:

- **[ReSkate](https://github.com/Dingo-Shenanigans/ReSkate)** by **Dingo-Shenanigans** – the launcher,
  runtime and mod platform for skate. that SkateCraft is built into. GPL-3.0.
- **[SkyCraft](https://github.com/chasmlol/SkyCraft)** by **chasmlol** – Minecraft in Skyrim. SkateCraft
  uses SkyCraft's Minecraft mod unchanged and speaks its shared-memory protocol. MIT.

Also included: [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) (GPL-3.0),
[Fabric API](https://github.com/FabricMC/fabric) (Apache-2.0),
[e4mc](https://github.com/vgskye/e4mc-minecraft-architectury) (MIT).

SkateCraft is a fan project. It is not affiliated with or endorsed by Electronic Arts, Full Circle,
Mojang or Microsoft, nor by the ReSkate or SkyCraft developers.
