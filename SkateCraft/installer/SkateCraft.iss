; SkateCraft installer (Inno Setup 6).
;
; Installs, for the current Windows user and without admin rights:
;   - a ReSkate of its own with SkateCraft in it (ReSkateLauncher.exe and ReSkate.dll built from this
;     repository) in a folder of its choosing. ReSkate's launcher installs skate. into that folder
;     from Steam, or the installer takes the game files from a ReSkate folder you already have.
;   - SkyCraft's Minecraft (portable Prism Launcher, a "SkateCraft" instance with SkyCraft's mod) in
;     %LOCALAPPDATA%\SkateCraft, where SkateCraft looks for it.
;
; Built by .github/workflows/build.yml:
;   iscc /DAppVersion=0.1.0 /DBinDir=<ReSkate build output> /DBundleDir=<make_minecraft_bundle.ps1 output>
;        /DRepoDir=<repository root> /O<output folder> SkateCraft\installer\SkateCraft.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BinDir
  #define BinDir "..\..\build\vs2022-x64\Release"
#endif
#ifndef BundleDir
  #define BundleDir "..\..\build\minecraft"
#endif
#ifndef RepoDir
  #define RepoDir "..\.."
#endif

[Setup]
AppId={{5B7C2E61-4F0A-4C8E-9D3B-6A1E2F8C9B47}
AppName=SkateCraft
AppVersion={#AppVersion}
AppVerName=SkateCraft {#AppVersion}
AppPublisher=MaxiMagDich93
AppPublisherURL=https://github.com/MaxiMagDich93/Skatecraft
AppSupportURL=https://github.com/MaxiMagDich93/Skatecraft/issues
AppUpdatesURL=https://github.com/MaxiMagDich93/Skatecraft/releases
DefaultDirName={sd}\Games\SkateCraft
DefaultGroupName=SkateCraft
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=SkateCraft-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#RepoDir}\assets\launcher\icon.ico
UninstallDisplayIcon={app}\ReSkateLauncher.exe
LicenseFile={#RepoDir}\LICENSE
DirExistsWarning=no
UsePreviousAppDir=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"; InfoBeforeFile: "before_install.en.txt"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"; InfoBeforeFile: "before_install.de.txt"

[CustomMessages]
english.GamePageTitle=skate. game files
english.GamePageDescription=Where should SkateCraft's ReSkate get skate. from?
english.GamePageText=SkateCraft installs a ReSkate of its own, so your normal ReSkate stays untouched. That ReSkate needs its own copy of skate. (about 14 GB).
english.GameDownload=Download skate. with ReSkate's launcher on the first start (Steam sign-in, about 14 GB)
english.GameShare=Take the game files from a ReSkate folder I already have (fast; on the same drive it uses no extra space)
english.SharePageTitle=Your existing ReSkate folder
english.SharePageDescription=The folder with ReSkateLauncher.exe and Skate.exe in it.
english.SharePageText=Pick the folder of the ReSkate you already play with. Its game files are linked (same drive) or copied (other drive) into the SkateCraft folder. Your ReSkate there is not changed.
english.ShareNoSkate=There is no Skate.exe in that folder. Pick the folder where ReSkate installed skate. (the one with ReSkateLauncher.exe and Skate.exe).
english.ShareSameFolder=That is the SkateCraft folder itself. Pick your other ReSkate folder.
english.Sharing=Taking the game files from your ReSkate folder (a window shows the progress)...
english.ShareFailed=The game files could not be taken from your ReSkate folder (code %1). No problem: ReSkate's launcher will download skate. on the first start instead.
english.IconSkate=SkateCraft (skate. with ReSkate)
english.IconMinecraft=SkateCraft Minecraft (sign in / start by hand)
english.IconGuide=SkateCraft guide
english.IconFolder=SkateCraft Minecraft folder
english.RunPrism=Sign in to Minecraft now (opens Prism Launcher: sign in with your Microsoft account)
english.RunLauncher=Start SkateCraft (ReSkate's launcher)
english.TaskDesktop=Desktop shortcuts
german.GamePageTitle=Spieldateien von skate.
german.GamePageDescription=Woher soll das ReSkate von SkateCraft skate. bekommen?
german.GamePageText=SkateCraft installiert ein eigenes ReSkate, damit dein normales ReSkate unverändert bleibt. Dieses ReSkate braucht eine eigene Kopie von skate. (etwa 14 GB).
german.GameDownload=skate. beim ersten Start mit dem ReSkate-Launcher herunterladen (Steam-Anmeldung, etwa 14 GB)
german.GameShare=Spieldateien aus einem ReSkate-Ordner nehmen, den ich schon habe (schnell; auf derselben Festplatte ohne zusätzlichen Platz)
german.SharePageTitle=Dein vorhandener ReSkate-Ordner
german.SharePageDescription=Der Ordner, in dem ReSkateLauncher.exe und Skate.exe liegen.
german.SharePageText=Wähle den Ordner des ReSkate, mit dem du schon spielst. Seine Spieldateien werden in den SkateCraft-Ordner verlinkt (gleiche Festplatte) oder kopiert (andere Festplatte). Dein ReSkate dort wird nicht verändert.
german.ShareNoSkate=In diesem Ordner liegt keine Skate.exe. Wähle den Ordner, in den ReSkate skate. installiert hat (der mit ReSkateLauncher.exe und Skate.exe).
german.ShareSameFolder=Das ist der SkateCraft-Ordner selbst. Wähle deinen anderen ReSkate-Ordner.
german.Sharing=Übernehme die Spieldateien aus deinem ReSkate-Ordner (ein Fenster zeigt den Fortschritt) ...
german.ShareFailed=Die Spieldateien konnten nicht aus deinem ReSkate-Ordner übernommen werden (Code %1). Kein Problem: Der ReSkate-Launcher lädt skate. dann beim ersten Start herunter.
german.IconSkate=SkateCraft (skate. mit ReSkate)
german.IconMinecraft=SkateCraft Minecraft (anmelden / von Hand starten)
german.IconGuide=SkateCraft Anleitung
german.IconFolder=SkateCraft Minecraft-Ordner
german.RunPrism=Jetzt bei Minecraft anmelden (öffnet den Prism Launcher: mit deinem Microsoft-Konto anmelden)
german.RunLauncher=SkateCraft starten (ReSkate-Launcher)
german.TaskDesktop=Verknüpfungen auf dem Desktop

[Tasks]
Name: "desktopicon"; Description: "{cm:TaskDesktop}"; Flags: unchecked

[InstallDelete]
; The mods SkyCraft's package brings are replaced with each install (as SkyCraft does).
Type: files; Name: "{localappdata}\SkateCraft\Prism\instances\SkateCraft\.minecraft\mods\skycraft-*.jar"
Type: files; Name: "{localappdata}\SkateCraft\Prism\instances\SkateCraft\.minecraft\mods\fabric-api-*.jar"
Type: files; Name: "{localappdata}\SkateCraft\Prism\instances\SkateCraft\.minecraft\mods\e4mc-*.jar"

[Files]
; ReSkate with SkateCraft.
Source: "{#BinDir}\ReSkateLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\ReSkate.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoDir}\LICENSE"; DestDir: "{app}\SkateCraft"; DestName: "LICENSE-ReSkate-and-SkateCraft.txt"; Flags: ignoreversion
Source: "{#RepoDir}\External\skycraft_protocol\LICENSE"; DestDir: "{app}\SkateCraft"; DestName: "LICENSE-SkyCraft-protocol.txt"; Flags: ignoreversion
Source: "{#RepoDir}\SkateCraft\docs\INSTALL.en.md"; DestDir: "{app}\SkateCraft"; Flags: ignoreversion
Source: "{#RepoDir}\SkateCraft\docs\INSTALL.de.md"; DestDir: "{app}\SkateCraft"; Flags: ignoreversion
Source: "share_game_files.ps1"; DestDir: "{app}\SkateCraft"; Flags: ignoreversion
; SkyCraft's Minecraft. Prism's settings only when there are none yet (they hold your choices).
Source: "{#BundleDir}\*"; DestDir: "{localappdata}\SkateCraft"; Excludes: "\defaults\*"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BundleDir}\defaults\prismlauncher.cfg"; DestDir: "{localappdata}\SkateCraft\Prism"; Flags: onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\{cm:IconSkate}"; Filename: "{app}\ReSkateLauncher.exe"; WorkingDir: "{app}"
Name: "{group}\{cm:IconMinecraft}"; Filename: "{localappdata}\SkateCraft\Prism\prismlauncher.exe"; Parameters: "--launch SkateCraft"; WorkingDir: "{localappdata}\SkateCraft\Prism"
Name: "{group}\{cm:IconGuide}"; Filename: "https://github.com/MaxiMagDich93/Skatecraft#readme"
Name: "{group}\{cm:IconFolder}"; Filename: "{localappdata}\SkateCraft"
Name: "{group}\{cm:UninstallProgram,SkateCraft}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{cm:IconSkate}"; Filename: "{app}\ReSkateLauncher.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{localappdata}\SkateCraft\Prism\prismlauncher.exe"; WorkingDir: "{localappdata}\SkateCraft\Prism"; Description: "{cm:RunPrism}"; Flags: postinstall nowait skipifsilent
Filename: "{app}\ReSkateLauncher.exe"; WorkingDir: "{app}"; Description: "{cm:RunLauncher}"; Flags: postinstall nowait skipifsilent unchecked

[Code]
var
  GamePage: TInputOptionWizardPage;
  SharePage: TInputDirWizardPage;

procedure InitializeWizard;
begin
  GamePage := CreateInputOptionPage(wpSelectDir, CustomMessage('GamePageTitle'), CustomMessage('GamePageDescription'),
    CustomMessage('GamePageText'), True, False);
  GamePage.Add(CustomMessage('GameDownload'));
  GamePage.Add(CustomMessage('GameShare'));
  GamePage.SelectedValueIndex := 0;

  SharePage := CreateInputDirPage(GamePage.ID, CustomMessage('SharePageTitle'), CustomMessage('SharePageDescription'),
    CustomMessage('SharePageText'), False, '');
  SharePage.Add('');
  if DirExists(ExpandConstant('{sd}\Games\ReSkate')) then
    SharePage.Values[0] := ExpandConstant('{sd}\Games\ReSkate');
end;

function SharingGameFiles: Boolean;
begin
  Result := GamePage.SelectedValueIndex = 1;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = SharePage.ID) and not SharingGameFiles;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  From: String;
begin
  Result := True;
  if CurPageID = SharePage.ID then begin
    From := RemoveBackslashUnlessRoot(SharePage.Values[0]);
    if not FileExists(AddBackslash(From) + 'Skate.exe') then begin
      MsgBox(CustomMessage('ShareNoSkate'), mbError, MB_OK);
      Result := False;
    end else if CompareText(From, RemoveBackslashUnlessRoot(WizardDirValue)) = 0 then begin
      MsgBox(CustomMessage('ShareSameFolder'), mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
  Params: String;
begin
  if (CurStep = ssPostInstall) and SharingGameFiles then begin
    WizardForm.StatusLabel.Caption := CustomMessage('Sharing');
    Params := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{app}\SkateCraft\share_game_files.ps1') +
      '" -From "' + RemoveBackslashUnlessRoot(SharePage.Values[0]) + '" -To "' + RemoveBackslashUnlessRoot(ExpandConstant('{app}')) + '"';
    if not Exec('powershell.exe', Params, '', SW_SHOWNORMAL, ewWaitUntilTerminated, Code) or (Code <> 0) then
      MsgBox(FmtMessage(CustomMessage('ShareFailed'), [IntToStr(Code)]), mbInformation, MB_OK);
  end;
end;
