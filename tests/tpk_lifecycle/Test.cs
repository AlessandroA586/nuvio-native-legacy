// Compila o Video.cs REAL contra a superficie usada do Tizen.Multimedia.
// Contrato (#412): nenhuma API do player toca o fio principal; o descarte
// trava/pifa vira EV_ERRO para o C em vez de encerrar o processo, e um player
// novo so nasce depois do Dispose do anterior (o decoder da TV e um so).
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using NuvioTpk;
using Tizen.Multimedia;
class Test {
    static readonly Queue<Action> queue = new Queue<Action>();
    static int failures;
    static Video video;
    static readonly List<int> erros = new List<int>();
    static int Erros { get { lock (erros) return erros.Count; } }
    static void Check(bool ok, string what) { Console.WriteLine((ok ? "ok: " : "FAIL: ") + what); if (!ok) failures++; }
    static void Entry(string field, params object[] args) => ((Delegate)typeof(Video).GetField(field, BindingFlags.Instance|BindingFlags.NonPublic).GetValue(video)).DynamicInvoke(args);
    static void Pump() { for (;;) { Action a; lock (queue) { if (queue.Count == 0) return; a = queue.Dequeue(); } a(); } }
    static void Wait(Func<bool> ready) { var sw = Stopwatch.StartNew(); while (!ready() && sw.ElapsedMilliseconds < 3000) { Pump(); Thread.Sleep(1); } Check(ready(), "host respondeu a tempo"); }
    static void Setup() {
        lock (queue) queue.Clear();
        Player.All.Clear(); Player.Defer = false; Player.Overlap = 0; lock (erros) erros.Clear();
        video = new Video(() => new Display(), a => { lock (queue) queue.Enqueue(a); }, "/tmp", 1920, 1080);
        typeof(Video).GetField("PrazoPararMs", BindingFlags.Instance|BindingFlags.NonPublic)?.SetValue(video, 150);
    }
    static void Open() {
        var p = Marshal.StringToHGlobalAnsi("https://test/movie");
        try { Entry("fAbrir", p, IntPtr.Zero); } finally { Marshal.FreeHGlobal(p); }
    }
    static void OpenTocado() { Open(); Wait(() => Player.All.Count > 0 && Player.All[Player.All.Count - 1].Starts == 1); }
    static int Main() {
        NvVid.Registrar = (a,b,c,d,e,f,g) => {}; NvVid.RegistrarFaixas = a => {}; NvVid.RegistrarAspecto = a => {};
        NvVid.Evento = (t,a,b) => { if (t == 5) lock (erros) erros.Add(a); };
        NvVid.LogNativo = Console.WriteLine;
        NvVid.Faixa = (a,b,c) => {}; NvVid.FaixasFim = (a,b) => {};

        // Parar e abrir: o velho sai mudo e sem plano ANTES do novo existir.
        Setup(); OpenTocado(); var old = Player.All[0];
        Entry("fParar"); Open();
        Wait(() => Player.All.Count == 2 && Player.All[1].Starts == 1);
        Check(old.Disposed && old.Muted && old.Display == null, "mudo, plano solto e dispose antes do proximo player");
        Check(Player.Overlap == 0, "nenhuma sobreposicao de players");
        Entry("fParar"); Wait(() => Player.All[1].Disposed);

        // Dispose recusado: sem player novo, o C fica sabendo, o processo segue.
        Setup(); OpenTocado(); old = Player.All[0]; old.FailDispose = true;
        Entry("fParar"); Open();
        Thread.Sleep(400); Pump();
        Check(Player.All.Count == 1, "dispose recusado impede abertura seguinte");
        Check(Erros > 0, "descarte falho vira erro para o C, nao saida do processo");

        // Dispose preso: parar de novo nao re-arma nem duplica o erro; o prazo
        // vence sem player novo; destravou, a abertura seguinte funciona.
        Setup(); OpenTocado(); old = Player.All[0]; old.BlockDispose = true;
        Entry("fParar"); Entry("fParar"); Open();
        Check(old.DisposeEntered.Wait(1000), "Dispose nativo entrou");
        Thread.Sleep(400); Pump();
        Check(Player.All.Count == 1, "Dispose preso: nenhum player novo sem o Dispose");
        Check(Erros == 1, "Dispose preso: um erro por prazo, parada repetida nao duplica");
        old.DisposeRelease.Set();
        Wait(() => old.Disposed);
        Open(); Wait(() => Player.All.Count == 2 && Player.All[1].Starts == 1);
        Check(Player.Overlap == 0, "destravado, o player seguinte abre sem sobrepor");
        Entry("fParar"); Wait(() => Player.All[1].Disposed);

        // Posicao: o C le o cache, nunca espera o host.
        Setup(); OpenTocado();
        var sw = Stopwatch.StartNew(); for (int i = 0; i < 10000; i++) Entry("fPos");
        Check(sw.ElapsedMilliseconds < 100, "leituras da posicao nao esperam o host");
        Entry("fParar"); Wait(() => Player.All[0].Disposed);

        // Voltar em Preparing: sem Unprepare, dispose direto; o prepare tardio
        // nao inicia nem destroi de novo (a continuacao morreu com a sessao).
        Setup(); Player.Defer = true; Open();
        Wait(() => Player.All.Count > 0 && Player.All[0].State == PlayerState.Preparing);
        old = Player.All[0];
        Entry("fParar");
        Wait(() => old.Disposed);
        Check(old.UnprepareCalls == 0, "Voltar em Preparing nao chama Unprepare");
        old.Prepared.SetResult(true); Thread.Sleep(100); Pump();
        Check(old.Starts == 0 && old.DisposeCalls == 1, "prepare tardio nao inicia nem destroi novamente");

        // Troca em Preparing: o velho e liberado antes de o novo abrir.
        Setup(); Player.Defer = true; Open();
        Wait(() => Player.All.Count > 0 && Player.All[0].State == PlayerState.Preparing);
        old = Player.All[0];
        Player.Defer = false; Open();
        Wait(() => Player.All.Count == 2 && Player.All[1].Starts == 1);
        Check(old.Disposed && old.UnprepareCalls == 0 && Player.Overlap == 0,
              "troca em Preparing libera antes de abrir, sem sobrepor");
        old.Prepared.SetResult(true); Thread.Sleep(100); Pump();
        Check(old.Starts == 0 && old.DisposeCalls == 1, "prepare antigo nao inicia apos troca");
        Entry("fParar"); Wait(() => Player.All[1].Disposed);
        return failures == 0 ? 0 : 1;
    }
}
namespace Tizen.Multimedia {
    public enum PlayerState { Idle, Preparing, Ready, Playing, Paused }
    public enum PlayerDisplayMode { LetterBox, Roi, FullScreen }
    public class Rectangle { public Rectangle(int x,int y,int w,int h) {} }
    public class Display {}
    public class MediaUriSource { public MediaUriSource(string s) {} }
    public class Info { public int Selected {get;set;} public int GetCount()=>1; public string GetLanguageCode(int i)=>"eng"; }
    public class Props { public int Channels=2, SampleRate=48000; public Props Size=>this; public int Width=1920, Height=1080; }
    public class Stream { public int GetDuration()=>60000; public Props GetVideoProperties()=>new Props(); public Props GetAudioProperties()=>new Props(); }
    public class Settings { public PlayerDisplayMode Mode {get;set;} public void SetRoi(Rectangle r) {} }
    public class Event : EventArgs { public int Error,Percent,Duration; public string Reason,Text; }
    public class Player {
        public IntPtr Handle => new IntPtr(1);
        public static List<Player> All = new List<Player>(); public static bool Defer; public static int Overlap;
        public TaskCompletionSource<bool> Prepared = new TaskCompletionSource<bool>();
        public Player() { if(All.Exists(p=>!p.Disposed)) Overlap++; All.Add(this); }
        public bool Disposed, FailDispose, BlockDispose, BlockStop, FailStop, FailUnprepare;
        public ManualResetEventSlim StopEntered = new ManualResetEventSlim(), StopRelease = new ManualResetEventSlim(); public int Starts, DisposeCalls, UnprepareCalls;
        public ManualResetEventSlim DisposeEntered = new ManualResetEventSlim(), DisposeRelease = new ManualResetEventSlim();
        public PlayerState State {get;set;} public bool Muted {get;set;} public float Volume {get;set;}
        public Display Display {get;set;} public string UserAgent,Cookie;
        public Info AudioTrackInfo = new Info(), SubtitleTrackInfo = new Info(); public Stream StreamInfo = new Stream(); public Settings DisplaySettings = new Settings();
        public event EventHandler<Event> PlaybackCompleted, ErrorOccurred, BufferingProgressChanged, PlaybackInterrupted, SubtitleUpdated;
        public void SetSource(MediaUriSource s) {} public void SetSubtitleOffset(int n) {} public void SetPlaybackRate(float n) {}
        public async Task PrepareAsync() { State=PlayerState.Preparing; if(Defer) await Prepared.Task; if(!Disposed) State=PlayerState.Ready; }
        public void Start() { Starts++; State=PlayerState.Playing; } public void Stop() { StopEntered.Set(); if(BlockStop) StopRelease.Wait(); if(FailStop) throw new Exception("stop recusado"); State=PlayerState.Ready; }
        public void Pause() { State=PlayerState.Paused; } public void Unprepare() { UnprepareCalls++; if(State != PlayerState.Ready && State != PlayerState.Playing && State != PlayerState.Paused) throw new InvalidOperationException("unprepare em " + State); if(FailUnprepare) throw new Exception("unprepare recusado"); State=PlayerState.Idle; }
        public void Dispose() { DisposeCalls++; DisposeEntered.Set(); if(BlockDispose) DisposeRelease.Wait(); if(FailDispose) throw new Exception("dispose recusado"); Disposed=true; }
        public int PositionDelayMs, PositionCalls;
        public ManualResetEventSlim PositionEntered = new ManualResetEventSlim();
        public int GetPlayPosition() { Interlocked.Increment(ref PositionCalls); PositionEntered.Set(); Thread.Sleep(PositionDelayMs); return 1234; }
        public Task SetPlayPositionAsync(int ms,bool precise)=>Task.CompletedTask;
    }
}
