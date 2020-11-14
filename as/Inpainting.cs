using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;

namespace Net.Easimer.KAA.Front
{
    class InpaintingSession : IDisposable
    {
        private IntPtr _handle;

        protected InpaintingSession(IntPtr handle)
        {
            _handle = handle;
        }

        public void Dispose()
        {
            API.End(_handle);
        }

        public static InpaintingSession Make(string pathSource, string pathMask)
        {
            var handle = API.Begin(pathSource, pathMask);

            if (handle == IntPtr.Zero)
            {
                return null;
            }

            return new InpaintingSession(handle);
        }

        private static class API
        {
            [DllImport("core.dll", EntryPoint = "oeip_begin_inpainting", CharSet = CharSet.Ansi)]
            public static extern IntPtr Begin(string pathSource, string pathMask);

            [DllImport("core.dll", EntryPoint = "oeip_end_inpainting")]
            public static extern void End(IntPtr handle);

            [DllImport("core.dll", EntryPoint = "oeip_inpaint")]
            public static extern void Inpaint(IntPtr handle, ref IntPtr buffer, ref int bytes, ref int width, ref int height, ref int stride);
        }
    }
}
