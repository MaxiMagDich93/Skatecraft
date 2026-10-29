# SkateCraft

**Lauf als deine echte Minecraft-Figur durch die Stadt von skate. – und bau darin.**
Drück in skate. **F8** und dein Skater übergibt an deine Minecraft-Figur: laufen, springen, Blöcke
abbauen und setzen, Inventar und Chat – direkt auf den Straßen, Rampen und Dächern von skate. Drück
wieder F8 und skate dort weiter, wo du hingelaufen bist.

🇬🇧 **English:** [README.md](README.md) · [Installation guide](SkateCraft/docs/INSTALL.en.md)

## Was ist SkateCraft?

SkateCraft bringt **Minecraft in skate.** Keine Nachbildung von Minecraft und keine reinkopierten
Minecraft-Blöcke: Das echte Minecraft: Java Edition läuft im Hintergrund, mit *deinem* Konto und einer
echten Minecraft-Welt – und skate. zeigt es dir.

- **Werde in skate. zu deiner Minecraft-Figur** – eine Taste (**F8**) wechselt vom Skater zu deiner
  Minecraft-Figur und zurück.
- **Die Stadt von skate. ist deine Minecraft-Welt** – Straßen, Treppen, Rampen, Kanten und Dächer sind
  für Minecraft fester Boden. Du läufst die Treppe einer Plaza hoch oder springst von einer Kante.
- **Bau in der Stadt** – echte Minecraft-Blöcke abbauen und setzen. Sie erscheinen direkt in der Welt
  von skate., mit den Texturen und dem Licht von Minecraft.
- **Alles von Minecraft** – Schnellleiste, Inventar, Crafting, Chat, Survival-HUD, Sprinten, Schleichen,
  F5-Kameras.
- **Deine Welten** – jede Sitzung beginnt in einer frischen Welt; bis zu 9 Welten speicherst du mit
  **Strg+1…9** und lädst sie mit **Strg+Umschalt+1…9** wieder.
- **Weiterskaten, wo du hingelaufen bist** – F8 drücken und dein Skater steht dort, wo deine
  Minecraft-Figur steht.
- **Ein Installer** – richtet ein eigenes ReSkate ein (dein normales ReSkate bleibt unberührt) und
  Minecraft mit dem Prism Launcher. F8 startet Minecraft sogar für dich.

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

## Wie funktioniert das? (einfach erklärt)

Zwei Spiele laufen gleichzeitig und reden viele Male pro Sekunde miteinander:

1. **skate.** (mit ReSkate und eingebautem SkateCraft) schießt tausende unsichtbare Strahlen nach unten
   auf die Stadt um dich herum und sagt Minecraft, wo Boden, Wände und Bordsteine sind. Minecraft macht
   daraus unsichtbare Kollision – so kann seine Figur auf der Welt von skate. laufen.
2. Deine **Tastatur und Maus** gehen im Minecraft-Modus an Minecraft. skate. versteckt Skater und Board.
3. **Minecraft** steuert deine Figur ganz normal – laufen, springen, Schwerkraft, Blöcke abbauen und
   setzen, Inventar – in seinem eigenen, versteckten Fenster.
4. Minecraft schickt zurück, **wo deine Figur steht und wohin sie schaut**, die **Blöcke, die du gebaut
   hast** (als 3D-Modelle mit den Texturen) und ein Bild seines **HUD** (Schnellleiste, Herzen, Inventar,
   Chat).
5. skate. setzt seine **Kamera auf die Augen deiner Minecraft-Figur**, malt die **Blöcke in die Stadt**
   und das **HUD darüber** – was du siehst, ist die Welt von skate., gespielt als Minecraft.

Beide reden über einen gemeinsamen Speicherbereich auf deinem PC (`Local\SkateCraft_v1`) mit dem
Protokoll von SkyCraft. Nichts geht übers Internet.

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
