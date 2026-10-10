// Player do Nuvio .tpk, comum aos dois hosts (NUI no Tizen 6+, ElmSharp no
// 4/5). Tizen.Multimedia.Player no plano de video da TV; a janela GL fica por
// cima, translucida, e o C abre o furo onde o video aparece. O C pede pelas
// funcoes registradas em nv_tpk_video_registrar e ouve pelo
// nv_tpk_video_evento (src/video_tpk.c).
//
// Toda chamada ao player passa pelo fio principal (Principal), porque o C
// chama do fio dele. A posicao e lida pelo relogio do host (Tique) e o C so le
// o numero guardado, sem esperar ninguem.
using System;
using System.IO;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Threading;
using Tizen.Multimedia;

namespace NuvioTpk
{
    // Despacho dos pontos de entrada de video/log do libnuvio, comum aos dois
    // hosts. POR PADRAO cada delegate aponta para o [DllImport("libnuvio.so")]
    // correspondente — que resolve pelo soname. E o caminho dos hosts Tizen 6+
    // (NuvioTpk/60/65) e tambem da rota memfd do NuvioTpk40, onde a lib entra no
    // link map do loader: comportamento identico ao codigo anterior.
    //
    // So o NuvioTpk40, quando a lib e carregada pelo carregador de ELF proprio
    // (a lib NAO entra no link map, entao DllImport-por-soname NAO resolveria),
    // chama NvVid.Ligar(resolve) para repontar estes delegates para ponteiros de
    // funcao vindos do dynsym do carregador. Nada disso e acionado nos 6+.
    static class NvVid
    {
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_registrar(IntPtr abrir, IntPtr parar, IntPtr pausar,
                                                                              IntPtr buscar, IntPtr volume, IntPtr janela, IntPtr pos);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_evento(int tipo, int a, int b);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_registrar_faixas(IntPtr escolher);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_crop_register(IntPtr handler);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_crop_available(int available);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_faixa(int tipo, int idx, string lingua);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_faixas_fim(int selAudio, int selLeg);
        [DllImport("libnuvio.so")] static extern void nv_tpk_video_legenda(string texto, int durMs);
        [DllImport("libnuvio.so")] static extern void nv_tpk_log(string linha);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void RegistrarDel(IntPtr abrir, IntPtr parar, IntPtr pausar, IntPtr buscar, IntPtr volume, IntPtr janela, IntPtr pos);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void EventoDel(int tipo, int a, int b);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void RegistrarFaixasDel(IntPtr escolher);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void RegisterCropDel(IntPtr handler);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void CropAvailableDel(int available);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void FaixaDel(int tipo, int idx, string lingua);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void FaixasFimDel(int selAudio, int selLeg);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void LegendaDel(string texto, int durMs);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void LogDel(string linha);

        // Padrao: as thunks de P/Invoke acima (soname). O NuvioTpk40 repontа na
        // rota ELF.
        public static RegistrarDel Registrar = nv_tpk_video_registrar;
        public static EventoDel Evento = nv_tpk_video_evento;
        public static RegistrarFaixasDel RegistrarFaixas = nv_tpk_video_registrar_faixas;
        // Registered apart from Registrar on purpose (see nv_tpk_video_crop_register):
        // widening that signature would mean touching every host and the ELF loader.
        public static RegisterCropDel RegisterCrop = nv_tpk_video_crop_register;
        public static CropAvailableDel CropAvailable = nv_tpk_video_crop_available;
        public static FaixaDel Faixa = nv_tpk_video_faixa;
        public static FaixasFimDel FaixasFim = nv_tpk_video_faixas_fim;
        public static LegendaDel Legenda = nv_tpk_video_legenda;
        public static LogDel LogNativo = nv_tpk_log;

        // Repontа tudo por ponteiro de funcao (rota do carregador ELF do
        // NuvioTpk40). resolve(nome) devolve o endereco do simbolo no dynsym.
        public static void Ligar(Func<string, IntPtr> resolve)
        {
            IntPtr p;
            if ((p = resolve("nv_tpk_video_registrar")) != IntPtr.Zero) Registrar = Marshal.GetDelegateForFunctionPointer<RegistrarDel>(p);
            if ((p = resolve("nv_tpk_video_evento")) != IntPtr.Zero) Evento = Marshal.GetDelegateForFunctionPointer<EventoDel>(p);
            if ((p = resolve("nv_tpk_video_registrar_faixas")) != IntPtr.Zero) RegistrarFaixas = Marshal.GetDelegateForFunctionPointer<RegistrarFaixasDel>(p);
            if ((p = resolve("nv_tpk_video_crop_register")) != IntPtr.Zero) RegisterCrop = Marshal.GetDelegateForFunctionPointer<RegisterCropDel>(p);
            if ((p = resolve("nv_tpk_video_crop_available")) != IntPtr.Zero) CropAvailable = Marshal.GetDelegateForFunctionPointer<CropAvailableDel>(p);
            if ((p = resolve("nv_tpk_video_faixa")) != IntPtr.Zero) Faixa = Marshal.GetDelegateForFunctionPointer<FaixaDel>(p);
            if ((p = resolve("nv_tpk_video_faixas_fim")) != IntPtr.Zero) FaixasFim = Marshal.GetDelegateForFunctionPointer<FaixasFimDel>(p);
            if ((p = resolve("nv_tpk_video_legenda")) != IntPtr.Zero) Legenda = Marshal.GetDelegateForFunctionPointer<LegendaDel>(p);
            if ((p = resolve("nv_tpk_log")) != IntPtr.Zero) LogNativo = Marshal.GetDelegateForFunctionPointer<LogDel>(p);
        }
    }

    class Video
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnAbrir(IntPtr url, IntPtr cabecalhos);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnSemArg();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnInt(int v);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnRet(int x, int y, int w, int h);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int FnPos();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void FnEscolher(int tipo, int idx);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int FnCropSource(double rx, double ry, double rw, double rh, int dx, int dy, int dw, int dh);

        const int EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5, EV_TAMANHO = 6, EV_BUFFER = 7, EV_VELOCIDADE = 8;

        readonly Func<Display> fazDisplay;
        readonly Action<Action> principal;
        readonly string logArq;
        readonly int telaW, telaH;
        // Referencias vivas: o C guarda os ponteiros, o GC nao pode recolher.
        FnAbrir fAbrir; FnSemArg fParar; FnInt fPausar, fBuscar, fVolume; FnRet fJanela; FnPos fPos; FnEscolher fEscolher;
        FnCropSource fCropSource;

        readonly VideoWindowMetrics windowMetrics = new VideoWindowMetrics();
        Player player;
        int sessao, playerSessao;
        volatile int posMs;

