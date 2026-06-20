using System;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Windows.Forms;

internal static class TrukendoosWindowsInstaller
{
    private static readonly byte[] Marker = System.Text.Encoding.ASCII.GetBytes("TRUKENDOOS_PAYLOAD_V1");

    [STAThread]
    private static int Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);

        try
        {
            using (var installer = new InstallerForm())
                Application.Run(installer);

            return 0;
        }
        catch (Exception ex)
        {
            MessageBox.Show(ex.Message, "trukendoos installer", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }

    private sealed class InstallerForm : Form
    {
        private readonly CheckBox vst3Check = new CheckBox();
        private readonly CheckBox standaloneCheck = new CheckBox();
        private readonly Button installButton = new Button();
        private readonly Button cancelButton = new Button();

        public InstallerForm()
        {
            Text = "trukendoos installer";
            StartPosition = FormStartPosition.CenterScreen;
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            ClientSize = new Size(470, 260);
            BackColor = Color.FromArgb(22, 18, 15);
            ForeColor = Color.FromArgb(255, 241, 189);

            var title = new Label
            {
                Text = "trukendoos",
                AutoSize = true,
                Location = new Point(24, 18),
                Font = new Font("Segoe UI", 22.0f, FontStyle.Bold),
                ForeColor = ForeColor,
                BackColor = BackColor
            };

            var subtitle = new Label
            {
                Text = "tijd voor een trucje!",
                AutoSize = true,
                Location = new Point(28, 60),
                Font = new Font("Segoe UI", 9.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(98, 244, 200),
                BackColor = BackColor
            };

            vst3Check.Text = "Install VST3 plugin";
            vst3Check.Checked = true;
            vst3Check.AutoSize = true;
            vst3Check.Location = new Point(32, 100);
            vst3Check.ForeColor = ForeColor;
            vst3Check.BackColor = BackColor;

            var vst3Path = new Label
            {
                Text = Vst3Target,
                AutoSize = true,
                Location = new Point(52, 124),
                ForeColor = Color.FromArgb(160, 150, 125),
                BackColor = BackColor
            };

            standaloneCheck.Text = "Install standalone app";
            standaloneCheck.Checked = true;
            standaloneCheck.AutoSize = true;
            standaloneCheck.Location = new Point(32, 154);
            standaloneCheck.ForeColor = ForeColor;
            standaloneCheck.BackColor = BackColor;

            var standalonePath = new Label
            {
                Text = AppTarget,
                AutoSize = true,
                Location = new Point(52, 178),
                ForeColor = Color.FromArgb(160, 150, 125),
                BackColor = BackColor
            };

            installButton.Text = "Install";
            installButton.Font = new Font("Segoe UI", 10.0f, FontStyle.Bold);
            installButton.Size = new Size(100, 34);
            installButton.Location = new Point(244, 214);
            installButton.Click += InstallButtonClicked;

            cancelButton.Text = "Cancel";
            cancelButton.Size = new Size(100, 34);
            cancelButton.Location = new Point(352, 214);
            cancelButton.Click += (sender, args) => Close();

            Controls.Add(title);
            Controls.Add(subtitle);
            Controls.Add(vst3Check);
            Controls.Add(vst3Path);
            Controls.Add(standaloneCheck);
            Controls.Add(standalonePath);
            Controls.Add(installButton);
            Controls.Add(cancelButton);
        }

        private static string Vst3Target
        {
            get
            {
                return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    @"Programs\Common\VST3");
            }
        }

        private static string AppTarget
        {
            get
            {
                return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    @"Programs\trukendoos");
            }
        }

        private static string PresetTarget
        {
            get
            {
                return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                    @"trukendoos\Presets");
            }
        }

        private void InstallButtonClicked(object sender, EventArgs args)
        {
            try
            {
                if (!vst3Check.Checked && !standaloneCheck.Checked)
                {
                    MessageBox.Show("Select at least one thing to install.", "trukendoos installer", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }

                installButton.Enabled = false;
                cancelButton.Enabled = false;
                Cursor = Cursors.WaitCursor;

                var tempRoot = Path.Combine(Path.GetTempPath(), "trukendoos-install-" + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(tempRoot);

                try
                {
                    var payload = Path.Combine(tempRoot, "payload.zip");
                    ExtractEmbeddedPayload(payload);
                    ZipFile.ExtractToDirectory(payload, tempRoot);

                    Directory.CreateDirectory(PresetTarget);

                    if (vst3Check.Checked)
                    {
                        var source = Path.Combine(tempRoot, "trukendoos.vst3");
                        if (!Directory.Exists(source))
                            throw new DirectoryNotFoundException("Could not find trukendoos.vst3 in installer payload.");

                        Directory.CreateDirectory(Vst3Target);
                        var destination = Path.Combine(Vst3Target, "trukendoos.vst3");
                        if (Directory.Exists(destination))
                            Directory.Delete(destination, true);

                        CopyDirectory(source, destination);
                    }

                    if (standaloneCheck.Checked)
                    {
                        var source = Path.Combine(tempRoot, "trukendoos.exe");
                        if (!File.Exists(source))
                            throw new FileNotFoundException("Could not find trukendoos.exe in installer payload.");

                        Directory.CreateDirectory(AppTarget);
                        File.Copy(source, Path.Combine(AppTarget, "trukendoos.exe"), true);
                    }

                    MessageBox.Show("trukendoos installed. Rescan plugins in your DAW if needed.", "trukendoos installer", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    Close();
                }
                finally
                {
                    try { Directory.Delete(tempRoot, true); } catch { }
                    Cursor = Cursors.Default;
                    installButton.Enabled = true;
                    cancelButton.Enabled = true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "trukendoos installer", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }

    private static void ExtractEmbeddedPayload(string outputZip)
    {
        var executable = Application.ExecutablePath;
        var bytes = File.ReadAllBytes(executable);

        if (bytes.Length < Marker.Length + sizeof(long))
            throw new InvalidDataException("Installer payload is missing.");

        var markerStart = bytes.Length - Marker.Length;
        for (var i = 0; i < Marker.Length; ++i)
        {
            if (bytes[markerStart + i] != Marker[i])
                throw new InvalidDataException("Installer payload marker is missing.");
        }

        var lengthStart = markerStart - sizeof(long);
        var payloadLength = BitConverter.ToInt64(bytes, lengthStart);
        var payloadStart = lengthStart - payloadLength;

        if (payloadLength <= 0 || payloadStart < 0)
            throw new InvalidDataException("Installer payload is corrupt.");

        using (var output = File.Create(outputZip))
            output.Write(bytes, (int)payloadStart, (int)payloadLength);
    }

    private static void CopyDirectory(string source, string destination)
    {
        Directory.CreateDirectory(destination);

        foreach (var directory in Directory.GetDirectories(source, "*", SearchOption.AllDirectories))
        {
            var target = directory.Replace(source, destination);
            Directory.CreateDirectory(target);
        }

        foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
        {
            var target = file.Replace(source, destination);
            Directory.CreateDirectory(Path.GetDirectoryName(target));
            File.Copy(file, target, true);
        }
    }
}
