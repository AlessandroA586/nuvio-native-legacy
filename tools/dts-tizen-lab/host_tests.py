#!/usr/bin/env python3
"""Host tests for the DTS/TrueHD -> AC3 design (no TV needed).

  sh tools/dts-tizen-lab/build.sh
  python3 tools/dts-tizen-lab/host_tests.py            # fixtures in .build/fx

Proves on the PC what does not depend on the Samsung player:
  T1 video packets leave bit-identical in Matroska, fragmented MP4 and MPEG-TS
  T2 HDR10 signalling (VUI) and in-band mastering/MaxCLL SEI survive
  T3 AC3 5.1 640 kb/s, channel map (tone per channel), no level change
  T4 A/V offset vs the source, from 0 and after seeks: |error| <= 1 ms
  T5 segments generated alone == one continuous run (HLS on demand)
  T6 the lab server: HLS end to end, HLS seek fetches the right segment,
     progressive streams start at the keyframe before ?t=
  T7 audio CPU cost (report only)
Exit status 0 only if every check passed.
"""
import array, json, math, os, subprocess, sys, tempfile, time, urllib.request, socket

HERE = os.path.dirname(os.path.abspath(__file__))
TV = os.path.join(HERE, "tvconv")
FX = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, ".build", "fx")
TMP = tempfile.mkdtemp(prefix="nvlab-")
FAIL = []

def run(*a, **k): return subprocess.check_output(list(a), stderr=subprocess.STDOUT if k.get("err") else None)
def check(name, ok, detail=""):
    print(("PASS " if ok else "FAIL ") + name + (" | " + detail if detail else ""), flush=True)
    if not ok: FAIL.append(name)
def conv(src, out, *args): return subprocess.run([TV, src, out] + list(args), capture_output=True, text=True, check=True).stderr
def probe(f, sel, entries):
    return json.loads(run("ffprobe", "-v", "error", "-select_streams", sel, "-show_entries", entries, "-of", "json", f))
def md5s(f, bsf=None):
    cmd = ["ffmpeg", "-v", "error", "-i", f, "-map", "0:v", "-c", "copy"] + (["-bsf:v", bsf] if bsf else []) + ["-f", "framemd5", "-"]
    return [l.split(",")[5].strip() for l in run(*cmd).decode().splitlines() if not l.startswith("#")]
def ac3_pcm(f, mono=False):
    raw = run("ffmpeg", "-v", "error", "-i", f, "-map", "0:a:0", "-c", "copy", "-f", "ac3", "-")
    p = subprocess.run(["ffmpeg", "-v", "error", "-f", "ac3", "-i", "-"] + (["-ac", "1"] if mono else []) +
                       ["-f", "f32le", "-c:a", "pcm_f32le", "-"], input=raw, capture_output=True, check=True).stdout
    a = array.array("f"); a.frombytes(p); return a
def pkts(f, sel):
    return [(int(p["pts"]), int(p.get("dts", p["pts"]))) for p in probe(f, sel, "packet=pts,dts")["packets"]]

# ---------------------------------------------------------------- fixtures
if not os.path.exists(os.path.join(FX, "src_dts51.mkv")):
    print("generating fixtures in", FX, flush=True)
    subprocess.run(["sh", os.path.join(HERE, "make_fixtures.sh"), FX], check=True, stdout=subprocess.DEVNULL)
SRC = {k: os.path.join(FX, f"src_{k}.mkv") for k in ("dts51", "truehd51", "beep_dts", "beep_thd", "dual")}