        public Video(Func<Display> fazDisplay, Action<Action> principal, string dados, int telaW, int telaH)
        {
            this.fazDisplay = fazDisplay;
            this.principal = principal;
            this.telaW = telaW;
            this.telaH = telaH;
            logArq = Path.Combine(dados, "tpk-host.log");
            fAbrir = (u, c) => { string url = Marshal.PtrToStringAnsi(u), cab = Marshal.PtrToStringAnsi(c); Trocar(minha => Abrir(url, cab, minha)); };
            fParar = Parar;
            fPausar = p => ComSessao(() => Pausar(p != 0));
            fBuscar = ms => ComSessao(() => Buscar(ms));
            fVolume = v => ComSessao(() => PedirVolume(v));
            fJanela = (x, y, w, h) => ComSessao(() => Janela(x, y, w, h));
            fPos = () => posMs;
            NvVid.Registrar(Marshal.GetFunctionPointerForDelegate(fAbrir), Marshal.GetFunctionPointerForDelegate(fParar),
                                   Marshal.GetFunctionPointerForDelegate(fPausar), Marshal.GetFunctionPointerForDelegate(fBuscar),
                                   Marshal.GetFunctionPointerForDelegate(fVolume), Marshal.GetFunctionPointerForDelegate(fJanela),
                                   Marshal.GetFunctionPointerForDelegate(fPos));
            fEscolher = (tipo, idx) => ComSessao(() => Escolher(tipo, idx));
            NvVid.RegistrarFaixas(Marshal.GetFunctionPointerForDelegate(fEscolher));
            // THE CROP IS A CROSS-THREAD CALL and is marshalled like every other one
            // in this list. The C side asks for it from `video_bombear()`, which runs
            // on the DRAW thread (nv_tpk_quadro), not on the host's main thread.
            // Touching Tizen.Multimedia.Player from the wrong thread is undefined
            // behaviour, and it is what tore the picture and the controls down while
            // the aspect key was being pressed.
            fCropSource = (rx, ry, rw, rh, dx, dy, dw, dh) => CropSourcePost(rx, ry, rw, rh, dx, dy, dw, dh);
            NvVid.RegisterCrop(Marshal.GetFunctionPointerForDelegate(fCropSource));
            ProbeCrop();
        }

        // WHETHER THIS TV CAN CROP THE SOURCE, asked once and only at runtime.
        //
        // The call exists from Tizen 5 (TizenFX: Player.SetVideoRoi, since_tizen 5)
        // and the reference assembly the Tizen 4 host compiles against does NOT
        // declare it - a direct call fails to build (CS1061) and ScaleRectangle
        // does not exist at all (CS0246). Reflection is therefore the only way to
        // reach it from this package, and it is the same trick AppConfig already
        // uses here.
        //
        // A missing method must cost the zoom and NOTHING else, so the probe only
        // records what it found and never throws. The C side asks through
        // video_recorte_fonte(), and the probe's verdict goes to the log either
        // way: without that line the next log cannot tell "the TV has no such
        // call" from "the call is there and did not work".
        void ProbeCrop() {
            string method = null, kind = null, erro = null;
            try {
                var t = typeof(Player);
                var m = t.GetMethod("SetVideoRoi");
                if (m == null) erro = "Player.SetVideoRoi not found";
                else {
                    method = m.Name;
                    var p = m.GetParameters();
                    kind = p.Length + " arg(s): " + p[0].ParameterType.FullName;
                }
            }
            catch (Exception e) { erro = e.GetType().Name + ": " + e.Message; }

            cropSupported = erro == null;
            try { NvVid.CropAvailable(erro == null ? 1 : 0); } catch { }
            Log("[aspect] source crop probe: " + (cropSupported ? "yes" : "no")
                + (method != null ? " (" + method + ", " + kind + ")" : "")
                + (erro != null ? " (" + erro + ")" : ""));
        }

        // A REFUSAL IS EVIDENCE, and it is permanent.
        //
        // Finding the method is NOT proof it works: it is measured on a Tizen 6
        // set (QN85Q70AAGXZD) that SetVideoRoi exists and every in-range value
        // comes back NotSupportedErr. So the first real refusal revokes support,
        // and the C side stops offering the crop modes at once - otherwise the
        // button would again offer three modes that cannot do anything, which is
        // the exact report this change exists to close.
        void RevokeCrop(string why) {
            if (!cropSupported) return;
            cropSupported = false;
            try { NvVid.CropAvailable(0); } catch { }
            Log("[aspect] source crop revoked: " + why);
        }
        bool cropSupported;

        // Applies the crop the C side computed, through the probed method.
        // Returns 1 when the TV took it.
        //
        // THREE STEPS, in this order, because both API sets say so:
        //   - the display mode must be ROI for a video ROI to be honoured (native
        //     docs: "the ROI area is valid only in PLAYER_DISPLAY_MODE_DST_ROI");
        //   - the DESTINATION is where the caller wants the picture: the whole
        //     screen for the player, a smaller box for the hero trailer. It is
        //     always on-screen, which is the whole point - the crop replaces the
        //     old trick of pushing the destination off the panel;
        //   - the crop itself, ratios of the frame, so it is never negative.
        //
        // The ScaleRectangle argument is built by reflection too: the type does
        // not exist in the Tizen 4 reference assembly this host compiles against.
        // Any step failing means no crop, and the caller falls back to the old
        // destination trick - a crop that half-applied would be worse than none.
        // Hands the crop to the main thread and answers the C side AT ONCE.
        //
        // The real result cannot be returned: Principal() only POSTS, so there is
        // nothing to return yet. The answer is the PROBE's verdict instead - whether
        // this TV has the call at all - which is the question the C side is really
        // asking ("may I offer the zoom modes?"). A refusal found later revokes that
        // verdict through RevokeCrop, so a TV that has the method but will not honour
        // it still loses the modes rather than leaving them lying.
        //
        // A cached previous result would be worse than useless here: the first call
        // would answer 0 and the C side would fall back to the off-screen
        // destination trick, the very path this platform refuses.
        int CropSourcePost(double rx, double ry, double rw, double rh,
                           int dx, int dy, int dw, int dh) {
            if (!cropSupported) return 0;
            Principal(() => CropSource(rx, ry, rw, rh, dx, dy, dw, dh));
            return 1;
        }

        // WHAT THE PLANE ACTUALLY HOLDS, read back.
        //
        // Every write here returns without throwing, and that is only the setter's own
        // answer - it says nothing about what the plane kept. MEASURED on this TV: going
        // back to Original was accepted TWICE (paused, and again once playing) and the
        // picture stayed zoomed. Whether the identity write landed or was ignored cannot
        // be told from the writes; it needs a read. GetVideoRoi is the readback the API
        // offers (the C# facade does not declare it in the Tizen 4 reference assembly, so
        // it is reached the same way SetVideoRoi is).
        System.Reflection.MethodInfo getRoi;
        int getRoiProbe;
        string ReadRoi() {
            if (getRoiProbe == 0)
            {
                getRoiProbe = 1;
                try { getRoi = typeof(Player).GetMethod("GetVideoRoi"); } catch { }
                getRoiProbe = getRoi == null ? -1 : 2;
                Log("[aspect] roi readback: " + (getRoi == null ? "GetVideoRoi NOT on this TV" : "GetVideoRoi present"));
            }
            if (getRoiProbe < 0 || player == null) return "";
            string modo = "?";
            try { modo = player.DisplaySettings.Mode.ToString(); } catch { }
            try
            {
                object r = getRoi.Invoke(player, null);
                if (r == null) return " [mode=" + modo + " roi=null]";
                var t = r.GetType();
                string F(string n) {
                    var pr = t.GetProperty(n);
                    return pr == null ? "?" : Convert.ToString(pr.GetValue(r), CultureInfo.InvariantCulture);
                }
                return " [mode=" + modo + " roi=" + F("ScaleX") + "," + F("ScaleY") + " "
                       + F("ScaleWidth") + "x" + F("ScaleHeight") + "]";
            }
            catch (Exception e) { return " [mode=" + modo + " roi read failed: " + Unwrap(e) + "]"; }
        }

