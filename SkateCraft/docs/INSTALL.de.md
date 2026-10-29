# SkateCraft – Installationsanleitung (Deutsch)

Mit dieser Anleitung kommst du von null bis dahin, dass du als deine Minecraft-Figur durch die Stadt
von skate. läufst. Mach die Schritte der Reihe nach. Programmierkenntnisse brauchst du keine.

🇬🇧 English guide: [INSTALL.en.md](INSTALL.en.md)

---

## 1. Was du brauchst

| | Was | Wofür |
|---|---|---|
| ✅ | **skate.** auf Steam (kostenlos, in deiner Steam-Bibliothek) | SkateCraft läuft in skate. |
| ✅ | **Minecraft: Java Edition** in deinem Microsoft-Konto | Du spielst als deine eigene Minecraft-Figur |
| ✅ | **Windows 10 oder 11**, 64 Bit | |
| ✅ | Etwa **16 GB** freier Speicherplatz | ca. 14 GB für skate., ca. 1–2 GB für Minecraft |
| ✅ | Tastatur und Maus | Im Minecraft-Modus werden nur Tastatur und Maus weitergegeben |

Du brauchst **kein** Skyrim, kein Visual Studio und sonst nichts. Du musst auch ReSkate nicht vorher
installieren – SkateCraft bringt sein eigenes mit. Wenn du schon mit ReSkate spielst, ist das kein
Problem: Dein ReSkate wird nicht angefasst.

---

## 2. Installer herunterladen

1. Öffne die Release-Seite: **https://github.com/MaxiMagDich93/Skatecraft/releases**
2. Ganz oben (das neueste Release) unter **Assets** auf **`SkateCraft-Setup-<version>.exe`** klicken.
3. Dein Browser speichert die Datei, meistens im Ordner **Downloads**.

> **„Der Computer wurde durch Windows geschützt“?** Der Installer ist nicht mit einem kostenpflichtigen
> Zertifikat signiert, deshalb warnt Windows SmartScreen. Klick auf **Weitere Informationen** und dann
> auf **Trotzdem ausführen**. (Wer vorher nachsehen will: Der Installer wird öffentlich von GitHub aus
> dem Quellcode in diesem Repository gebaut – siehe Reiter *Actions*.)

---

## 3. Installer ausführen

Doppelklick auf `SkateCraft-Setup-<version>.exe`. Du brauchst **keine** Administratorrechte.

1. **Sprache** – *Deutsch* oder *English* auswählen.
2. **Lizenz** – SkateCraft ist freie Software unter der GPL-3.0 (wie ReSkate). *Ich akzeptiere* und
   *Weiter*.
3. **Information** – eine kurze Liste, was du brauchst und was installiert wird. *Weiter*.
4. **Ordner** – hier landet das ReSkate von SkateCraft (und seine Kopie von skate.). Vorgeschlagen ist
   `C:\Games\SkateCraft`. Nimm eine Festplatte mit etwa 14 GB frei. Wähle **nicht** den skate.-Ordner
   von Steam und nicht den Ordner deines anderen ReSkate. *Weiter*.
5. **Spieldateien von skate.** – eins auswählen:
   - **skate. mit dem ReSkate-Launcher herunterladen** – die normale Wahl. Das Spiel wird beim ersten
     Start von SkateCraft heruntergeladen (Steam-Anmeldung, ca. 14 GB).
   - **Spieldateien aus einem ReSkate-Ordner nehmen, den ich schon habe** – nur, wenn du schon mit
     ReSkate spielst. Viel schneller. Auf derselben Festplatte braucht das gar keinen zusätzlichen
     Platz (die Dateien werden *fest verlinkt*), auf einer anderen werden sie kopiert. Auf der nächsten
     Seite wählst du den Ordner, in dem `ReSkateLauncher.exe` und `Skate.exe` liegen (zum Beispiel
     `D:\Spiele\Reskate`).
6. **Verknüpfungen auf dem Desktop** – anhaken, wenn du ein Symbol auf dem Desktop willst. *Weiter* →
   *Installieren*.
7. Wenn du die Spieldateien übernehmen lässt, zeigt ein blaues PowerShell-Fenster den Fortschritt.
   Warte, bis es sich von selbst schließt.
8. **Fertigstellen** – lass **„Jetzt bei Minecraft anmelden“** angehakt und klick auf *Fertigstellen*.

Was installiert wurde:

