param(
    [switch] $InstallStandalone = $true
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceVst3 = Join-Path $scriptDir "trukendoos.vst3"
$sourceExe = Join-Path $scriptDir "trukendoos.exe"
$vst3Target = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
$appTarget = Join-Path $env:LOCALAPPDATA "Programs\trukendoos"

if (-not (Test-Path $sourceVst3)) {
    throw "Could not find trukendoos.vst3 next to this installer script."
}

New-Item -ItemType Directory -Force $vst3Target | Out-Null
Copy-Item -LiteralPath $sourceVst3 -Destination $vst3Target -Recurse -Force

if ($InstallStandalone -and (Test-Path $sourceExe)) {
    New-Item -ItemType Directory -Force $appTarget | Out-Null
    Copy-Item -LiteralPath $sourceExe -Destination (Join-Path $appTarget "trukendoos.exe") -Force
}

Write-Host "Installed trukendoos VST3 to: $vst3Target"
if ($InstallStandalone -and (Test-Path $sourceExe)) {
    Write-Host "Installed trukendoos standalone to: $appTarget"
}
Write-Host "Done. Rescan plugins in your DAW if trukendoos does not show up immediately."