        // A CROP OF THE SOURCE, IN RATIOS OF THE FRAME. Everything else about how
        // the picture is fitted belongs to the WINDOW path (Janela/JanelaTizen45),
        // which owns DisplaySettings.Mode and DisplaySettings.SetRoi.
        //
        // THE FULL FRAME IS A RESET and must not touch those two. The window path
        // has just set the destination and the fit mode (LetterBox for full screen),
        // and writing Mode here undid it - the whole-frame modes then showed whatever
        // Mode last won, and the controls looked like they had fallen over. Measured:
        // video_janela runs first on that path, then this, so the last writer won.
        //
        // A real crop (a slice smaller than the frame) does set both: that is the
        // combination measured to work on this TV, and it is the only path that has
        // ever shown a zoom. Copied from the run that produced four `applied` crops.
        int CropSource(double rx, double ry, double rw, double rh,
                         int dx, int dy, int dw, int dh) {
            if (player == null) return 0;
            try
            {
                var m = typeof(Player).GetMethod("SetVideoRoi");
                if (m == null) return 0;
                var st = m.GetParameters()[0].ParameterType;
                var ca = st.GetConstructor(new[] { typeof(double), typeof(double), typeof(double), typeof(double) });
                if (ca == null) { Log("[aspect] source crop: " + st.Name + " has no (double x4) constructor"); return 0; }

                // NO HOLD HERE. The app decides whether a crop may go out; the host
                // only carries it out. Holding it on `player.State == Paused` was
                // wrong twice over: that state oscillates on its own (the platform
                // pauses internally while buffering), so crops were held while the
                // film was running, and the flush that "sent them on resume" was
                // itself what called Start() - the film resumed exactly because a
                // crop had been held. The pause intent lives on the C side, where
                // the app already tracks it, and it re-asserts there.
                return CropApply(m, ca, rx, ry, rw, rh, dx, dy, dw, dh);
            }
            catch (Exception e) { RevokeCrop(Unwrap(e)); return 0; }
        }

        int CropApply(System.Reflection.MethodInfo m, System.Reflection.ConstructorInfo ca,
                      double rx, double ry, double rw, double rh,
                      int dx, int dy, int dw, int dh) {
            bool wholeFrame = rx <= 0.0 && ry <= 0.0 && rw >= 1.0 && rh >= 1.0;
            if (!wholeFrame) {
                if (dw < 1 || dh < 1) return 0;
                player.DisplaySettings.Mode = PlayerDisplayMode.Roi;
                player.DisplaySettings.SetRoi(new Rectangle(dx, dy, dw, dh));
                roiInForce = true;
                // A new zoom supersedes any release that was still owed: that release
                // belonged to the zoom this one replaces, and replaying it later would
                // write the fit that the window path owns.
                releaseAtRisk = false;
            }
            // THE STATE AROUND THE CALL, both sides of it. Whether SetVideoRoi resumes
            // playback decides a lot here, and the answer is not a clean yes or no: the
            // state either side of the call reads Paused -> Paused, every time, so the
            // call does not resume synchronously - but the vendor's own note says it
            // resumes playback by force during buffering, and the app's pause guard has
            // caught the pipeline playing on its own. The C side therefore re-asserts the
            // pause after every crop.
            if (wholeFrame) Log("[aspect] source crop: before write" + ReadRoi());
            string antes = "?", depois = "?";
            try { antes = player.State.ToString(); } catch { }
            // BEFORE the crop, not after. A paused crop is presented by resuming the
            // film (see StartRepaint), and where that resume begins is not ours to
            // choose - MEASURED: the player already reads Playing on the NEXT press, so
            // something resumed it during the previous crop. Muting only after the call
            // leaves the first frames audible.
            if (antes == "Paused") RepaintMute(true);

            // RELEASING A ZOOM WRITES THE IDENTITY WHILE THE PLANE IS STILL IN ROI MODE.
            //
            // The order is the difference between the two paths, and it is the one thing
            // they do not share. Zooming IN sets Mode=Roi and only then writes the slice,
            // and that works. Coming OUT gets Mode=LetterBox from the window path FIRST
            // (video_janela runs before this on that path), and the identity write then
            // follows - and the picture stays zoomed, on a write the setter accepted.
            //
            // The API note on the display ROI is that the area "is valid only in
            // PLAYER_DISPLAY_MODE_DST_ROI display mode". If the SOURCE ROI is honoured
            // only in Roi mode as well, then the identity write below is dropped for
            // exactly this reason, and the plane keeps the slice the zoom left on it.
            // MEASURED, not assumed: the readback on both sides of the write says which
            // of the two the TV did, so a wrong guess costs one log line.
            // A RELEASE MADE WHILE PAUSED IS AT RISK, and the retry has to repeat the whole
            // sequence, not just the identity write. The C side re-sends the whole frame
            // once the film is running (see cropSendOwed), and by then `roiInForce` is
            // already false - so the retry used to skip the mode dance and repeat the very
            // write that had just been ignored. That is the case this flag covers.
            bool releasing = wholeFrame && (roiInForce || releaseAtRisk);
            if (releasing)
            {
                try { player.DisplaySettings.Mode = PlayerDisplayMode.Roi; }
                catch (Exception e) { Log("[aspect] source crop: could not re-enter roi to release: " + Unwrap(e)); }
            }
            // The identity ROI is sent in both cases: a previous zoom left one on the
            // plane and it has to come off, and 0,0,1,1 is the whole frame.
            m.Invoke(player, new[] { ca.Invoke(new object[] { rx, ry, rw, rh }) });
            if (wholeFrame) Log("[aspect] source crop: after SetVideoRoi(" + rx + "," + ry + " " + rw + "x" + rh + ")"
                + ReadRoi());
            // COMING BACK OUT OF A ZOOM IS ITS OWN WRITE.
            //
            // MEASURED on the TV: Original from the last zoomed mode left the picture
            // zoomed. The way in sets Mode=Roi plus SetRoi, and coming out only sent the
            // identity crop and asked the window path for LetterBox - the plane stayed
            // in Roi with the destination the zoom had left on it, so the frame was
            // still cropped. The identity crop alone is not what undoes a zoom.
            //
            // THIS IS GATED ON OUR OWN MARK, and that is the whole reason it is safe.
            // The window path (Janela/JanelaTizen45) also fits the picture, and
            // overwriting what it just set is the bug this file already paid for once.
            // Writing Mode here again is only allowed when THIS code is the thing that
            // set Roi, so a mode the window path fitted is never touched.
            if (releasing)
            {
                roiInForce = false;
                player.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                if (dw >= 1 && dh >= 1) player.DisplaySettings.SetRoi(new Rectangle(dx, dy, dw, dh));
                // Released while the film was stopped: the plane may have dropped it, so the
                // next whole-frame write - the retry, once the film runs - must do this again.
                releaseAtRisk = antes == "Paused";
                Log("[aspect] source crop: zoom released (letterbox " + dx + "," + dy + " " + dw + "x" + dh + ")"
                    + (releaseAtRisk ? " while paused, so the release stays owed" : "") + ReadRoi());
            }
            try { depois = player.State.ToString(); } catch { }
            Log("[aspect] source crop applied; player " + antes + " -> " + depois);

            // A CROP LANDS ON A FRAME DECODED AFTER THE CALL.
            //
            // MEASURED on the TV. A paused plane does not recomposite because the
            // crop changed: the crop is a property of the presentation, not of the
            // frame. Two cheaper levers were tried and both fail. Taking the display
            // out and putting it back makes the compositor re-add the plane and
            // re-present the frame it already had, which shows as a black flash and
            // no change. What is needed is for the film to run far enough for the frame
            // to be presented.
            //
            // So the repaint resumes the film and stops it again as soon as the
            // decoder has moved on, which is the only "a frame exists now" evidence
            // this TV gives. A full seek also works and costs 2.7-3.7 s (measured,
            // three presses), which is why this does not use one.
            if (depois == "Paused") StartRepaint();
            // The mute is only handed over when a repaint takes responsibility for it,
            // and while one runs the mute is ITS: the owed write is re-sent inside that
            // same window, so taking the mute back here would unmute mid-repaint.
            else if (!repaintActive) RepaintMute(false);
            return 1;
        }

