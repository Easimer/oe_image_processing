using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Net.Easimer.KAA.Front
{
    class Oeip : IDisposable
    {
        private IntPtr _handle;
        private Dictionary<Stage, Tuple<BufferColorSpace, byte[]>> _outputs;

        protected Oeip(IntPtr handle)
        {
            _handle = handle;
            _outputs = new Dictionary<Stage, Tuple<BufferColorSpace, byte[]>>();
            BenchmarkData = new Dictionary<Stage, uint>();

            OeipCApi.RegisterStageOutputCallback(_handle, StageOutputCallback);
        }

        public static Oeip Create(string pathToVideoFile)
        {
            var handle = OeipCApi.OpenVideo(pathToVideoFile);
            if(handle != IntPtr.Zero)
            {
                return new Oeip(handle);
            }

            return null;
        }

        public void Dispose()
        {
            var h = _handle;
            _handle = IntPtr.Zero;
            OeipCApi.CloseVideo(h);
        }

        public bool Step()
        {
            _outputs.Clear();
            BenchmarkData.Clear();

            var res = OeipCApi.Step(_handle);

            foreach(var kv in _outputs)
            {
                StageHasOutput?.Invoke(kv.Key, kv.Value.Item1, kv.Value.Item2);
            }

            return res;
        }

        public Dictionary<Stage, uint> BenchmarkData { get; private set; }

        protected void StageOutputCallback(Stage stage, BufferColorSpace cs, IntPtr buffer, int bytes)
        {
            var dest = new byte[bytes];
            Marshal.Copy(buffer, dest, 0, bytes);
        }

        protected void StageBenchmarkCallback(Stage stage, uint microsecs)
        {
            BenchmarkData.Add(stage, microsecs);
        }

        public event OutputCallback StageHasOutput;

        public enum Stage
        {
            Input = 0,

            CurrentEdgeBuffer,
            AccumulatedEdgeBuffer,

            Output
        }

        public enum BufferColorSpace
        {
            Gray_1D = 0,
            RGB,
            YCBCR,
            UNSPEC_3D
        }

        public delegate void OutputCallback(Stage stage, BufferColorSpace cs, byte[] buffer);
        public delegate void BenchmarkCallback(Stage stage, uint microsecs);

        private static class OeipCApi
        {
            public delegate void OutputCallback(Stage stage, BufferColorSpace cs, IntPtr buffer, int bytes);
            public delegate void BenchmarkCallback(Stage stage, uint microsecs);

            [DllImport("core.dll", EntryPoint = "oeip_open_video")]
            public static extern IntPtr OpenVideo(string path);

            [DllImport("core.dll", EntryPoint = "oeip_close_video")]
            public static extern void CloseVideo(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_step")]
            public static extern bool Step(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_register_stage_output_callback")]
            public static extern bool RegisterStageOutputCallback(IntPtr handle, OutputCallback fun);

            [DllImport("core.dll", EntryPoint = "oeip_register_stage_benchmark_callback")]
            public static extern bool RegisterStageBenchmarkCallback(IntPtr handle, Stage stage, BenchmarkCallback fun);
        }
    }
}