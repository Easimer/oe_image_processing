using System;
using System.Runtime.InteropServices;

namespace Net.Easimer.KAA.Front
{
    internal class ProcessingSession : IDisposable
    {
        private IntPtr _handle;
        private CAPI.ProgressCallback _cb_progress;

        public static ProcessingSession Make(string pathInput, string pathOutput)
        {
            var handle = CAPI.OpenVideo(pathInput, pathOutput);
            if(handle == IntPtr.Zero)
            {
                return null;
            }

            return new ProcessingSession(handle);
        }

        protected ProcessingSession(IntPtr handle)
        {
            _handle = handle;

            _cb_progress = new CAPI.ProgressCallback(ProgressCallback);
            CAPI.RegisterProgressCallback(_handle, _cb_progress, 0x0F);
        }

        public bool Process()
        {
            return CAPI.Process(_handle);
        }

        private void ProgressCallback(ref CAPI.ProgressInfo inf)
        {
            OnProgress?.Invoke(inf.CurrentFrame, inf.TotalFrames);
        }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                var h = _handle;
                _handle = IntPtr.Zero;
                CAPI.CloseVideo(h);
                _cb_progress = null;
            }
        }

        public delegate void ProcessingProgress(int CurrentFrame, int TotalFrames);

        public event ProcessingProgress OnProgress;

        private static class CAPI
        {
            [StructLayout(LayoutKind.Sequential)]
            public struct ProgressInfo
            {
                public int CurrentFrame;
                public int TotalFrames;
            }

            public delegate void ProgressCallback([MarshalAs(UnmanagedType.Struct)] ref ProgressInfo inf);

            [DllImport("core.dll", EntryPoint = "oeip_open_video")]
            public static extern IntPtr OpenVideo(string pathInput, string pathOutput);

            [DllImport("core.dll", EntryPoint = "oeip_close_video")]
            public static extern void CloseVideo(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_register_progress_callback")]
            public static extern bool RegisterProgressCallback(IntPtr handle, ProgressCallback fun, uint mask);

            [DllImport("core.dll", EntryPoint = "oeip_process")]
            public static extern bool Process(IntPtr handle);
        }
    }
}