        // True while a zoom this code applied is still on the plane. Cleared when the
        // full frame takes it back out, and on teardown.
        bool roiInForce, releaseAtRisk;
        bool repaintActive, repaintMuted, repaintMutedSet, repaintEventWarned;
        long repaintStart, repaintDeadline;
        int repaintSessao, repaintStartPos;
        // HOW FAR THE FILM MUST MOVE BEFORE THE REPAINT IS DONE.
        //
        // -1 MEANS WAIT FOR NOTHING: Start() and Pause() in the same turn, with no poll
        // and no position read at all. The frame comes back because Start() alone makes
        // the sink present it.
        //
        // IT DOES NOT STOP THE FILM MOVING. MEASURED on the TV, with this at -1: the
        // position still advances 37 ms per press, and the pair takes 40-50 ms of wall
        // time. Start() is asynchronous, so the player really resumes and decodes before
        // the Pause lands, and no value here changes that. Setting 0 did not either: the
        // film then moved 49-71 ms, because the wait was spent waiting for the position
        // to reach where it already was.
        //
        // So this chooses whether to WAIT for a position, not whether the film advances.
        // A positive value waits for the clock first, which is what 0 and 40 did.
        const int REPAINT_ADVANCE_MS = -1;
        // The ceiling, for a stream that never moves at all. It is a fallback, not the
        // plan, and the log says which of the two ended the wait.
        const long REPAINT_BUDGET_MS = 700;
        const int REPAINT_POLL_MS = 8;
        EventHandler<VideoFrameDecodedEventArgs> repaintHandler;

        // SILENCE, and say so. Every branch is logged because a silent failure here is
        // exactly what produced audio over a paused picture with nothing in the log to
        // show for it.
        void RepaintMute(bool ligar) {
            if (player == null) return;
            if (ligar)
            {
                bool primeiro = !repaintMutedSet;
                try
                {
                    if (primeiro) repaintMuted = player.Muted;
                    // WRITTEN AGAIN ON EVERY CALL, not only the first. The player is muted
                    // while Paused and then Start() is called, and a player that drops the
                    // flag across that state change is exactly how the film was heard.
                    if (!repaintMuted) player.Muted = true;
                    repaintMutedSet = !repaintMuted;
                }
                catch (Exception e)
                {
                    if (primeiro) repaintMutedSet = false;
                    Log("[aspect] source crop: mute refused: " + Unwrap(e));
                }
                // READ IT BACK, EVERY TIME, AND ASSERT AGAIN AFTER Start(). Reading only
                // the value BEFORE the write cannot tell "the write was refused" from
                // "the write was accepted and the player did not keep it" -
                // it only ever printed the value it read BEFORE writing. This is also
                // asserted again after Start(): the state change is the obvious moment for
                // a player to drop the flag.
                string lido = "?";
                try { lido = player.Muted.ToString(); } catch { }
                Log("[aspect] source crop: repaint mute " + (repaintMutedSet ? "on" : "NOT SET")
                    + " (player says muted=" + lido + ", we set it " + (repaintMutedSet ? "from " + repaintMuted : "not at all") + ")");
                return;
            }
            if (!repaintMutedSet) return;
            repaintMutedSet = false;
            try { player.Muted = repaintMuted; }
            catch (Exception e) { Log("[aspect] source crop: unmute refused: " + Unwrap(e)); }
        }

        // Resumes the film just long enough for the crop to be presented, then stops
        // it. Every call stays on the main thread: the poll posts there and so does
        // the frame event, when the TV has one.
        void StartRepaint() {
            // Nobody takes over the mute, so give it back: CropApply muted before the
            // crop and only hands the responsibility over when a repaint starts.
            if (player == null) { RepaintMute(false); return; }
            // A second press while one is still running: the crop went out on the same
            // call, so the film is already running over the new one. Only the ceiling
            // moves; restarting would lose the position the wait is measured from.
            if (repaintActive) { repaintDeadline = Stopwatch.GetTimestamp() + REPAINT_BUDGET_MS * Stopwatch.Frequency / 1000; return; }
            repaintActive = true;
            repaintSessao = playerSessao;
            repaintStart = Stopwatch.GetTimestamp();
            repaintDeadline = repaintStart + REPAINT_BUDGET_MS * Stopwatch.Frequency / 1000;
            repaintStartPos = -1;
            string st0 = "?", st1 = "?", por = "";
            try { repaintStartPos = player.GetPlayPosition(); } catch (Exception e) { por = Unwrap(e); }
            try { st0 = player.State.ToString(); } catch { }
            // Already muted by CropApply, before the crop. This is the belt to that
            // brace: a crop sent while the player read Playing got no mute there.
            RepaintMute(true);
            // THE EVENT IS A BONUS, NOT THE PLAN. On this TV it does not exist:
            // NotSupportedException, "raw_video is not supported", 37 of 37 presses.
            // Relying on it meant every repaint ran the whole ceiling and played
            // audio for it.
            repaintHandler = (s, e) => Principal(() => { if (repaintActive) FinishRepaint(true, null, true); });
            try { player.VideoFrameDecoded += repaintHandler; }
            catch (Exception e)
            {
                repaintHandler = null;
                // LOG ONCE, NEVER RETURN: returning skipped player.Start() below, so only
                // the first press ever ran the film and every later one timed out.
                if (!repaintEventWarned)
                {
                    repaintEventWarned = true;
                    Log("[aspect] source crop: this TV has no frame event, the position clock is "
                        + "the evidence instead (" + Unwrap(e) + ")");
                }
            }
            try { player.Start(); }
            catch (Exception e) { FinishRepaint(false, e, true); return; }
            try { st1 = player.State.ToString(); } catch { }
            // AGAIN, NOW THAT IT IS RUNNING. The first mute was set while the player was
            // Paused; a player that drops the flag on a state change would lose it here,
            // and this is the only window in which the film is audible.
            if (repaintMutedSet) RepaintMute(true);
            // ONLY WHEN SOMETHING IS WRONG: Start() can leave the player where it was, and
            // that used to be invisible.
            if (st1 != "Playing" || repaintStartPos < 0)
                Log("[aspect] source crop: repaint wanted Playing, got " + st1
                    + " (pos=" + repaintStartPos + "ms)" + (por.Length > 0 ? " read: " + por : ""));
            // SAY THAT THE FILM IS RUNNING. The app tracks playback on its own flag, set
            // only by EV_TOCANDO, and this repaint starts the player behind its back. The
            // app believes the film is still paused otherwise, and its held write is only
            // re-sent while the film runs - so the retry would have no window.
            NvVid.Evento(EV_TOCANDO, 0, 0);
            // ADVANCE NOTHING: stop in the very turn the film was started. No poll, no
            // position check, nothing between Start() and Pause().
            // The frame here is NOT evidence of anything: nothing was waited for, so the
            // log must not claim one was presented. Whether the sink showed the new crop
            // is decided on the TV, and this line is only the timing.
            if (REPAINT_ADVANCE_MS < 0) { FinishRepaint(false, null, true); return; }
            RepaintPoll();
        }