# ---------------------------------------------------------------- T1-T3
TONES = [300, 500, 700, 60, 1300, 1500]; NAMES = "FL FR FC LFE SL SR".split()
for k in ("dts51", "truehd51"):
    for fmt, ext in (("matroska", "mkv"), ("mp4", "mp4"), ("mpegts", "ts")):
        out = os.path.join(TMP, f"{k}.{ext}")
        conv(SRC[k], out, "--fmt", fmt, *(["--offset", "10"] if fmt == "mpegts" else []))
        if ext == "ts":   # the TS muxer adds access-unit delimiters (NAL 35): compare without them
            a, b = md5s(SRC[k], "hevc_mp4toannexb,filter_units=remove_types=35"), md5s(out, "filter_units=remove_types=35")
        else:
            a, b = md5s(SRC[k]), md5s(out)
        check(f"T1 {k}->{ext} video bit-identical", a == b, f"{len(b)}/{len(a)} packets")
        st = probe(out, "v:0", "stream=color_transfer,color_primaries,color_space")["streams"][0]
        fr = probe(out, "v:0", "frame_side_data=side_data_type").get("frames", [{}])
        side = {d.get("side_data_type") for f in fr[:1] for d in f.get("side_data_list", [])}
        hdr = st.get("color_transfer") == "smpte2084" and st.get("color_primaries") == "bt2020" \
              and "Mastering display metadata" in side and "Content light level metadata" in side
        check(f"T2 {k}->{ext} HDR10 kept", hdr, f"{st.get('color_transfer')}/{st.get('color_primaries')}, SEI: {sorted(s for s in side if s)}")
        info = probe(out, "a:0", "stream=codec_name,channels,sample_rate,bit_rate")["streams"][0]
        s = ac3_pcm(out); n = len(s) // 6; seg = range(48000 * 5, 48000 * 6)
        def power(c, f):
            re = im = 0.0
            for t in seg[::2]:
                v = s[t * 6 + c]; w = 2 * math.pi * f * t / 48000; re += v * math.cos(w); im += v * math.sin(w)
            return re * re + im * im
        got = [TONES[max(range(6), key=lambda i: power(c, TONES[i]))] for c in range(6)]
        peak = 20 * math.log10(max(abs(v) for v in s[48000 * 6 * 2:]))
        ok = info["codec_name"] == "ac3" and info["channels"] == 6 and int(info["sample_rate"]) == 48000 \
             and int(info.get("bit_rate", 640000)) == 640000 and got == TONES and abs(peak - (-14.0)) < 0.6
        check(f"T3 {k}->{ext} AC3 5.1 map/level", ok, f"{info['codec_name']} {info['channels']}ch "
              + " ".join(f"{NAMES[c]}={got[c]}" for c in range(6)) + f" peak {peak:.1f} dBFS (source -14.0)")

