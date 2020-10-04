using Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Net.Easimer.KAA.Front
{
    partial class FormPipeline : Form
    {
        private Oeip _oeip;

        public FormPipeline(Oeip oeip)
        {
            InitializeComponent();
            _oeip = oeip;

            _oeip.StageHasOutput += Output;
        }

        private void Output(Oeip.Stage stage, ImageBuffer buf)
        {
            switch(stage)
            {
                case Oeip.Stage.Input:
                    OutputGrayscale(picInput, buf);
                    break;
                default:
                    break;
            }
        }

        private void OutputGrayscale(PictureBox box, ImageBuffer buf)
        {
            var img = new Bitmap(buf.Width, buf.Height, PixelFormat.Format32bppArgb);
            
            var rect = new Rectangle(0, 0, buf.Width, buf.Height);
            var dat = img.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            var scan0 = dat.Scan0;
            var stride = dat.Stride;

            unsafe
            {
                var p = (uint*)scan0;
                var elemStride = stride / 4;
                for(int y = 0; y < buf.Height; y++)
                {
                    var buf_base = y * buf.Stride;

                    for(int x = 0; x < buf.Width; x++)
                    {
                        uint c = buf.Data[buf_base + x];
                        var v = 0xFF000000 | (c << 16) | (c << 8) | (c << 0);
                        p[x] = v;
                    }

                    p += elemStride;
                }
            }

            img.UnlockBits(dat);
            

            /*
            for (int y = 0; y < buf.Height; y++)
            {
                var buf_base = buf.Stride * y;
                for (int x = 0; x < buf.Width; x++)
                {
                    var c = buf.Data[buf_base + x];
                    img.SetPixel(x, y, Color.FromArgb(c, c, c));
                }
            }
            */

            if(box.Image != null)
            {
                box.Image.Dispose();
            }

            box.Image = img;
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.picInput = new System.Windows.Forms.PictureBox();
            this.stepTimer = new System.Windows.Forms.Timer(this.components);
            this.toolStrip = new System.Windows.Forms.ToolStrip();
            this.btnStart = new System.Windows.Forms.ToolStripButton();
            this.btnStop = new System.Windows.Forms.ToolStripButton();
            ((System.ComponentModel.ISupportInitialize)(this.picInput)).BeginInit();
            this.toolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // picInput
            // 
            this.picInput.Location = new System.Drawing.Point(34, 48);
            this.picInput.Name = "picInput";
            this.picInput.Size = new System.Drawing.Size(306, 256);
            this.picInput.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.picInput.TabIndex = 0;
            this.picInput.TabStop = false;
            // 
            // stepTimer
            // 
            this.stepTimer.Interval = 33;
            this.stepTimer.Tick += new System.EventHandler(this.TimerTick);
            // 
            // toolStrip
            // 
            this.toolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btnStart,
            this.btnStop});
            this.toolStrip.Location = new System.Drawing.Point(0, 0);
            this.toolStrip.Name = "toolStrip";
            this.toolStrip.Size = new System.Drawing.Size(637, 25);
            this.toolStrip.TabIndex = 1;
            this.toolStrip.Text = "toolStrip1";
            // 
            // btnStart
            // 
            this.btnStart.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.btnStart.Image = global::Properties.Resources.play;
            this.btnStart.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btnStart.Name = "btnStart";
            this.btnStart.Size = new System.Drawing.Size(23, 22);
            this.btnStart.Click += new System.EventHandler(this.OnClickStart);
            // 
            // btnStop
            // 
            this.btnStop.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.btnStop.Image = global::Properties.Resources.stop;
            this.btnStop.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(23, 22);
            this.btnStop.Text = "toolStripButton1";
            this.btnStop.Click += new System.EventHandler(this.OnClickStop);
            // 
            // FormPipeline
            // 
            this.ClientSize = new System.Drawing.Size(637, 380);
            this.Controls.Add(this.toolStrip);
            this.Controls.Add(this.picInput);
            this.Name = "FormPipeline";
            this.ShowIcon = false;
            this.Text = "Pipeline";
            ((System.ComponentModel.ISupportInitialize)(this.picInput)).EndInit();
            this.toolStrip.ResumeLayout(false);
            this.toolStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private Timer stepTimer;
        private IContainer components;
        private ToolStrip toolStrip;
        private ToolStripButton btnStart;
        private ToolStripButton btnStop;
        private PictureBox picInput;

        private void TimerTick(object sender, EventArgs e)
        {
            if(!_oeip.Step())
            {
                stepTimer.Stop();
            }
        }

        private void OnClickStart(object sender, EventArgs e)
        {
            stepTimer.Start();
        }

        private void OnClickStop(object sender, EventArgs e)
        {
            stepTimer.Stop();
        }
    }
}