        // Watches the film come back to life. The host's own Tique runs at 250 ms,
        // which is far too coarse to cut a repaint short, so this waits on its own.
        async void RepaintPoll() {
            int minha = repaintSessao;
            while (repaintActive && minha == repaintSessao)
            {
                await System.Threading.Tasks.Task.Delay(REPAINT_POLL_MS);
                if (!repaintActive || minha != repaintSessao) return;
                Principal(() => {
                    if (!repaintActive || minha != repaintSessao) return;
                    if (Stopwatch.GetTimestamp() >= repaintDeadline) { FinishRepaint(false, null, true); return; }
                    int pos = -1;
                    try { if (player != null) pos = player.GetPlayPosition(); } catch { }
                    if (repaintStartPos >= 0 && pos - repaintStartPos >= REPAINT_ADVANCE_MS)
                        FinishRepaint(true, null, true);
                });
            }
        }

        // `pause` says whether the repaint still owns the film. The person can press
        // play while one is in flight, and that request is theirs, not ours.
        void FinishRepaint(bool gotFrame, Exception err, bool pause) {
            if (!repaintActive) return;
            repaintActive = false;
            if (repaintHandler != null && player != null)
            {
                try { player.VideoFrameDecoded -= repaintHandler; } catch { }
                repaintHandler = null;
            }
            if (player != null && repaintSessao == playerSessao)
            {
                if (pause)
                {
                    try
                    {
                        if (player.State == PlayerState.Playing)
                        {
                            player.Pause();
                            // The matching half of the EV_TOCANDO above, so the app's own
                            // pause is not left looking like a resume the person asked for.
                            NvVid.Evento(EV_PAUSADO, 0, 0);
                        }
                        else
                        {
                            // START() IS ASYNCHRONOUS, so the state can still read Paused
                            // when the pause is asked for. Skipping it here would leave the
                            // film RUNNING - the person paused and would come back to a
                            // film playing, which is far worse than a crop that did not
                            // land. It is retried from Tique until it takes.
                            pausePendente = true;
                            pauseAte = Stopwatch.GetTimestamp() + 2 * Stopwatch.Frequency;
                        }
                    }
                    catch (Exception e) { Log("[aspect] source crop: repaint pause refused: " + Unwrap(e)); }
                }
                RepaintMute(false);
            }
            int pos = -1;
            string stf = "?";
            try { if (player != null) { pos = player.GetPlayPosition(); stf = player.State.ToString(); } } catch { }
            long ms = (Stopwatch.GetTimestamp() - repaintStart) * 1000 / Stopwatch.Frequency;
            if (!gotFrame) Log("[aspect] source crop: repaint gave up with state=" + stf + " pos=" + pos
                               + "ms (started at " + repaintStartPos + "ms)");
            Log("[aspect] source crop: paused repaint "
                + (REPAINT_ADVANCE_MS < 0 ? "stopped immediately, no frame waited for (pos " + pos + "ms)"
                   : gotFrame ? "presented after " + (pos - repaintStartPos) + " ms of film"
                              : "gave up waiting for the film to move")
                + (pause ? "" : ", leaving it playing (play was pressed)")
                + " in " + ms + " ms" + (err != null ? " (" + Unwrap(err) + ")" : ""));
        }

        // Drops a repaint without touching the player: the session it belonged to is
        // already going away.
        void RepaintAbort() {
            if (!repaintActive && repaintHandler == null) return;
            repaintActive = false;
            roiInForce = false;
            releaseAtRisk = false;
            // A pause still owed to the PREVIOUS session must not be applied to the next
            // player: it would pause a film the person just started on purpose.
            pausePendente = false;
            RepaintMute(false);
            if (repaintHandler != null && player != null)
            {
                try { player.VideoFrameDecoded -= repaintHandler; } catch { }
            }
            repaintHandler = null;
        }

        // The pause a repaint asked for that the player was not ready to take yet.
        bool pausePendente;
        long pauseAte;

        // Finishes it once the player is really playing. Bounded: if the state never
        // arrives the request is dropped rather than retried for the rest of the film.
        void PausaPendente() {
            if (!pausePendente) return;
            if (player == null || playerSessao != Volatile.Read(ref sessao)) { pausePendente = false; return; }
            try
            {
                if (player.State == PlayerState.Playing)
                {
                    player.Pause();
                    NvVid.Evento(EV_PAUSADO, 0, 0);
                    pausePendente = false;
                    Log("[aspect] source crop: repaint pause was late, applied now");
                    return;
                }
            }
            catch (Exception e) { pausePendente = false; Log("[aspect] source crop: late repaint pause refused: " + Unwrap(e)); return; }
            if (Stopwatch.GetTimestamp() >= pauseAte)
            {
                pausePendente = false;
                Log("[aspect] source crop: repaint pause never became possible, giving up");
            }
        }

        // The poll owns the ceiling; this is only a backstop for a session whose
        // poll loop was left behind by a teardown.
        void RepaintTick() {
            if (repaintActive && Stopwatch.GetTimestamp() >= repaintDeadline) FinishRepaint(false, null, true);
        }


        static string Unwrap(Exception e) {
            var t = e as System.Reflection.TargetInvocationException;
            if (t != null && t.InnerException != null) e = t.InnerException;
            return e.GetType().Name + ": " + e.Message;
        }

        // 0 = audio, 1 = legenda embutida, 2 = atraso da legenda (ms),
        // 3 = velocidade em centesimos (#202).
        void Escolher(int tipo, int idx)
        {
            if (player == null) return;
            if (tipo == 3) { Velocidade(idx); return; }
            // Diagnostics: the player's REAL state at the moment of the write.
            // Tizen.Multimedia track selection also supports Ready and Paused.
            // The native delay is a measured firmware workaround: an early
            // accepted write did not change the demuxed subtitle on the test TV.
            // This is not the web AVPlay state contract.
            string estado = "?";
            try { estado = player.State.ToString(); } catch { }
            Log("select track " + tipo + "/" + idx + " state=" + estado);
            try
            {
                if (tipo == 0) player.AudioTrackInfo.Selected = idx;
                else if (tipo == 1) player.SubtitleTrackInfo.Selected = idx;
                else player.SetSubtitleOffset(idx);
            }
            catch (Exception e) { if (tipo != 2) Log("escolher " + tipo + "/" + idx + ": " + e.Message); }
        }

        // VELOCIDADE (#202). A documentacao diz que SetPlaybackRate lanca
        // InvalidOperationException em streaming e NotAvailableException com
        // audio offload; a resposta vai ao C (EV_VELOCIDADE, b = 1 aceitou), que
        // esconde a linha se a TV recusar. Nao provado em TV.
        void Velocidade(int centesimos)
        {
            float v = Math.Max(25, Math.Min(400, centesimos)) / 100f;
            try
            {
                player.SetPlaybackRate(v);
                Log("velocidade " + v.ToString(System.Globalization.CultureInfo.InvariantCulture) + " ok");
                NvVid.Evento(EV_VELOCIDADE, centesimos, 1);
            }
            catch (Exception e)
            {
                Log("velocidade " + v.ToString(System.Globalization.CultureInfo.InvariantCulture) + ": " + e.GetType().Name + " " + e.Message);
                NvVid.Evento(EV_VELOCIDADE, centesimos, 0);
            }
        }