# ---------------------------------------------------------------- T4 sync
def onsets(f, start_offset):
    ap = [p for p, _ in pkts(f, "a:0")]; s = ac3_pcm(f, mono=True); out = []; i = 0
    while i < len(s):
        if abs(s[i]) > 0.05:
            out.append(ap[i // 1536] / 90000.0 - start_offset + (i % 1536) / 48000.0); i += 96000
        else: i += 1
    return out
for k, srcon in (("beep_dts", 0.0107), ("beep_thd", 0.0)):   # FFmpeg's dca encoder itself delays by 10.7 ms
    for st in (0, 7, 13):
        out = os.path.join(TMP, f"{k}_{st}.ts")
        conv(SRC[k], out, "--fmt", "mpegts", "--offset", "10", "--start", str(st))
        on = [x for x in onsets(out, 10.0) if x > st + 0.1]   # a beep cut by the start point is not an onset
        exp = [b + srcon for b in (10.0, 20.0) if b > st]
        err = [abs(a - b) for a, b in zip(on, exp)]
        check(f"T4 {k} start={st}s A/V offset", len(err) == len(exp) and max(err) <= 0.001,
              "beeps at " + ", ".join(f"{x:.4f}" for x in on) + " s, expected " + ", ".join(f"{x:.4f}" for x in exp))

# ---------------------------------------------------------------- T5 segments
idx = run(TV, SRC["dts51"], "x", "--index").decode()
kf = [float(l.split()[1]) for l in idx.splitlines() if l.startswith("kf ")]
check("T5 keyframe index from Cues", len(kf) >= 10, f"{len(kf)} keyframes")
bounds = kf[::3]
for k in ("dts51", "truehd51", "beep_thd"):
    full = os.path.join(TMP, f"{k}_full.ts"); cat = os.path.join(TMP, f"{k}_cat.ts")
    conv(SRC[k], full, "--fmt", "mpegts", "--offset", "10")
    with open(cat, "wb") as w:
        for i, s0 in enumerate(bounds):
            seg = os.path.join(TMP, f"{k}_seg{i}.ts"); a = ["--fmt", "mpegts", "--offset", "10"]
            if s0 > 0: a += ["--start", str(s0)]
            if i + 1 < len(bounds): a += ["--end", str(bounds[i + 1])]
            conv(SRC[k], seg, *a); w.write(open(seg, "rb").read())
    vf, vc, af, ac = pkts(full, "v:0"), pkts(cat, "v:0"), pkts(full, "a:0"), pkts(cat, "a:0")
    x, y = ac3_pcm(full), ac3_pcm(cat)
    same = md5s(full) == md5s(cat) and vf == vc and af == ac and x == y
    check(f"T5 {k} {len(bounds)} segments == continuous", same,
          f"video {len(vc)}/{len(vf)}, audio {len(ac)}/{len(af)}, decoded PCM identical: {x == y}, "
          f"dts monotonic: {all(b[1] > a[1] for a, b in zip(vc, vc[1:]))}")

# ---------------------------------------------------------------- T6 server
port = (lambda s: (s.bind(("127.0.0.1", 0)), s.getsockname()[1], s.close())[1])(socket.socket())
srv = subprocess.Popen([sys.executable, os.path.join(HERE, "nvlab_server.py"), SRC["dts51"], "--port", str(port), "--seg", "6"],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
try:
    for _ in range(50):
        try: urllib.request.urlopen(f"http://127.0.0.1:{port}/hls/src_dts51/index.m3u8", timeout=1); break
        except Exception: time.sleep(0.2)
    B = f"http://127.0.0.1:{port}"
    dl = os.path.join(TMP, "hls.ts")
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", f"{B}/hls/src_dts51/index.m3u8", "-map", "0", "-c", "copy", dl], check=True)
    ref = os.path.join(TMP, "dts51_full.ts")
    check("T6 HLS over HTTP == continuous conversion", md5s(dl) == md5s(ref) and ac3_pcm(dl) == ac3_pcm(ref))
    hs = os.path.join(TMP, "hlsseek.ts")
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-ss", "25", "-i", f"{B}/hls/src_dts51/index.m3u8", "-frames:v", "30", "-map", "0",
                    "-c", "copy", hs], check=True)
    time.sleep(0.3)
    srv.terminate(); log = srv.communicate(timeout=5)[0]
    segs = [l.split()[2].rsplit("/", 1)[-1] for l in log.splitlines() if " GET /hls" in l and ".ts" in l]
    after = segs[len(bounds):]
    check("T6 HLS seek to 25 s jumps to seg4 without generating seg2/seg3", "seg4.ts" in after and "seg2.ts" not in after
          and "seg3.ts" not in after, "requests (FFmpeg probes the first segments when it opens): " + ", ".join(after))
    srv = subprocess.Popen([sys.executable, os.path.join(HERE, "nvlab_server.py"), SRC["dts51"], "--port", str(port)],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.5)
    for ext in ("ts", "mkv", "mp4"):
        st = probe(f"{B}/prog/src_dts51.{ext}?t=13", "v:0", "packet=pts_time")["packets"][:12]
        first = min(float(p["pts_time"]) for p in st) - (10 if ext == "ts" else 0)
        check(f"T6 progressive {ext} ?t=13 starts at keyframe 12.012 with absolute time", abs(first - 12.012) < 0.2,
              f"first video pts {first:.3f} (leading pictures may sit just before the keyframe)")
finally:
    srv.kill()

# ---------------------------------------------------------------- T7 CPU (report)
for k in ("dts51", "truehd51"):
    e = conv(SRC[k], os.devnull, "--fmt", "mpegts", "--stats")
    line = [l for l in e.splitlines() if "audio-cpu" in l][0]
    print("INFO T7 " + k + ": " + line.split("| media=")[1], flush=True)

print("\n%d failure(s)%s" % (len(FAIL), (": " + ", ".join(FAIL)) if FAIL else ""))
sys.exit(1 if FAIL else 0)
