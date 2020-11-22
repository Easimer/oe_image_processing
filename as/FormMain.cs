using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Net.Easimer.KAA.Front
{
    public partial class FormMain : Form
    {
        private Button btnPipeline;

        public FormMain()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            this.btnPipeline = new System.Windows.Forms.Button();
            this.btnInpaintingDemo = new System.Windows.Forms.Button();
            this.btnProcessVideo = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnPipeline
            // 
            this.btnPipeline.Location = new System.Drawing.Point(12, 12);
            this.btnPipeline.Name = "btnPipeline";
            this.btnPipeline.Size = new System.Drawing.Size(319, 23);
            this.btnPipeline.TabIndex = 0;
            this.btnPipeline.Text = "Open video (pipeline view)";
            this.btnPipeline.UseVisualStyleBackColor = true;
            this.btnPipeline.Click += new System.EventHandler(this.OpenVideoInPipelineView);
            // 
            // btnInpaintingDemo
            // 
            this.btnInpaintingDemo.Location = new System.Drawing.Point(12, 41);
            this.btnInpaintingDemo.Name = "btnInpaintingDemo";
            this.btnInpaintingDemo.Size = new System.Drawing.Size(319, 23);
            this.btnInpaintingDemo.TabIndex = 1;
            this.btnInpaintingDemo.Text = "Inpainting demo";
            this.btnInpaintingDemo.UseVisualStyleBackColor = true;
            this.btnInpaintingDemo.Click += new System.EventHandler(this.BeginInpaintingDemo);
            // 
            // btnProcessVideo
            // 
            this.btnProcessVideo.Location = new System.Drawing.Point(12, 70);
            this.btnProcessVideo.Name = "btnProcessVideo";
            this.btnProcessVideo.Size = new System.Drawing.Size(319, 23);
            this.btnProcessVideo.TabIndex = 2;
            this.btnProcessVideo.Text = "Process video";
            this.btnProcessVideo.UseVisualStyleBackColor = true;
            this.btnProcessVideo.Click += new System.EventHandler(this.ProcessVideo);
            // 
            // FormMain
            // 
            this.ClientSize = new System.Drawing.Size(343, 371);
            this.Controls.Add(this.btnProcessVideo);
            this.Controls.Add(this.btnInpaintingDemo);
            this.Controls.Add(this.btnPipeline);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "FormMain";
            this.ShowIcon = false;
            this.Text = "Front";
            this.ResumeLayout(false);

        }

        private void OpenVideoInPipelineView(object sender, EventArgs e)
        {
            var dlg = new OpenFileDialog();
            dlg.Filter = "Videos (*.mp4;*.mkv;*.webm)|*.mp4;*.mkv;*.webm|All files (*.*)|*.*";
            var res = dlg.ShowDialog();
            if(res == DialogResult.OK)
            {
                var oeip = PipelineDemoSession.Create(dlg.FileName);

                if(oeip != null)
                {
                    new FormPipeline(oeip).Show();
                }
            }
        }

        private Button btnInpaintingDemo;

        private void BeginInpaintingDemo(object sender, EventArgs e)
        {
            var wnd = FormInpaint.Make();
            wnd?.Show();
        }

        private Button btnProcessVideo;

        private void ProcessVideo(object sender, EventArgs e)
        {
            var wnd = FormProcess.Make();
            wnd?.Show();
        }
    }
}