        // Lista de faixas para o C, logo depois do prepare. Devolve quantas
        // faixas de audio o player listou.
        int Faixas(Player p, bool soAudio = false)
        {
            int selA = -1, selL = -1, nA = 0;
            try
            {
                var a = p.AudioTrackInfo;
                nA = a.GetCount();
                for (int i = 0; i < nA; i++) NvVid.Faixa(0, i, Lingua(() => a.GetLanguageCode(i)));
                try { selA = a.Selected; } catch { }
            }
            catch (Exception e) { Log("faixas de audio: " + e.Message); }
            // #165: legenda listada e audio nao. Se o video tem som, a faixa que
            // toca aparece como unica, em vez de "nenhuma faixa".
            if (nA == 0)
            {
                try
                {
                    var ap = p.StreamInfo.GetAudioProperties();
                    if (ap.Channels > 0) { NvVid.Faixa(0, 0, ""); selA = 0; Log($"audio sem lista do player: {ap.Channels} canais, {ap.SampleRate} Hz"); }
                }
                catch (Exception e) { Log("propriedades de audio: " + e.Message); }
            }
            if (soAudio) { NvVid.FaixasFim(selA, -1); return nA; }
            // #269: SubtitleTrackInfo lanca InvalidOperation fora de Ready/Playing/Paused
            // (player solto ou ainda em Idle). Sem estado valido nao ha lista: segue sem legenda.
            PlayerState estL;
            try { estL = p.State; } catch { estL = PlayerState.Idle; }
            if (estL != PlayerState.Ready && estL != PlayerState.Playing && estL != PlayerState.Paused)
            {
                Log("faixas de legenda: player em " + estL + ", lista pulada");
                NvVid.FaixasFim(selA, -1);
                return nA;
            }
            try
            {
                var l = p.SubtitleTrackInfo;
                int n = l.GetCount();
                for (int i = 0; i < n; i++) NvVid.Faixa(1, i, Lingua(() => l.GetLanguageCode(i)));
                try { selL = l.Selected; } catch { }
            }
            catch (Exception e) { Log("faixas de legenda: " + e.Message); }
            Log($"faixas do player: {nA} audio (sel={selA}), legenda sel={selL}");
            NvVid.FaixasFim(selA, selL);
            return nA;
        }

        static string Lingua(Func<string> f)
        {
            try { return f() ?? ""; } catch { return ""; }
        }

        // App foi para segundo plano: o player pausa (e o C fica sabendo).
        public void PausarPeloSistema() { SoltaPrimer(); Pausar(true); }

        // VOLUME DO TRAILER (#281: S90C, Tizen 9, trailer sem som com o ajuste
        // de som ligado). So o trailer pede volume (trailer.c); o filme nunca
        // pede e segue intocado. O pedido chega logo depois do Abrir, com o
        // Player ainda em Idle/preparando, e era o UNICO momento em que o
        // Volume era escrito. NAO PROVADO que o Tizen 9 descarta o volume
        // escrito antes do prepare, mas nada mais difere do filme (que toca
        // com som). Entao o alvo fica guardado por sessao e e reaplicado depois
        // do prepare, depois do Start e 1,5 s depois, com Muted=false explicito
        // e o valor LIDO DE VOLTA no registro.
        float volAlvo = 1f;
        bool volPedido;

        void PedirVolume(int v)
        {
            volAlvo = Math.Max(0, Math.Min(100, v)) / 100f;
            volPedido = true;
            if (player != null) AplicaVolume(player, "pedido");
        }

        void AplicaVolume(Player p, string quando)
        {
            if (!volPedido || p == null) return;
            string estado = "?", lido = "?", erro = "";
            try { estado = p.State.ToString(); } catch { }
            // Cada escrita no seu try: uma recusa do Muted nao pode pular o Volume.
            try { p.Muted = volAlvo <= 0f; } catch (Exception e) { erro += " muted:" + e.GetType().Name; }
            try { p.Volume = volAlvo; } catch (Exception e) { erro += " volume:" + e.GetType().Name + " " + e.Message; }
            try { lido = p.Volume.ToString("0.00", CultureInfo.InvariantCulture) + " muted=" + p.Muted; }
            catch (Exception e) { lido = "? (" + e.GetType().Name + ")"; }
            if (erro.Length > 0) lido += " falhou" + erro;
            Log("[audio] volume alvo=" + volAlvo.ToString("0.00", CultureInfo.InvariantCulture) +
                " lido=" + lido + " estado=" + estado + " (" + quando + ")");
        }

        // Relogio do host, no fio principal.
        public void Tique()
        {
            // Uma consulta por tique no principal; fPos so le o cache.
            long inicio = Stopwatch.GetTimestamp();
            try { if (player != null && playerSessao == Volatile.Read(ref sessao) && player.State == PlayerState.Playing) posMs = player.GetPlayPosition(); } catch { }
            long ms = (Stopwatch.GetTimestamp() - inicio) * 1000 / Stopwatch.Frequency;
            if (ms >= 50) Log("[video] tpk consulta posicao levou " + ms + " ms (fio principal)");
            RepaintTick();
            PausaPendente();
        }

        public void Log(string s)
        {
            try { NvVid.LogNativo(s); } catch { }
            try { File.AppendAllText(logArq, DateTime.Now.ToString("HH:mm:ss ") + s + "\n"); } catch { }
        }

        void Principal(Action a)
        {
            principal(() => { try { a(); } catch (Exception e) { Log("principal: " + e); } });
        }

        // Invalida tambem comandos que ficaram na fila durante uma troca.
        void ComSessao(Action a)
        {
            int minha = Volatile.Read(ref sessao);
            Principal(() => { if (minha == Volatile.Read(ref sessao) && playerSessao == minha && !paradaFalhou) a(); });
        }

        [DllImport("libc.so.6", EntryPoint = "_exit")] static extern void SairImediatamente(int codigo);
        internal int PrazoPararMs = 5000;
        internal Action FalhaFatal = () => { try { SairImediatamente(1); } catch { Environment.Exit(1); } };
        int fatal;
        bool paradaFalhou;
        readonly object trocaTrava = new object();
        System.Threading.Timer prazoParada;
        Stopwatch tempoParada;

        // O prazo nasce no fio solicitante, ANTES do Post: o principal pode
        // estar preso em GetPlayPosition/Stop. Pedidos repetidos nao adiam o
        // limite. A trava so protege o prazo, nunca uma chamada Tizen.
        void Trocar(Action<int> abrir)
        {
            int minha;
            lock (trocaTrava)
            {
                if (fatal != 0) return;
                minha = Interlocked.Increment(ref sessao);
                if (prazoParada == null)
                {
                    var tempo = tempoParada = Stopwatch.StartNew();
                    prazoParada = new System.Threading.Timer(_ =>
                    {
                        lock (trocaTrava)
                        {
                            if (prazoParada == null || tempoParada != tempo || fatal != 0) return;
                            prazoParada.Dispose(); prazoParada = null;
                            // Fila atrasada/flags nao provam recurso retido. As
                            // referencias so somem quando Dispose retorna.
                            if (Volatile.Read(ref player) == null && Volatile.Read(ref primer) == null) return;
                            Volatile.Write(ref fatal, 1);
                        }
                        Log("[video] tpk parar NAO confirmou em " + tempo.ElapsedMilliseconds + " ms; encerrando processo");
                        FalhaFatal();
                    }, null, PrazoPararMs, Timeout.Infinite);
                }
            }
            Principal(() =>
            {
                if (minha != Volatile.Read(ref sessao) || Volatile.Read(ref fatal) != 0) return;
                if (paradaFalhou || !PararAtual()) { paradaFalhou = true; return; }
                long ms;
                lock (trocaTrava)
                {
                    if (minha != sessao || fatal != 0) return;
                    ms = tempoParada.ElapsedMilliseconds;
                    prazoParada?.Dispose(); prazoParada = null;
                }
                Log("[video] tpk parar confirmado em " + ms + " ms");
                if (abrir != null && minha == Volatile.Read(ref sessao)) abrir(minha);
            });
        }

