$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$dist = Join-Path $root "dist"
$installerWork = Join-Path $dist "installer-work"
$payloadDir = Join-Path $installerWork "payload"
$payloadZip = Join-Path $installerWork "payload.zip"
$installerSource = Join-Path $root "tools\TrukendoosWindowsInstaller.cs"
$baseInstallerExe = Join-Path $installerWork "trukendoos-installer-base.exe"
$outputExe = Join-Path $dist "trukendoos-windows-installer.exe"

$vst3Source = Join-Path $root "build-vs\SpectralRot_artefacts\Release\VST3\trukendoos.vst3"
$standaloneSource = Join-Path $root "build-vs\SpectralRot_artefacts\Release\Standalone\trukendoos.exe"
$csc = Join-Path $env:WINDIR "Microsoft.NET\Framework64\v4.0.30319\csc.exe"
$marker = [System.Text.Encoding]::ASCII.GetBytes("TRUKENDOOS_PAYLOAD_V1")

if (-not (Test-Path -LiteralPath $vst3Source)) {
    throw "Missing VST3 build artifact: $vst3Source. Run tools\build_vs.cmd first."
}

if (-not (Test-Path -LiteralPath $standaloneSource)) {
    throw "Missing standalone build artifact: $standaloneSource. Run tools\build_vs.cmd first."
}

if (-not (Test-Path -LiteralPath $installerSource)) {
    throw "Missing installer source: $installerSource"
}

if (-not (Test-Path -LiteralPath $csc)) {
    throw "Could not find C# compiler: $csc"
}

Remove-Item -LiteralPath $installerWork -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $payloadDir | Out-Null
New-Item -ItemType Directory -Force -Path $dist | Out-Null

Copy-Item -LiteralPath $vst3Source -Destination $payloadDir -Recurse -Force
Copy-Item -LiteralPath $standaloneSource -Destination (Join-Path $payloadDir "trukendoos.exe") -Force

if (Test-Path -LiteralPath $payloadZip) {
    Remove-Item -LiteralPath $payloadZip -Force
}

Compress-Archive -Path (Join-Path $payloadDir "*") -DestinationPath $payloadZip -Force

& $csc `
    /nologo `
    /target:winexe `
    /optimize+ `
    /out:$baseInstallerExe `
    /reference:System.Windows.Forms.dll `
    /reference:System.Drawing.dll `
    /reference:System.IO.Compression.dll `
    /reference:System.IO.Compression.FileSystem.dll `
    $installerSource

if (-not (Test-Path -LiteralPath $baseInstallerExe)) {
    throw "C# compiler did not create installer base exe."
}

Copy-Item -LiteralPath $baseInstallerExe -Destination $outputExe -Force

$payloadBytes = [System.IO.File]::ReadAllBytes($payloadZip)
$lengthBytes = [System.BitConverter]::GetBytes([Int64] $payloadBytes.Length)

$stream = [System.IO.File]::Open($outputExe, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
try {
    $stream.Write($payloadBytes, 0, $payloadBytes.Length)
    $stream.Write($lengthBytes, 0, $lengthBytes.Length)
    $stream.Write($marker, 0, $marker.Length)
} finally {
    $stream.Dispose()
}

Get-Item -LiteralPath $outputExe | Format-List FullName,Length,LastWriteTime
