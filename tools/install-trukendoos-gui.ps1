Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$payloadZip = Join-Path $scriptDir "payload.zip"

if (-not (Test-Path -LiteralPath $payloadZip)) {
    [System.Windows.Forms.MessageBox]::Show("Installer payload.zip is missing.", "trukendoos installer", "OK", "Error") | Out-Null
    exit 1
}

$installRoot = Join-Path $env:TEMP ("trukendoos-install-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
Expand-Archive -LiteralPath $payloadZip -DestinationPath $installRoot -Force

$sourceVst3 = Join-Path $installRoot "trukendoos.vst3"
$sourceStandalone = Join-Path $installRoot "trukendoos.exe"
$vst3Target = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
$appTarget = Join-Path $env:LOCALAPPDATA "Programs\trukendoos"
$presetTarget = Join-Path $env:APPDATA "trukendoos\Presets"
$vst3Installed = Test-Path -LiteralPath (Join-Path $vst3Target "trukendoos.vst3")
$standaloneInstalled = Test-Path -LiteralPath (Join-Path $appTarget "trukendoos.exe")

$form = New-Object System.Windows.Forms.Form
$form.Text = "trukendoos installer / updater"
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.ClientSize = New-Object System.Drawing.Size(440, 260)
$form.BackColor = [System.Drawing.Color]::FromArgb(22, 18, 15)
$form.ForeColor = [System.Drawing.Color]::FromArgb(255, 241, 189)

$title = New-Object System.Windows.Forms.Label
$title.Text = "trukendoos"
$title.Font = New-Object System.Drawing.Font("Segoe UI", 22, [System.Drawing.FontStyle]::Bold)
$title.AutoSize = $true
$title.Location = New-Object System.Drawing.Point(24, 20)
$form.Controls.Add($title)

$subtitle = New-Object System.Windows.Forms.Label
$subtitle.Text = if ($vst3Installed -or $standaloneInstalled) { "update found install - tijd voor een trucje!" } else { "tijd voor een trucje!" }
$subtitle.Font = New-Object System.Drawing.Font("Segoe UI", 9, [System.Drawing.FontStyle]::Bold)
$subtitle.AutoSize = $true
$subtitle.Location = New-Object System.Drawing.Point(28, 62)
$subtitle.ForeColor = [System.Drawing.Color]::FromArgb(98, 244, 200)
$form.Controls.Add($subtitle)

$vst3Check = New-Object System.Windows.Forms.CheckBox
$vst3Check.Text = if ($vst3Installed) { "Update VST3 plugin" } else { "Install VST3 plugin" }
$vst3Check.Checked = $true
$vst3Check.AutoSize = $true
$vst3Check.Location = New-Object System.Drawing.Point(32, 104)
$vst3Check.ForeColor = $form.ForeColor
$vst3Check.BackColor = $form.BackColor
$form.Controls.Add($vst3Check)

$vst3Path = New-Object System.Windows.Forms.Label
$vst3Path.Text = $vst3Target
$vst3Path.AutoSize = $true
$vst3Path.Location = New-Object System.Drawing.Point(52, 128)
$vst3Path.ForeColor = [System.Drawing.Color]::FromArgb(160, 150, 125)
$form.Controls.Add($vst3Path)

$standaloneCheck = New-Object System.Windows.Forms.CheckBox
$standaloneCheck.Text = if ($standaloneInstalled) { "Update standalone app" } else { "Install standalone app" }
$standaloneCheck.Checked = $true
$standaloneCheck.AutoSize = $true
$standaloneCheck.Location = New-Object System.Drawing.Point(32, 156)
$standaloneCheck.ForeColor = $form.ForeColor
$standaloneCheck.BackColor = $form.BackColor
$form.Controls.Add($standaloneCheck)

$standalonePath = New-Object System.Windows.Forms.Label
$standalonePath.Text = $appTarget
$standalonePath.AutoSize = $true
$standalonePath.Location = New-Object System.Drawing.Point(52, 180)
$standalonePath.ForeColor = [System.Drawing.Color]::FromArgb(160, 150, 125)
$form.Controls.Add($standalonePath)

$installButton = New-Object System.Windows.Forms.Button
$installButton.Text = if ($vst3Installed -or $standaloneInstalled) { "Update" } else { "Install" }
$installButton.Font = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$installButton.Size = New-Object System.Drawing.Size(100, 34)
$installButton.Location = New-Object System.Drawing.Point(216, 214)
$form.Controls.Add($installButton)

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = "Cancel"
$cancelButton.Size = New-Object System.Drawing.Size(100, 34)
$cancelButton.Location = New-Object System.Drawing.Point(324, 214)
$form.Controls.Add($cancelButton)

$cancelButton.Add_Click({
    $form.Close()
})

$installButton.Add_Click({
    try {
        if (-not $vst3Check.Checked -and -not $standaloneCheck.Checked) {
            [System.Windows.Forms.MessageBox]::Show("Select at least one thing to install.", "trukendoos installer", "OK", "Warning") | Out-Null
            return
        }

        New-Item -ItemType Directory -Force -Path $presetTarget | Out-Null

        if ($vst3Check.Checked) {
            if (-not (Test-Path -LiteralPath $sourceVst3)) {
                throw "Could not find trukendoos.vst3 in installer payload."
            }

            New-Item -ItemType Directory -Force -Path $vst3Target | Out-Null
            Copy-Item -LiteralPath $sourceVst3 -Destination $vst3Target -Recurse -Force
        }

        if ($standaloneCheck.Checked) {
            if (-not (Test-Path -LiteralPath $sourceStandalone)) {
                throw "Could not find trukendoos.exe in installer payload."
            }

            New-Item -ItemType Directory -Force -Path $appTarget | Out-Null
            Copy-Item -LiteralPath $sourceStandalone -Destination (Join-Path $appTarget "trukendoos.exe") -Force
        }

        [System.Windows.Forms.MessageBox]::Show("trukendoos installed/updated. Rescan plugins in your DAW if needed.", "trukendoos installer", "OK", "Information") | Out-Null
        $form.Close()
    } catch {
        [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, "trukendoos installer", "OK", "Error") | Out-Null
    }
})

[void]$form.ShowDialog()

try {
    Remove-Item -LiteralPath $installRoot -Recurse -Force -ErrorAction SilentlyContinue
} catch {}