        async void Abrir(string url, string cabecalhos, int minha)
        {
            posMs = 0;
            // Player novo nasce no volume cheio; so um pedido DESTA sessao
            // (trailer.c, logo depois do video_tocar) o muda.
            volAlvo = 1f; volPedido = false;
            try
            {
                var p = new Player();
                Volatile.Write(ref player, p); playerSessao = minha;
                p.PlaybackCompleted += (s, e) => { if (minha == Volatile.Read(ref sessao)) NvVid.Evento(EV_FIM, 0, 0); };
                p.ErrorOccurred += (s, e) => { if (minha == Volatile.Read(ref sessao)) { Log("erro " + e.Error); NvVid.Evento(EV_ERRO, (int)e.Error, 0); } };
                p.BufferingProgressChanged += (s, e) => { if (minha == Volatile.Read(ref sessao)) NvVid.Evento(EV_BUFFER, e.Percent, 0); };
                p.PlaybackInterrupted += (s, e) => { if (minha == Volatile.Read(ref sessao)) { Log("interrompido: " + e.Reason); NvVid.Evento(EV_PAUSADO, 0, 0); } };
                p.SubtitleUpdated += (s, e) => { if (minha == Volatile.Read(ref sessao)) NvVid.Legenda(e.Text ?? "", (int)e.Duration); };
                foreach (var linha in (cabecalhos ?? "").Split('\n'))
                {
                    int i = linha.IndexOf(':');
                    if (i <= 0) continue;
                    string nome = linha.Substring(0, i).Trim(), valor = linha.Substring(i + 1).Trim();
                    if (nome.Equals("User-Agent", StringComparison.OrdinalIgnoreCase)) p.UserAgent = valor;
                    else if (nome.Equals("Cookie", StringComparison.OrdinalIgnoreCase)) p.Cookie = valor;
                    else Log("cabecalho ignorado pelo player: " + nome);
                }
                p.SetSource(new MediaUriSource(url));
                p.Display = fazDisplay();
                p.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                await p.PrepareAsync();
                // A parada e dona unica do descarte, inclusive se o Prepare
                // terminar depois do pedido de saida ou de outra abertura.
                if (minha != Volatile.Read(ref sessao)) return;
                int dur = 0;
                try { dur = p.StreamInfo.GetDuration(); } catch { }
                try { var v = p.StreamInfo.GetVideoProperties(); NvVid.Evento(EV_TAMANHO, v.Size.Width, v.Size.Height); } catch { }
                int nAudio = Faixas(p);
                AplicaVolume(p, "preparado");
                NvVid.Evento(EV_PRONTO, dur, 0);
                if (minha != Volatile.Read(ref sessao)) return;
                p.Start();
                windowMetrics.Invalidate();
                AplicaVolume(p, "start");
                NvVid.Evento(EV_TOCANDO, 0, 0);
                if (volPedido)
                {
                    await System.Threading.Tasks.Task.Delay(1500);
                    if (minha == Volatile.Read(ref sessao) && player == p) AplicaVolume(p, "tocando 1,5 s");
                }
                // Alguns contêineres/HLS so publicam as faixas de audio depois
                // que a reproducao comeca: le de novo, uma vez.
                if (nAudio == 0)
                {
                    await System.Threading.Tasks.Task.Delay(2000);
                    if (minha == Volatile.Read(ref sessao) && player == p) Faixas(p, true);
                }
            }
            catch (Exception e)
            {
                if (minha != Volatile.Read(ref sessao)) { Log("abrir: sessao antiga encerrada (" + e.GetType().Name + ")"); return; }
                Log("abrir: " + e);
                NvVid.Evento(EV_ERRO, -1, 0);
            }
        }

        // CANARIO (#137, Samsung TV Plus tocando por baixo do Nuvio). O relato:
        // o som do canal so para quando um filme comeca, isto e, quando um
        // Player deste arquivo prepara e toca. A aposta (NAO provada) e que e o
        // gerenciador de recursos da TV que tira o decodificador/saida de audio
        // do TV Plus nesse momento. Entao, logo que a janela sobe, o host 6+
        // toca pelo MESMO caminho do filme (Player + Display da janela NUI) um
        // clipe de 2 s preto e mudo (res/silencio.mp4, H.264 Main + AAC-LC) por
        // ~1 s e solta tudo (Stop/Unprepare/Dispose), como ao fim de um filme.
        // Nada fica preso: o filme de verdade e a saida seguem como hoje. So o
        // host 6+ (Program.cs) chama isto; no 4/5 `primer` e sempre null e os
        // SoltaPrimer() de Parar/PausarPeloSistema nao fazem nada.
        Player primer;
        int primerGen;

        public async void PrimeAudio(string arquivo)
        {
            int minha = ++primerGen;
            Log("[audio] prime begin " + Path.GetFileName(arquivo));
            try
            {
                if (!File.Exists(arquivo)) { Log("[audio] prime fail sem arquivo " + arquivo); return; }
                if (player != null) { Log("[audio] prime skip: player do app ja aberto"); return; }
                var p = new Player();
                Volatile.Write(ref primer, p);
                p.ErrorOccurred += (s, e) => Log("[audio] prime erro do player " + e.Error);
                p.PlaybackInterrupted += (s, e) => Log("[audio] prime interrompido " + e.Reason);
                p.SetSource(new MediaUriSource(arquivo));
                p.Display = fazDisplay();
                p.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                var prep = p.PrepareAsync();
                // Excecao de um prepare abandonado (timeout) nao pode sobrar solta.
                _ = prep.ContinueWith(t => { var _e = t.Exception; }, System.Threading.Tasks.TaskContinuationOptions.OnlyOnFaulted);
                if (await System.Threading.Tasks.Task.WhenAny(prep, System.Threading.Tasks.Task.Delay(8000)) != prep)
                {
                    Log("[audio] prime fail prepare demorou mais de 8 s");
                    return;
                }
                await prep;
                if (minha != primerGen) { Log("[audio] prime cancelado (filme/saida antes de preparar)"); return; }
                Log("[audio] prime prepared");
                p.Start();
                Log("[audio] prime started");
                await System.Threading.Tasks.Task.Delay(1200);
                if (minha != primerGen) { Log("[audio] prime cancelado (filme/saida durante o clipe)"); return; }
                Log("[audio] prime ok");
            }
            catch (Exception e) { Log("[audio] prime fail " + e.GetType().Name + ": " + e.Message); }
            finally { if (minha == primerGen) SoltaPrimer(); }
        }

        bool SoltaPrimer()
        {
            primerGen++;
            if (primer == null) return true;
            if (!Liberar(ref primer)) return false;
            Log("[audio] prime released");
            return true;
        }

        void ReportWindowMetrics()
        {
            if (windowMetrics.Requests > 0)
            {
                double scale = 1000.0 / Stopwatch.Frequency;
                Log(string.Format(CultureInfo.InvariantCulture,
                    "[video-window] requests={0} repeated={1} applied={2} failed={3} native_ms={4:F3} max_ms={5:F3}",
                    windowMetrics.Requests, windowMetrics.RepeatedRequests, windowMetrics.Applied,
                    windowMetrics.Failed, windowMetrics.ElapsedTicks * scale, windowMetrics.MaxTicks * scale));
            }
            windowMetrics.Reset();
        }

        public void Parar() { Trocar(null); }
        public void Encerrar() { Parar(); }

