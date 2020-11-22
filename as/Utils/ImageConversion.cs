using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace Net.Easimer.KAA.Front
{
    static class ImageConversion
    {
        public static Bitmap CreateRGB(IntPtr buffer, int width, int height, int stride)
        {
            return CreateBitmapAndTransform(width, height, bmp =>
            {
                unsafe
                {
                    var dst = (uint *)bmp.Scan0;
                    var src = (byte *)buffer;
                    var srcElemStride = stride;
                    var dstElemStride = bmp.Stride / 4;

                    for(int y = 0; y < height; y++)
                    {
                        for(int x = 0; x < width; x++)
                        {
                            var r = (uint)src[x * 3 + 0];
                            var g = (uint)src[x * 3 + 1];
                            var b = (uint)src[x * 3 + 2];

                            var v = 0xFF000000 | (b << 16) | (g << 8) | (r << 0);

                            dst[x] = v;
                        }

                        src += srcElemStride;
                        dst += dstElemStride;
                    }
                }
            });
        }

        public static Bitmap CreateGrayscale8(IntPtr buffer, int width, int height, int stride)
        {
            return CreateBitmapAndTransform(width, height, bmp =>
            {
                unsafe
                {
                    var dst = (uint*)bmp.Scan0;
                    var src = (byte*)buffer;
                    var srcElemStride = stride;
                    var dstElemStride = bmp.Stride / 4;

                    for (int y = 0; y < height; y++)
                    {
                        for (int x = 0; x < width; x++)
                        {
                            var r = (uint)src[x];

                            var v = 0xFF000000 | (r << 16) | (r << 8) | (r << 0);

                            dst[x] = v;
                        }

                        src += srcElemStride;
                        dst += dstElemStride;
                    }
                }
            });
        }

        public static Bitmap CreateGrayscale16(IntPtr buffer, int width, int height, int stride)
        {
            return CreateBitmapAndTransform(width, height, bmp =>
            {
                unsafe
                {
                    var dst = (uint*)bmp.Scan0;
                    var src = (short*)buffer;
                    var srcElemStride = stride / 2;
                    var dstElemStride = bmp.Stride / 4;

                    for (int y = 0; y < height; y++)
                    {
                        for (int x = 0; x < width; x++)
                        {
                            var r = (uint)src[x] / 256;

                            var v = 0xFF000000 | (r << 16) | (r << 8) | (r << 0);

                            dst[x] = v;
                        }

                        src += srcElemStride;
                        dst += dstElemStride;
                    }
                }
            });
        }

        private static Bitmap CreateBitmapAndTransform(int width, int height, Action<BitmapData> transformation)
        {
            var img = new Bitmap(width, height, PixelFormat.Format32bppArgb);
            var rect = new Rectangle(0, 0, width, height);
            var dat = img.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);

            transformation(dat);

            img.UnlockBits(dat);

            return img;
        }
    }
}