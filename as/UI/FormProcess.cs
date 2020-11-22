using System;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Net.Easimer.KAA.Front
{
    internal partial class FormProcess : Form
    {
        public static FormProcess Make()
        {
            // Bekerjuk az utvonalakat
            var paths = new string[2];
            var titles = new string[2] { "Path to input video", "Path to output video" };
            var filters = new string[2] { "All files (*.*)|*.*", "MP4 files (*.mp4)|*.mp4|All files (*.*)|*.*" };
            var isOutput = new bool[2] { false, true };

            for(int i = 0; i < titles.Length; i++)
            {
                var dlg = isOutput[i] ? (FileDialog)new SaveFileDialog() : (FileDialog)new OpenFileDialog();
                dlg.Filter = filters[i];
                dlg.Title = titles[i];

                var res = dlg.ShowDialog();
                if(res != DialogResult.OK)
                {
                    return null;
                }

                paths[i] = dlg.FileName;
            }

            var session = ProcessingSession.Make(paths[0], paths[1]);
            if(session == null)
            {
                return null;
            }

            return new FormProcess(session);
        }

        private ProcessingSession _session;
        private ProgressBar progressBar;
        private Label lblProgress;
        private Task _processTask;

        protected FormProcess(ProcessingSession session)
        {
            _session = session;
            InitializeComponent();

            _session.OnProgress += OnProgress;
            _processTask = new Task(() =>
            {
                _session.Process();

                this.Invoke((MethodInvoker)delegate
                {
                    MessageBox.Show("Done!");
                    Close();
                });
                _session.Dispose();
                _session = null;
            });
        }

        private void OnProgress(int CurrentFrame, int TotalFrames)
        {
            progressBar.Invoke((MethodInvoker) delegate
            {
                progressBar.Maximum = TotalFrames;
                progressBar.Value = CurrentFrame;
            });

            lblProgress.Invoke((MethodInvoker)delegate
            {
                 lblProgress.Text = $"{CurrentFrame} / {TotalFrames} ({100 * CurrentFrame / (float)TotalFrames}%)";
            });
        }

        private void InitializeComponent()
        {
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.lblProgress = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(12, 25);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(232, 23);
            this.progressBar.TabIndex = 0;
            // 
            // lblProgress
            // 
            this.lblProgress.AutoSize = true;
            this.lblProgress.Location = new System.Drawing.Point(12, 9);
            this.lblProgress.Name = "lblProgress";
            this.lblProgress.Size = new System.Drawing.Size(35, 13);
            this.lblProgress.TabIndex = 1;
            this.lblProgress.Text = "label1";
            // 
            // FormProcess
            // 
            this.ClientSize = new System.Drawing.Size(256, 55);
            this.Controls.Add(this.lblProgress);
            this.Controls.Add(this.progressBar);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FormProcess";
            this.ShowIcon = false;
            this.Text = "Processing";
            this.Load += new System.EventHandler(this.OnLoad);
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void OnLoad(object sender, EventArgs e)
        {
            _processTask.Start();
        }
    }
}