        // Dispose confirma a liberacao, independentemente do estado anterior.
        // Preparing nao admite Unprepare; cada passo ainda tenta o Dispose.
        bool Liberar(ref Player p)
        {
            try { p.Muted = true; } catch (Exception e) { Log("parar muted: " + e.Message); }
            try { p.Display = null; } catch (Exception e) { Log("parar display: " + e.Message); }
            try { if (p.State == PlayerState.Playing || p.State == PlayerState.Paused) p.Stop(); }
            catch (Exception e) { Log("parar stop: " + e.Message); }
            try { if (p.State == PlayerState.Ready || p.State == PlayerState.Playing || p.State == PlayerState.Paused) p.Unprepare(); }
            catch (Exception e) { Log("parar unprepare: " + e.Message); }
            try { p.Dispose(); Volatile.Write(ref p, null); return true; }
            catch (Exception e) { Log("parar dispose: " + e.Message); return false; }
        }

        bool PararAtual()
        {
            ReportWindowMetrics();
            // An in-flight repaint must not outlive the player it belongs to: its
            // handler would stay on a doomed Player and `repaintActive` would stay set,
            // deferring the next session's pause forever.
            RepaintAbort();
            if (!SoltaPrimer()) return false;
            if (player == null) return true;
            return Liberar(ref player); // cache preserva a retomada durante a espera da reconexao
        }

        void Pausar(bool pausa)
        {
            if (player == null) return;
            // THE REPAINT OWNS THE FILM WHILE IT RUNS.
            //
            // A paused crop resumes the film for one frame (see StartRepaint), and the
            // C side sends its own pause right after every crop. Both are queued on
            // this thread, so without this the pause runs immediately after the start
            // and the frame never comes. The repaint pauses on its own when the frame
            // arrives, so this request is already satisfied; letting it through is what
            // made the wait time out.
            if (pausa && repaintActive) { Log("[aspect] source crop: pause deferred, the repaint owns the film"); return; }
            // A PLAY PRESSED WHILE A LATE PAUSE IS STILL OWED IS THE PERSON'S, NOT
            // ours: PausaPendente would pause the film they just started, anywhere in
            // its retry window. The owed pause dies here.
            if (!pausa) pausePendente = false;
            try
            {
                // A play request during a repaint cancels it and keeps the film
                // running: it is the person's, not ours.
                if (!pausa && repaintActive) FinishRepaint(false, null, false);
                if (pausa && player.State == PlayerState.Playing) { player.Pause(); NvVid.Evento(EV_PAUSADO, 0, 0); }
                else if (!pausa && player.State == PlayerState.Paused)
                {
                    player.Start();
                    windowMetrics.Invalidate();
                    NvVid.Evento(EV_TOCANDO, 0, 0);
                }
            }
            catch (InvalidOperationException e)
            {
                // #269: o estado mudou entre a checagem e o Pause/Start. Estado ja e outro; avisa uma vez.
                if (!pausarAvisado) { pausarAvisado = true; Log("pausar: estado mudou (" + e.Message + ")"); }
            }
            catch (Exception e) { Log("pausar: " + e.Message); }
        }
        bool pausarAvisado;

        async void Buscar(int ms)
        {
            if (player == null) return;
            windowMetrics.Invalidate();
            try { posMs = ms; await player.SetPlayPositionAsync(ms, false); }
            catch (Exception e) { Log("buscar: " + e.Message); }
        }

        // Host 4/5 (Program40): move/redimensiona a janela ElmSharp do video.
        // Devolve null se aplicou, ou a mensagem do erro. Fica null no 6+, que
        // continua so com Mode=Roi + SetRoi (comportamento inalterado).
        public Func<int, int, int, int, string> GeometriaJanela;
        bool janelaMovida;

        void Janela(int x, int y, int w, int h)
        {
            if (player == null) return;
            bool applied = false, fullscreen = x == 0 && y == 0 && w == telaW && h == telaH;
            windowMetrics.Begin(player, x, y, w, h, fullscreen);
            long started = Stopwatch.GetTimestamp(), ended = 0;
            try
            {
                // QUADRO CHEIO SEM ZOOM: LetterBox, o mesmo caminho da reproducao
                // normal nos 4 hosts. A reproducao cheia pede exatamente a tela
                // (0,0,telaW,telaH), entao esta guarda continua sendo um no-op
                // para ela — 6+ e o host 4/5 nao mudam.
                //
                // ZOOM (#178): o recorte de fonte emulado (src/video_tpk.c
                // video_janela_fonte) manda um ROI que RECUA a origem para
                // negativo ou ESTOURA a tela, para que a fatia desejada do quadro
                // preencha o destino. Antes, `w >= telaW && h >= telaH` engolia um
                // ROI ampliado ancorado em 0,0 de volta para LetterBox, matando o
                // zoom. Agora so o quadro EXATO da tela vira LetterBox; qualquer
                // ROI de zoom (origem negativa OU maior que a tela) passa cru ao
                // SetRoi.
                if (GeometriaJanela != null) { applied = JanelaTizen45(x, y, w, h, fullscreen); return; }
                if (fullscreen) { player.DisplaySettings.Mode = PlayerDisplayMode.LetterBox; applied = true; return; }
                player.DisplaySettings.Mode = PlayerDisplayMode.Roi;
                player.DisplaySettings.SetRoi(new Rectangle(x, y, w, h));
                applied = true;
            }
            catch (Exception e) { ended = Stopwatch.GetTimestamp(); Log("[video-window] apply failed: " + e.Message); }
            finally { windowMetrics.Complete(applied, (ended != 0 ? ended : Stopwatch.GetTimestamp()) - started); }
        }

        // Tizen 4/5 (#203, botao de aspecto/zoom sem efeito). CAUSA PROVAVEL
        // (nao provada em TV): Mode=Roi + SetRoi sobre um Display de janela
        // ElmSharp e aceito sem excecao mas o plano de video do 4/5 o ignora, e
        // o destino "tela cheia" (Esticar) cai em LetterBox, que nao muda nada.
        // Aqui o retangulo de destino vira a GEOMETRIA da janela do video e o
        // player preenche a janela (FullScreen). Se a janela falhar, tenta o
        // ROI antigo. Cada passo vai ao log para a TV provar qual funcionou.
        bool JanelaTizen45(int x, int y, int w, int h, bool fullscreen)
        {
            string rotulo = x + "," + y + " " + w + "x" + h;
            if (fullscreen)
            {
                string e0 = janelaMovida ? GeometriaJanela(0, 0, telaW, telaH) : null;
                janelaMovida = false;
                player.DisplaySettings.Mode = PlayerDisplayMode.LetterBox;
                Log("[aspect] tpk40 modo=letterbox " + rotulo + (e0 == null ? " ok" : " falhou: " + e0));
                return e0 == null;
            }
            string erro = GeometriaJanela(x, y, w, h);
            if (erro == null)
            {
                try
                {
                    player.DisplaySettings.Mode = PlayerDisplayMode.FullScreen;
                    janelaMovida = true;
                    Log("[aspect] tpk40 modo=janela+fullscreen " + rotulo + " ok");
                    return true;
                }
                catch (Exception e) { erro = "Mode=FullScreen " + e.GetType().Name + ": " + e.Message; }
            }
            Log("[aspect] tpk40 modo=janela " + rotulo + " falhou: " + erro);
            try
            {
                player.DisplaySettings.Mode = PlayerDisplayMode.Roi;
                player.DisplaySettings.SetRoi(new Rectangle(x, y, w, h));
                Log("[aspect] tpk40 modo=roi " + rotulo + " ok");
                return true;
            }
            catch (Exception e) { Log("[aspect] tpk40 modo=roi " + rotulo + " falhou: " + e.GetType().Name + ": " + e.Message); return false; }
        }
    }
}
