# Builds the Minecraft half of SkateCraft into a folder, for the installer.
#
# SkateCraft uses SkyCraft's Minecraft mod unchanged. SkyCraft's release carries a ready Minecraft
# package (SkyCraft-Minecraft.zip: a portable Prism Launcher with a "SkyCraft" instance of Minecraft
# 26.3, Fabric API, e4mc and SkyCraft's mod). This script takes that package from the pinned SkyCraft
# release, renames the instance to "SkateCraft" and points SkyCraft's mod at SkateCraft's shared
# memory (-Dskycraft.link=SkateCraft_v1) instead of Skyrim's.
#
# Minecraft itself, Java and Fabric Loader are not in it: Prism downloads them after the player signs
# in with a Microsoft account that owns Minecraft: Java Edition.
#
#   pwsh -File SkateCraft/installer/make_minecraft_bundle.ps1 -Out build/minecraft [-SkyCraftZip <file>]
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$SkyCraftZip
)
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
Add-Type -AssemblyName System.IO.Compression.FileSystem

# Pinned: SkyCraft 0.1.2 (the protocol in External/skycraft_protocol is from the same commit).
$skyCraftVersion = "0.1.2"
$skyCraftUrl = "https://github.com/chasmlol/SkyCraft/releases/download/v$skyCraftVersion/SkyCraft-$skyCraftVersion.zip"
$skyCraftSha256 = "1133ecde384d70d5261cbc9ba149bc7b544e1653b78846a1623b56241bb392c3"

$temp = Join-Path ([System.IO.Path]::GetTempPath()) ("skatecraft-bundle-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force $temp | Out-Null
try {
    if (-not $SkyCraftZip) {
        $SkyCraftZip = Join-Path $temp "SkyCraft-$skyCraftVersion.zip"
        Write-Host "Downloading $skyCraftUrl"
        Invoke-WebRequest -Uri $skyCraftUrl -OutFile $SkyCraftZip -UseBasicParsing
    }
    $hash = (Get-FileHash $SkyCraftZip -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $skyCraftSha256) { throw "SkyCraft-$skyCraftVersion.zip has SHA-256 $hash, expected $skyCraftSha256" }

    $release = Join-Path $temp "release"
    [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path $SkyCraftZip).Path, $release)
    $package = Join-Path $release "SKSE\Plugins\SkyCraft\SkyCraft-Minecraft.zip"
    if (-not (Test-Path $package)) { throw "SkyCraft-Minecraft.zip is not in the SkyCraft release" }

    if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }
    New-Item -ItemType Directory -Force $Out | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory($package, (Resolve-Path $Out).Path)

    $prism = Join-Path $Out "Prism"
    if (-not (Test-Path (Join-Path $prism "prismlauncher.exe"))) { throw "Prism\prismlauncher.exe is missing from the package" }

    # The instance "SkyCraft" becomes "SkateCraft", linked to skate. rather than Skyrim. The name has no
    # "Local\" in front: Prism's config file would swallow the backslash. SkyCraft's mod opens the name
    # as it is, which Windows looks up in the same session namespace as skate.'s Local\SkateCraft_v1.
    $from = Join-Path $prism "instances\SkyCraft"
    $to = Join-Path $prism "instances\SkateCraft"
    Move-Item $from $to
    $instanceCfg = Join-Path $to "instance.cfg"
    $jvm = "JvmArgs=--enable-native-access=ALL-UNNAMED -Dskycraft.startHidden=true -Dskycraft.link=SkateCraft_v1"
    $lines = Get-Content $instanceCfg | ForEach-Object {
        if ($_ -like "JvmArgs=*") { $jvm }
        elseif ($_ -like "name=*") { "name=SkateCraft" }
        elseif ($_ -like "notes=*") { "notes=SkateCraft: SkyCraft's Minecraft mod, linked to skate. (ReSkate with SkateCraft). Started by skate. when you press F8." }
        else { $_ }
    }
    if (-not ($lines -match "^JvmArgs=")) { $lines += $jvm; $lines += "OverrideJavaArgs=true" }
    Set-Content -Path $instanceCfg -Value $lines -Encoding ascii

    # Prism's settings: its defaults from the package, used when the player has none yet.
    $defaults = Join-Path $Out "defaults\prismlauncher.cfg"
    if (-not (Test-Path $defaults)) { throw "defaults\prismlauncher.cfg is missing from the package" }

    # SkyCraft's license and notices travel with the package.
    Copy-Item (Join-Path $release "SKSE\Plugins\SkyCraft\LICENSE.txt") (Join-Path $Out "LICENSE-SkyCraft.txt")
    Copy-Item (Join-Path $release "SKSE\Plugins\SkyCraft\THIRD-PARTY-NOTICES.md") (Join-Path $Out "THIRD-PARTY-NOTICES-SkyCraft.md")
    Add-Content -Path (Join-Path $Out "bundle-version.txt") -Value "`r`nRenamed to the SkateCraft instance by SkateCraft's make_minecraft_bundle.ps1."

    Get-ChildItem $Out -Recurse -File | Measure-Object -Property Length -Sum |
        ForEach-Object { Write-Host ("Minecraft bundle: {0} files, {1:N1} MB in {2}" -f $_.Count, ($_.Sum / 1MB), $Out) }
} finally {
    Remove-Item -Recurse -Force $temp -ErrorAction SilentlyContinue
}
