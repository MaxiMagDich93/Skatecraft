# Gives the new SkateCraft folder the game files of a ReSkate folder you already have, so ReSkate's
# launcher does not download skate. (about 14 GB) a second time.
#
# On the same drive each file is a hard link: the same file on disk under two names, so it takes no
# extra space. The game build is pinned for good (ReSkate supports one build only), so the files never
# change under either folder. On another drive the files are copied.
#
# ReSkate's own files (launcher, ReSkate.dll, its settings, logs, mods and the merged ModData) are left out: SkateCraft
# brings its own, and your normal ReSkate stays as it is.
#
#   powershell -ExecutionPolicy Bypass -File share_game_files.ps1 -From "C:\Games\ReSkate" -To "C:\Games\SkateCraft"
param(
    [Parameter(Mandatory = $true)][string]$From,
    [Parameter(Mandatory = $true)][string]$To
)
$ErrorActionPreference = "Stop"

$From = (Resolve-Path $From).Path.TrimEnd('\')
New-Item -ItemType Directory -Force $To | Out-Null
$To = (Resolve-Path $To).Path.TrimEnd('\')
if (-not (Test-Path (Join-Path $From "Skate.exe"))) { throw "No Skate.exe in $From" }
if ($From -eq $To) { throw "The two folders are the same" }

$skipFiles = @("ReSkateLauncher.exe", "ReSkate.dll", "ReSkateLauncher.settings.json", "ReSkateServer.exe", "capture-status.txt")
$skipFolders = @("Mods", "ModData", "logs", "crashes", "CrashDumps")

$files = Get-ChildItem -LiteralPath $From -Recurse -File -Force | Where-Object {
    $relative = $_.FullName.Substring($From.Length + 1)
    $top = $relative.Split('\')[0]
    -not ($skipFolders -contains $top) -and
    -not ($relative -notmatch '\\' -and ($skipFiles -contains $_.Name -or $_.Name -like "ReSkate.dll.*" -or $_.Name -like "*.log" -or $_.Name -like "*.update-*"))
}

$total = $files.Count
$linked = 0; $copied = 0; $kept = 0; $index = 0
Write-Host "SkateCraft: taking $total game files from $From"
foreach ($file in $files) {
    $index++
    $relative = $file.FullName.Substring($From.Length + 1)
    $target = Join-Path $To $relative
    if ($index % 200 -eq 0 -or $index -eq $total) {
        Write-Progress -Activity "SkateCraft: game files" -Status "$index / $total" -PercentComplete ([int](100 * $index / [math]::Max(1, $total)))
    }
    if (Test-Path -LiteralPath $target) { $kept++; continue }
    New-Item -ItemType Directory -Force (Split-Path $target) | Out-Null
    try {
        New-Item -ItemType HardLink -Path $target -Target $file.FullName -ErrorAction Stop | Out-Null
        $linked++
    } catch {
        Copy-Item -LiteralPath $file.FullName -Destination $target
        $copied++
    }
}
Write-Progress -Activity "SkateCraft: game files" -Completed
Write-Host "SkateCraft: $linked linked, $copied copied, $kept already there."
