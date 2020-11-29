using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;

namespace Net.Easimer.KAA.Front
{
    class PipelineDemoSession : IDisposable
    {
        private IntPtr _handle;
        private Dictionary<Stage, Bitmap> _outputs;
        private OeipCApi.OutputCallback _cb_output;

        protected PipelineDemoSession(IntPtr handle)
        {
            _handle = handle;
            _outputs = new Dictionary<Stage, Bitmap>();

            // "Pinneljuk" a delegate-et hogy ne GC-zodjon mikozben a lib meg pointert tarol ra
            _cb_output = StageOutputCallback;
            OeipCApi.RegisterStageOutputCallback(_handle, _cb_output);
        }

        public static PipelineDemoSession Create(string pathToVideoFile, bool applyOtsuBinarization)
        {
            IntPtr handle;

            try
            {
                int flags = (int)(applyOtsuBinarization ? OeipCApi.Flags.ApplyOtsuBinarization : OeipCApi.Flags.None);
                handle = OeipCApi.OpenVideo(pathToVideoFile, null, flags);
            }
            catch(DllNotFoundException ex)
            {
                throw new DllNotFoundException("Can't find one or two DLLs. Make sure that front.exe's working directory has a copy of both core.dll and opencv_world450.dll!", ex);
            }

            if (handle == IntPtr.Zero)
            {
                if(File.Exists(pathToVideoFile))
                {
                    throw new DllNotFoundException("Can't find one or more DLLs. Make sure that front.exe's working directory has a copy of both opencv_world440.dll and opencv_videoio_ffmpeg450_64.dll.");
                }
                else
                {
                    throw new FileNotFoundException($"File {pathToVideoFile} was not found!");
                }
            }

            return new PipelineDemoSession(handle);
        }

        public void Dispose()
        {
            var h = _handle;
            _handle = IntPtr.Zero;
            OeipCApi.CloseVideo(h);
            _cb_output = null;
        }

        public bool Step()
        {
            var res = OeipCApi.Step(_handle);

            foreach(var kv in _outputs)
            {
                StageHasOutput?.Invoke(kv.Key, kv.Value);
            }

            return res;
        }

        protected void StageOutputCallback(Stage stage, BufferColorSpace cs, IntPtr buffer, int bytes, int width, int height, int stride)
        {
            Bitmap img = null;

            switch(cs)
            {
                case BufferColorSpace.R8:
                    img = ImageConversion.CreateGrayscale8(buffer, width, height, stride);
                    break;
                case BufferColorSpace.R16:
                    img = ImageConversion.CreateGrayscale16(buffer, width, height, stride);
                    break;
                case BufferColorSpace.RGB888_RGB:
                    img = ImageConversion.CreateRGB(buffer, width, height, stride);
                    break;
                default:
                    return;
            }

            if(img != null)
            {
                if(_outputs.ContainsKey(stage))
                {
                    _outputs[stage].Dispose();
                }
                _outputs[stage] = img;
            }
            else
            {
                _outputs.Remove(stage);
            }
        }

        public event OutputCallback StageHasOutput;

        public enum Stage
        {
            Input = 0,

            CurrentEdgeBuffer,
            AccumulatedEdgeBuffer,
            SubtitleMask,

            Output
        }

        public enum BufferColorSpace
        {
            R8 = 0,
            R16,

            RGB888_RGB,
            RGB888_YCBCR,

            RGB888_UNSPEC
        }

        public delegate void OutputCallback(Stage stage, Bitmap buf);
        public delegate void BenchmarkCallback(Stage stage, uint microsecs);

        private static class OeipCApi
        {
            [Flags]
            public enum Flags
            {
                None = 0,
                ApplyOtsuBinarization = 1 << 0,
            }

            public delegate void OutputCallback(Stage stage, BufferColorSpace cs, IntPtr buffer, int bytes, int width, int height, int stride);

            [DllImport("core.dll", EntryPoint = "oeip_open_video", CharSet = CharSet.Ansi)]
            public static extern IntPtr OpenVideo(string pathInput, string pathOutput = null, int flags = 0);

            [DllImport("core.dll", EntryPoint = "oeip_close_video")]
            public static extern void CloseVideo(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_step")]
            public static extern bool Step(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_register_stage_output_callback")]
            public static extern bool RegisterStageOutputCallback(IntPtr handle, OutputCallback fun);
        }
    }
}