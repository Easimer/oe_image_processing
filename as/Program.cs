using System;

namespace Net.Easimer.KAA.Front
{
    class Program
    {
        static void Main(string[] args)
        {
            using (var oeip = Oeip.Create("video.mp4"))
            {
                var res = oeip.Step();
                Console.WriteLine($"Step res: {res}");
            }
        }
    }
}