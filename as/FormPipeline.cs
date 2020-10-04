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

        private void Output(Oeip.Stage stage, Bitmap buf)
        {
            OutputControl ctl = null;
            switch(stage)
            {
                case Oeip.Stage.Input:
                    ctl = imgInput;
                    break;
                case Oeip.Stage.CurrentEdgeBuffer:
                    ctl = imgEdgeCurrent;
                    break;
                case Oeip.Stage.AccumulatedEdgeBuffer:
                    ctl = imgEdgeAccumulated;
                    break;
                default:
                    break;
            }

            if(ctl == null)
            {
                return;
            }

            ctl.OutputImage = buf;
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.stepTimer = new System.Windows.Forms.Timer(this.components);
            this.toolStrip = new System.Windows.Forms.ToolStrip();
            this.btnStart = new System.Windows.Forms.ToolStripButton();
            this.btnStop = new System.Windows.Forms.ToolStripButton();
            this.imgInput = new Net.Easimer.KAA.Front.OutputControl();
            this.imgEdgeCurrent = new Net.Easimer.KAA.Front.OutputControl();
            this.imgEdgeAccumulated = new Net.Easimer.KAA.Front.OutputControl();
            this.toolStrip.SuspendLayout();
            this.SuspendLayout();
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
            this.toolStrip.Size = new System.Drawing.Size(922, 25);
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
            // imgInput
            // 
            this.imgInput.Location = new System.Drawing.Point(12, 28);
            this.imgInput.Name = "imgInput";
            this.imgInput.OutputImage = null;
            this.imgInput.OutputStageName = "Input";
            this.imgInput.Size = new System.Drawing.Size(296, 267);
            this.imgInput.TabIndex = 2;
            // 
            // imgEdgeCurrent
            // 
            this.imgEdgeCurrent.Location = new System.Drawing.Point(314, 28);
            this.imgEdgeCurrent.Name = "imgEdgeCurrent";
            this.imgEdgeCurrent.OutputImage = null;
            this.imgEdgeCurrent.OutputStageName = "Current edge buffer";
            this.imgEdgeCurrent.Size = new System.Drawing.Size(296, 267);
            this.imgEdgeCurrent.TabIndex = 3;
            // 
            // imgEdgeAccumulated
            // 
            this.imgEdgeAccumulated.Location = new System.Drawing.Point(616, 28);
            this.imgEdgeAccumulated.Name = "imgEdgeAccumulated";
            this.imgEdgeAccumulated.OutputImage = null;
            this.imgEdgeAccumulated.OutputStageName = "Accumulated edge buffer";
            this.imgEdgeAccumulated.Size = new System.Drawing.Size(296, 267);
            this.imgEdgeAccumulated.TabIndex = 4;
            // 
            // FormPipeline
            // 
            this.ClientSize = new System.Drawing.Size(922, 307);
            this.Controls.Add(this.imgEdgeAccumulated);
            this.Controls.Add(this.imgEdgeCurrent);
            this.Controls.Add(this.imgInput);
            this.Controls.Add(this.toolStrip);
            this.Name = "FormPipeline";
            this.ShowIcon = false;
            this.Text = "Pipeline";
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
        private OutputControl imgEdgeCurrent;
        private OutputControl imgEdgeAccumulated;
        private OutputControl imgInput;

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

        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);

            stepTimer.Stop();

            _oeip.Dispose();
            _oeip = null;
        }
    }
}