- `C:\Games\SkateCraft\` (oder dein Ordner): `ReSkateLauncher.exe` und `ReSkate.dll` mit SkateCraft.
- `%LOCALAPPDATA%\SkateCraft\`: der Prism Launcher mit der Minecraft-Instanz **SkateCraft** (Minecraft
  26.3 + Fabric + der Mod von SkyCraft). Deine Minecraft-Welten für SkateCraft liegen auch hier.
- Einträge im Startmenü: **SkateCraft (skate. mit ReSkate)**, **SkateCraft Minecraft**, **SkateCraft
  Anleitung**, **SkateCraft Minecraft-Ordner**.

---

## 4. Bei Minecraft anmelden (einmalig)

Der Installer hat den **Prism Launcher** geöffnet (ein Minecraft-Launcher). Das machst du nur einmal:

1. Wenn Prism nach Sprache oder Design fragt, einfach mit *Weiter* / *Fertig* durchklicken.
2. Oben rechts auf **Konten** (Accounts) → **Konten verwalten** → **Microsoft hinzufügen** klicken.
3. Prism zeigt einen **Code** und einen Link. Link öffnen, Code eingeben und mit dem Microsoft-Konto
   anmelden, dem Minecraft: Java Edition gehört.
4. Sobald dein Name in der Kontoliste steht: **Prism schließen**.

> Minecraft musst du nicht selbst starten – das macht SkateCraft, wenn du **F8** drückst. Prism lädt
> beim ersten Mal Minecraft und Java herunter (das dauert ein, zwei Minuten).

Verpasst? Starte **SkateCraft Minecraft** aus dem Startmenü und mach die Schritte oben.

---

## 5. Erster Start von skate.

1. Starte **SkateCraft (skate. mit ReSkate)** aus dem Startmenü (oder vom Desktop).
2. Der ReSkate-Launcher geht auf.
   - Wenn du *Herunterladen* gewählt hast: Er bittet dich, dich **bei Steam anzumelden** (QR-Code mit
     der Steam-App oder Name/Passwort + Steam Guard), und lädt dann skate. herunter (ca. 14 GB). Nur
     beim ersten Mal.
   - Wenn du die Dateien aus deinem ReSkate übernommen hast: Er prüft sie nur.
3. Klick auf **PLAY**.

Im Launcher steht *„ReSkate development build“* – das ist richtig so: Das ReSkate von SkateCraft
aktualisiert sich nie selbst, damit ein offizielles ReSkate-Update SkateCraft nicht überschreiben kann.

---

## 6. Spielen

1. Warte, bis du in der Stadt skaten kannst.
2. Drück **F8**.
   - SkateCraft meldet **„SkateCraft on“** und **„Neue Minecraft-Welt …“**.
   - Es startet Minecraft von selbst (**„Starting Minecraft …“**). Beim ersten Mal kann das 1–2 Minuten
     dauern. Das Minecraft-Fenster bleibt versteckt – das ist Absicht.
   - Dann: **„Minecraft has the player.“** Jetzt bist du deine Minecraft-Figur in der Welt von skate. 🎉
3. Drück wieder **F8**, um Skater und Board zurückzubekommen. Du skatest dort weiter, wo du hingelaufen
   bist.

### Steuerung im Minecraft-Modus

| Taste | Macht |
|---|---|
| **W A S D**, Maus | laufen und umschauen |
| **Leertaste** / **Umschalt** / **Strg** | springen / schleichen / sprinten |
| **Linke Maustaste** / **Rechte Maustaste** | Blöcke abbauen / setzen |
| **Mausrad**, **1–9** | Gegenstand in der Schnellleiste wählen |
| **E** | Inventar |
| **T** | Minecraft-Chat (Esc schließt ihn) |
| **O** | Minecraft-Menü (Optionen, …) |
| **F5** | Kameramodi von Minecraft |
| **Esc** | Menü von skate. (oder schließt einen offenen Minecraft-Bildschirm) |
| **F8** | zurück zu skate. (und wieder zu Minecraft) |
| **^** und **Einfg** | Konsole und Menü von ReSkate, wie immer |

### Deine Minecraft-Welten

Jeder Start von skate. beginnt mit einer **neuen** Minecraft-Welt, damit du immer auf einer sauberen
Stadt baust. Um eine Welt zu behalten:

| Tasten | Macht |
|---|---|
| **Strg + 1 … 9** | speichert die aktuelle Welt auf Platz 1 … 9 |
| **Strg + Umschalt + 1 … 9** | lädt Platz 1 … 9 (vor dem ersten F8: sofort; danach: beim nächsten Start von skate.) |
| **Strg + Umschalt + 0** | lädt die Welt deiner letzten Sitzung |

Die Plätze sind Ordner in `%LOCALAPPDATA%\SkateCraft\Welten` („Platz 1“ … „Platz 9“, „Letzte Sitzung“).

### Konsolenbefehle

Konsole mit **^** öffnen:

| Befehl | Macht |
|---|---|
| `skatecraft on` / `skatecraft off` | wie F8 |
| `skatecraft status` | zeigt, was SkateCraft gerade macht (praktisch, wenn etwas nicht geht) |
| `skatecraft mirror 1` / `0` | spiegelt die X-Achse von Minecraft (probier es, wenn deine Welt verdreht aussieht) |
| `skatecraft resync` | schickt den Boden von skate. noch einmal an Minecraft |

---

## 7. Wenn etwas nicht klappt

| Problem | Versuch das |
|---|---|
| Windows startet den Installer nicht | *Weitere Informationen* → *Trotzdem ausführen* (siehe Schritt 2). |
| **„Waiting for Minecraft …“** bleibt stehen | Starte einmal **SkateCraft Minecraft** aus dem Startmenü. Wenn Prism nach einer Anmeldung fragt, mach Schritt 4. Dann zweimal F8 drücken. |
| Prism sagt „kein Konto“ / „nicht im Besitz“ | Mit dem Microsoft-Konto anmelden, dem **Minecraft: Java Edition gehört** (Schritt 4). |
| Du kannst dich umschauen, aber nicht laufen | Noclip muss **aus** sein (ReSkate-Menü, *Einfg* → Skater). SkateCraft sagt Bescheid, wenn es an ist. |
| HUD / Inventar von Minecraft fehlt | Zweimal F8 drücken. |
| Blöcke sieht man durch Wände | Im Moment bekannt: SkateCraft malt die Blöcke über das Bild von skate. |
| Controller geht im Minecraft-Modus nicht | Bekannt: Nur Tastatur und Maus gehen an Minecraft. Den Controller zum Skaten benutzen. |
| Alles andere | In der Konsole `skatecraft status` eingeben, Screenshot machen und ein Issue mit den zwei Logs aufmachen: `ReSkate.log` (im Ordner `logs\` deines SkateCraft-Ordners) und `latest.log` (in `%LOCALAPPDATA%\SkateCraft\Prism\instances\SkateCraft\.minecraft\logs`). |

Issues: **https://github.com/MaxiMagDich93/Skatecraft/issues**

> Bitte melde Probleme mit SkateCraft nicht bei ReSkate oder SkyCraft – die haben SkateCraft nicht
> gemacht.

---

## 8. Deinstallieren

Windows-*Einstellungen* → *Apps* → **SkateCraft** → *Deinstallieren*. Damit verschwinden die Dateien,
die der Installer hingelegt hat. Die Spieldateien von skate., die der Launcher heruntergeladen hat, und
deine Minecraft-Daten in `%LOCALAPPDATA%\SkateCraft` (Welten, Prism-Anmeldung) bleiben – lösch diese
Ordner von Hand, wenn du sie loswerden willst. Dein anderes ReSkate bleibt unberührt.

---

## 9. Ohne Installer (portables Zip)

Jedes Release hat auch `SkateCraft-<version>-portable.zip`:

1. Entpacken.
2. `ReSkate\ReSkateLauncher.exe` und `ReSkate\ReSkate.dll` in einen neuen, leeren Ordner legen (z. B.
   `C:\Games\SkateCraft`).
3. Alles aus `Minecraft\` nach `%LOCALAPPDATA%\SkateCraft` kopieren (das in die Adresszeile des
   Explorers tippen). `defaults\prismlauncher.cfg` nach `%LOCALAPPDATA%\SkateCraft\Prism\` kopieren.
4. Weiter mit Schritt 4 dieser Anleitung (Anmelden mit `Prism\prismlauncher.exe`).

---

## Danksagung

SkateCraft gibt es nur wegen zwei großartiger Projekte – lasst ihnen einen ⭐ da:

- **[ReSkate](https://github.com/Dingo-Shenanigans/ReSkate)** von **Dingo-Shenanigans** – der Launcher,
  die Runtime und die Mod-Plattform für skate., in die SkateCraft eingebaut ist. GPL-3.0.
- **[SkyCraft](https://github.com/chasmlol/SkyCraft)** von **chasmlol** – Minecraft in Skyrim.
  SkateCraft nutzt den Minecraft-Mod von SkyCraft unverändert und spricht sein Shared-Memory-Protokoll.
  MIT.

Außerdem dabei: [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) (GPL-3.0),
[Fabric API](https://github.com/FabricMC/fabric) (Apache-2.0),
[e4mc](https://github.com/vgskye/e4mc-minecraft-architectury) (MIT).

SkateCraft ist ein Fanprojekt. Es hat nichts mit Electronic Arts, Full Circle, Mojang oder Microsoft zu
tun und wird von ihnen nicht unterstützt – auch nicht von den Entwicklern von ReSkate oder SkyCraft.
