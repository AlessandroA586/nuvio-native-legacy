using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using NuvioTpk;
using Tizen.Multimedia;
class AspectoTest {
    [DllImport("libcapi-media-player.so.0")] static extern int probe(int i);
    [DllImport("libcapi-media-player.so.0")] static extern void fail(int op, int rc);
    static readonly Queue<Action> queue = new Queue<Action>();
    static readonly List<string> log = new List<string>();
    static Video video;
    static void Check(bool ok, string msg) { if (!ok) throw new Exception(msg); Console.WriteLine("ok: " + msg); }
    static void Entry(string name, params object[] args) => ((Delegate)typeof(Video).GetField(name, BindingFlags.NonPublic|BindingFlags.Instance).GetValue(video)).DynamicInvoke(args);
    static void Pump() { while(queue.Count > 0) queue.Dequeue()(); }
    static void Open() { var u=Marshal.StringToHGlobalAnsi("https://test/movie"); try { Entry("fAbrir",u,IntPtr.Zero); } finally { Marshal.FreeHGlobal(u); } Pump(); }
    static void Roi(int x,int y,int w,int h) { Entry("fJanela",x,y,w,h); Pump(); Check(probe(0)==(int)PlayerDisplayMode.Roi && probe(1)==x && probe(2)==y && probe(3)==w && probe(4)==h,"B ROI " + x+","+y+" "+w+"x"+h); }
    static int Main(string[] args) {
        NativeLibrary.SetDllImportResolver(typeof(Video).Assembly,(name,a,p)=>name=="libcapi-media-player.so.0" ? NativeLibrary.Load(args[0]) : IntPtr.Zero);
        NvVid.Registrar=(a,b,c,d,e,f,g)=>{}; NvVid.RegistrarFaixas=a=>{}; NvVid.RegistrarAspecto=a=>{}; NvVid.Evento=(a,b,c)=>{};
        NvVid.Faixa=(a,b,c)=>{}; NvVid.FaixasFim=(a,b)=>{}; NvVid.LogNativo=log.Add;
        // Real bridge registration is covered by the C fixture; lifecycle stubs have no libnuvio.
        video=new Video(()=>new Display(),a=>queue.Enqueue(a),System.IO.Path.GetTempPath(),1920,1080);
        Open();
        foreach(bool elm in new[]{false,true}) {
            video.GeometriaJanela=elm ? (Func<int,int,int,int,string>)((x,y,w,h)=>null) : null;
            // Original / Fill / Zoom (contain, stretch, centered cover).
            Roi(0,0,1920,1080); Roi(0,0,1920,1080); Roi(0,0,1920,1080); // 16:9
            Roi(0,138,1920,804); Roi(0,0,1920,1080); Roi(-331,0,2581,1080); // 2.39:1 (1912x800)
            Roi(240,0,1440,1080); Roi(0,0,1920,1080); Roi(0,-180,1920,1440); // 4:3
        }
        var p=Player.All[Player.All.Count-1]; int starts=p.Starts;
        video.GeometriaJanela=null;
        fail(2,-95); Entry("fAspecto",-331,0,2581,1080,0,0,1920,1080); Pump();
        Check(p.DisplaySettings.Mode==PlayerDisplayMode.LetterBox,"ROI rejection restores old 6+ fullscreen path");
        Check(probe(0)==(int)PlayerDisplayMode.LetterBox,"partial native Roi state cleared");
        Check(log.Exists(l=>l.Contains("[aspecto]") && l.Contains("0xffffffa1")),"one-line rejection reason");
        int calls=probe(5); fail(2,0); Entry("fJanela",240,0,1440,1080); Pump();
        Check(probe(5)==calls && p.DisplaySettings.Mode==PlayerDisplayMode.Roi,"fallback retained for entire session");
        Check(p.Starts==starts && !p.Muted && p.State==PlayerState.Playing,"aspect never starts, pauses or mutes");
        Entry("fAspecto",0,-180,1920,1440,0,0,1920,1080); Open();
        Check(probe(5)==calls,"old queued aspect discarded after generation change");
        Roi(240,0,1440,1080); // New session tries B again.
        foreach(bool elm in new[]{false,true}) {
            Open(); p=Player.All[Player.All.Count-1]; int wx=0,wy=0,ww=1920,wh=1080;
            video.GeometriaJanela=elm ? (Func<int,int,int,int,string>)((x,y,w,h)=>{wx=x;wy=y;ww=w;wh=h;return null;}) : null;
            fail(1,-22); calls=probe(6); Entry("fJanela",0,140,1920,800); Pump();
            Check(probe(6)==calls,"mode rejection does not mutate ROI");
            Check(p.DisplaySettings.Mode==(elm?PlayerDisplayMode.FullScreen:PlayerDisplayMode.Roi),"mode rejection uses old host path");
            Check(!elm || (wy==140 && wh==800),"old ElmSharp geometry retained");
            fail(1,0);
        }
        // Legacy window writes can also fail after moving the surface.
        Open(); p=Player.All[Player.All.Count-1];
        int gx=0,gy=0,gw=1920,gh=1080;
        video.GeometriaJanela=(x,y,w,h)=>{ gx=x;gy=y;gw=w;gh=h; return x==0 && y==0 && w==1920 && h==1080 ? null : "geometry partially applied"; };
        fail(2,-95); Entry("fJanela",240,0,1440,1080); Pump(); fail(2,0);
        Check(gx==0 && gy==0 && gw==1920 && gh==1080,"partial ElmSharp mutation rolled back before legacy Roi");
        Check(p.DisplaySettings.Mode==PlayerDisplayMode.Roi,"legacy Roi after geometry rollback");
        video.GeometriaJanela=null; p.DisplaySettings.FailRoi=true;
        Entry("fJanela",240,0,1440,1080); Pump();
        Check(p.DisplaySettings.Mode==PlayerDisplayMode.LetterBox && probe(0)==(int)PlayerDisplayMode.LetterBox,"failed legacy ROI leaves neutral mode");
        Check(log.Exists(l=>l.Contains("[aspecto]") && l.Contains("fallback=failed")),"failed restoration is not reported as success");
        p.DisplaySettings.FailRoi=false;
        video.GeometriaJanela=(x,y,w,h)=>{gx=x;gy=y;gw=w;gh=h;return null;};
        Entry("fJanela",240,0,1440,1080); Pump();
        Entry("fParar"); Pump();
        Check(gx==0 && gy==0 && gw==1920 && gh==1080,"session teardown restores legacy window");
        Console.WriteLine("tpk_aspecto: PASS (native fake, no TV proof)"); return 0;
    }
}
