using System;
using System.Collections.Generic;
using System.Drawing;
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

        public static PipelineDemoSession Create(string pathToVideoFile)
        {
            try
            {
                var handle = OeipCApi.OpenVideo(pathToVideoFile);
                if (handle != IntPtr.Zero)
                {
                    return new PipelineDemoSession(handle);
                }
            }
            catch(System.DllNotFoundException)
            {
                throw new DllNotFoundException("Can't find one or two DLLs. Make sure that front.exe's working directory has a copy of both core.dll and opencv_world440.dll.");
            }

            return null;
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
            public delegate void OutputCallback(Stage stage, BufferColorSpace cs, IntPtr buffer, int bytes, int width, int height, int stride);

            [DllImport("core.dll", EntryPoint = "oeip_open_video")]
            public static extern IntPtr OpenVideo(string pathInput, string pathOutput = null);

            [DllImport("core.dll", EntryPoint = "oeip_close_video")]
            public static extern void CloseVideo(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_step")]
            public static extern bool Step(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_register_stage_output_callback")]
            public static extern bool RegisterStageOutputCallback(IntPtr handle, OutputCallback fun);
        }
    }
}