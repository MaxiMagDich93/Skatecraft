# SkateCraft

**Lauf als deine echte Minecraft-Figur durch die Stadt von skate. – und bau darin.**
Drück in skate. **F8** und dein Skater übergibt an deine Minecraft-Figur: laufen, springen, Blöcke
abbauen und setzen, Inventar und Chat – direkt auf den Straßen, Rampen und Dächern von skate. Drück
wieder F8 und skate dort weiter, wo du hingelaufen bist.

🇬🇧 **English:** [README.md](README.md) · [Installation guide](SkateCraft/docs/INSTALL.en.md)

## Download

➡️ **[Neuestes Release](https://github.com/MaxiMagDich93/Skatecraft/releases/latest)** –
`SkateCraft-Setup-<version>.exe` herunterladen und ausführen.

📖 **[Schritt-für-Schritt-Installationsanleitung (Deutsch)](SkateCraft/docs/INSTALL.de.md)**

Du brauchst: **skate.** auf Steam, **Minecraft: Java Edition**, Windows 10/11 (64 Bit), etwa 16 GB
freien Platz, Tastatur und Maus. Den Rest richtet der Installer ein:

- ein **eigenes ReSkate** mit eingebautem SkateCraft (dein normales ReSkate bleibt unberührt, und der
  Installer kann dessen Spieldateien übernehmen, statt skate. noch einmal herunterzuladen),
- **Minecraft** über einen portablen **Prism Launcher** mit dem Mod von SkyCraft, fertig eingerichtet –
  skate. startet es für dich, wenn du F8 drückst.

## Shoutouts ❤️

SkateCraft steht auf den Schultern von zwei großartigen Projekten. Schaut sie euch an und gebt ihnen
einen Stern:

- **[ReSkate](https://github.com/Dingo-Shenanigans/ReSkate)** von **Dingo-Shenanigans** – Offline-Spiel,
  Multiplayer, Mods und vieles mehr für skate. SkateCraft ist ein Fork von ReSkate; alles außer
  SkateCraft selbst ist ihre Arbeit. ([README von ReSkate](README.ReSkate.md))
- **[SkyCraft](https://github.com/chasmlol/SkyCraft)** von **chasmlol** – die geniale Idee, Minecraft in
  einem anderen Spiel (Skyrim) zu spielen. SkateCraft nutzt den Minecraft-Mod von SkyCraft unverändert
  und spricht mit ihm über das Shared-Memory-Protokoll von SkyCraft.

Danke auch an [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher),
[Fabric](https://fabricmc.net/) und [e4mc](https://github.com/vgskye/e4mc-minecraft-architectury).

## Wie es funktioniert

skate. (mit ReSkate und SkateCraft) schickt den Boden der Stadt als Kollision, deine Tasten und die
Maus an Minecraft und versteckt den Skater. Minecraft rechnet deine Figur, die Physik, die Blöcke, das
Inventar und den Chat und schickt die Position, die Block-Modelle, die Texturen und sein HUD zurück.
skate. malt die Blöcke in die Welt, das HUD auf den Bildschirm und setzt die Kamera auf die Augen deiner
Minecraft-Figur. Beide reden über Shared Memory (`Local\SkateCraft_v1`, SkyCraft-Protokoll v11).

Bekannte Grenzen im Moment: Blöcke sieht man durch Wände von skate., kein Avatar in F5, keine
animierten Block-Texturen, und nur Tastatur und Maus gehen an Minecraft.

Wie man es selbst baut, steht im [englischen README](README.md#building-from-source).

## Lizenz

SkateCraft ist ein Fork von ReSkate und steht wie ReSkate unter der **GNU GPL v3.0** ([LICENSE](LICENSE)).
Das Protokoll von SkyCraft ist MIT ([External/skycraft_protocol/LICENSE](External/skycraft_protocol/LICENSE)).

Es sind keine Spieldateien enthalten – weder von EA (skate.) noch von Mojang/Microsoft (Minecraft). Du
brauchst deine eigenen. SkateCraft ist ein Fanprojekt und hat nichts mit Electronic Arts, Full Circle,
Mojang oder Microsoft zu tun – und auch nicht mit den Entwicklern von ReSkate oder SkyCraft. Melde
Probleme mit SkateCraft deshalb bitte [hier](https://github.com/MaxiMagDich93/Skatecraft/issues).
