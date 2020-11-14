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
    partial class FormInpaint : Form
    {
        private string[] _paths = null;
        private InpaintingSession _session = null;

        protected FormInpaint(string[] paths)
        {
            InitializeComponent();

            _paths = paths;
        }

        public static FormInpaint Make()
        {
            // Bekerjuk az utvonalakat
            var paths = new string[2];
            var titles = new string[2] { "Choose an image to infill", "Choose the mask to use" };

            for(int i = 0; i < titles.Length; i++)
            {
                var dlg = new OpenFileDialog();
                dlg.Filter = "Image files (*.png, *.jpg, *.jpeg, *.bmp)|*.png;*.jpg;*.jpeg;*.bmp|All files (*.*)|*.*";
                dlg.Title = titles[i];

                var res = dlg.ShowDialog();
                if(res != DialogResult.OK)
                {
                    return null;
                }

                paths[i] = dlg.FileName;
            }

            return new FormInpaint(paths);
        }

        private void InitializeComponent()
        {
            this.toolStrip = new System.Windows.Forms.ToolStrip();
            this.btnStep = new System.Windows.Forms.ToolStripButton();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.picSource = new System.Windows.Forms.PictureBox();
            this.picMask = new System.Windows.Forms.PictureBox();
            this.picResult = new System.Windows.Forms.PictureBox();
            this.toolStrip.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picSource)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picMask)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picResult)).BeginInit();
            this.SuspendLayout();
            // 
            // toolStrip
            // 
            this.toolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btnStep});
            this.toolStrip.Location = new System.Drawing.Point(0, 0);
            this.toolStrip.Name = "toolStrip";
            this.toolStrip.Size = new System.Drawing.Size(922, 25);
            this.toolStrip.TabIndex = 0;
            this.toolStrip.Text = "toolStrip1";
            // 
            // btnStep
            // 
            this.btnStep.Image = global::Properties.Resources.go_paint;
            this.btnStep.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btnStep.Name = "btnStep";
            this.btnStep.Size = new System.Drawing.Size(50, 22);
            this.btnStep.Text = "Step";
            this.btnStep.Click += new System.EventHandler(this.OnStepClicked);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 3;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.Controls.Add(this.picSource, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.picMask, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.picResult, 2, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 25);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(922, 312);
            this.tableLayoutPanel1.TabIndex = 1;
            // 
            // picSource
            // 
            this.picSource.Dock = System.Windows.Forms.DockStyle.Fill;
            this.picSource.Location = new System.Drawing.Point(3, 3);
            this.picSource.Name = "picSource";
            this.picSource.Size = new System.Drawing.Size(301, 306);
            this.picSource.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.picSource.TabIndex = 0;
            this.picSource.TabStop = false;
            // 
            // picMask
            // 
            this.picMask.Dock = System.Windows.Forms.DockStyle.Fill;
            this.picMask.Location = new System.Drawing.Point(310, 3);
            this.picMask.Name = "picMask";
            this.picMask.Size = new System.Drawing.Size(301, 306);
            this.picMask.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.picMask.TabIndex = 1;
            this.picMask.TabStop = false;
            // 
            // picResult
            // 
            this.picResult.Dock = System.Windows.Forms.DockStyle.Fill;
            this.picResult.Location = new System.Drawing.Point(617, 3);
            this.picResult.Name = "picResult";
            this.picResult.Size = new System.Drawing.Size(302, 306);
            this.picResult.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.picResult.TabIndex = 2;
            this.picResult.TabStop = false;
            // 
            // FormInpaint
            // 
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(922, 337);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.toolStrip);
            this.Name = "FormInpaint";
            this.ShowIcon = false;
            this.Text = "Inpaint demo";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.OnFormClosing);
            this.Load += new System.EventHandler(this.OnLoad);
            this.toolStrip.ResumeLayout(false);
            this.toolStrip.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.picSource)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picMask)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picResult)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private ToolStrip toolStrip;
        private ToolStripButton btnStep;
        private TableLayoutPanel tableLayoutPanel1;
        private PictureBox picSource;
        private PictureBox picMask;
        private PictureBox picResult;

        private void DisplayResultImage(Bitmap img)
        {
            picResult.Image = img;
        }

        private async void OnStepClicked(object sender, EventArgs e)
        {
            var task = Task.Run(() =>
            {
            });

            await Task.WhenAll(task);
        }

        private void OnLoad(object sender, EventArgs e)
        {
            picSource.Image = new Bitmap(_paths[0]);
            picMask.Image = new Bitmap(_paths[1]);

            var session = InpaintingSession.Make(_paths[0], _paths[1]);

            if(session == null)
            {
                MessageBox.Show("Failed to create the inpainting session", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Close();
                return;
            }

            _session = session;
        }

        private void OnFormClosing(object sender, FormClosingEventArgs e)
        {
            if(_session != null)
            {
                _session.Dispose();
                _session = null;
            }
        }
    }
}
