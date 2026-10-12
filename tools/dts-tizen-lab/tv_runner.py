#!/usr/bin/env python3
"""Drives the S0 probe build on the TV through the test matrix and writes a report.

The probe build (Session 0, see docs/plans/dts-truehd-ac3-tizen-DESIGN.md) adds a
LAN control port to the app:  http://TV:8099/TOKEN/{play?u=URL, stop, seek?s=S, state, log?n=N}
This script runs every delivery variant served by nvlab_server.py on this PC and
records what the app reports. What only a person can judge (sound from the
receiver, Dolby Digital light, HDR badge, lip sync) is asked on the terminal.

  python3 tv_runner.py --tv 192.168.1.50 --token 123456 --pc 192.168.1.20:8000 --name Movie \
      [--only hls,prog-ts] [--seek 600] [--no-ask] [--out report.md]

Variants (as served by nvlab_server.py for --name):
  file      original file (control: DTS/TrueHD, expected silent)
  static-*  whole file converted once, served with Content-Length + Range
  prog-*    progressive conversion, no length (restart-on-seek model)
  hls       HLS VOD with on-demand TS segments (recommended model)
  loop-*    the same through the app's 127.0.0.1 relay (proves loopback)
"""
import argparse, json, sys, time, urllib.parse, urllib.request

ap = argparse.ArgumentParser()
ap.add_argument("--tv", required=True); ap.add_argument("--token", required=True)
ap.add_argument("--pc", required=True, help="host:port of nvlab_server.py as the TV sees it")
ap.add_argument("--name", required=True); ap.add_argument("--port", type=int, default=8099)
ap.add_argument("--only", default=""); ap.add_argument("--seek", type=float, default=0.0)
ap.add_argument("--no-ask", action="store_true"); ap.add_argument("--out", default="tv-report.md")
ap.add_argument("--wait", type=float, default=25.0, help="seconds to wait for the first frame")
A = ap.parse_args()
P = f"http://{A.pc}"; N = A.name
VARIANTS = [
    ("file", f"{P}/file/{N}.mkv"),
    ("static-mkv", f"{P}/static/{N}.mkv"), ("static-mp4", f"{P}/static/{N}.mp4"), ("static-ts", f"{P}/static/{N}.ts"),
    ("prog-ts", f"{P}/prog/{N}.ts?t=0"), ("prog-mkv", f"{P}/prog/{N}.mkv?t=0"), ("prog-mp4", f"{P}/prog/{N}.mp4?t=0"),
    ("hls", f"{P}/hls/{N}/index.m3u8"),
    ("loop-hls", f"loop:{P}/hls/{N}/index.m3u8"), ("loop-prog-ts", f"loop:{P}/prog/{N}.ts?t=0"),
]
if A.only: VARIANTS = [v for v in VARIANTS if v[0] in A.only.split(",")]
BASE = f"http://{A.tv}:{A.port}/{A.token}"

def call(path, timeout=10):
    with urllib.request.urlopen(BASE + path, timeout=timeout) as r: return r.read().decode(errors="replace")
def state(): return json.loads(call("/state"))
def ask(q):
    if A.no_ask: return "-"
    try: return input(f"    {q} [y/n/text]: ").strip() or "-"
    except EOFError: return "-"

rows = []
for name, url in VARIANTS:
    print(f"\n== {name}: {url}", flush=True)
    call("/stop"); time.sleep(2)
    call("/log?n=0")                         # marks the log position
    t0 = time.time(); call("/play?u=" + urllib.parse.quote(url, safe=""))
    ready = frame = None; st = {}
    while time.time() - t0 < A.wait:
        st = state()
        if st.get("failed"): break
        if ready is None and st.get("ready"): ready = time.time() - t0
        if st.get("pos", 0) >= 0.25: frame = time.time() - t0; break
        time.sleep(0.25)
    p1 = st.get("pos", 0); time.sleep(5); st2 = state(); adv = st2.get("pos", 0) - p1
    seek_t = None; seek_pos = None
    if A.seek and frame:
        ts = time.time(); call(f"/seek?s={A.seek}")
        while time.time() - ts < A.wait:
            s = state()
            if abs(s.get("pos", 0) - A.seek) < 15 and s.get("playing"): seek_t = time.time() - ts; seek_pos = s.get("pos"); break
            time.sleep(0.25)
    log = call("/log?n=400")
    keep = [l for l in log.splitlines() if any(k in l for k in ("[audio]", "faixas do player", "erro ", "[video] tpk", "NotSupported", "[dts", "[sonda]", "interrompido"))]
    print("\n".join("    " + l for l in keep[-15:]))
    human = {}
    if frame:
        human["sound"] = ask("Sound from the speakers/receiver?")
        human["dd"] = ask("Receiver/TV shows Dolby Digital (not PCM)?")
        human["hdr"] = ask("TV shows the HDR badge (if the source is HDR10)?")
        human["sync"] = ask("Lip sync / clap test OK?")
    rows.append(dict(name=name, ready=ready, frame=frame, advanced=round(adv, 2), dur=st2.get("dur"), naudio=st2.get("naudio"),
                     failed=st2.get("failed"), seek=seek_t, seek_pos=seek_pos, log=keep[-6:], **human))
call("/stop")

with open(A.out, "w") as f:
    f.write(f"# S95C delivery test: {N}\n\nTV {A.tv}, PC {A.pc}, {time.strftime('%Y-%m-%d %H:%M')}\n\n")
    f.write("| variant | prepared s | first frame s | advanced in 5 s | duration | audio tracks | failed | seek s | sound | DD | HDR | sync |\n")
    f.write("|---|---|---|---|---|---|---|---|---|---|---|---|\n")
    fmt = lambda v: "-" if v is None else (f"{v:.1f}" if isinstance(v, float) else str(v))
    for r in rows:
        f.write("| " + " | ".join(fmt(r.get(k)) for k in ("name", "ready", "frame", "advanced", "dur", "naudio", "failed", "seek",
                                                          "sound", "dd", "hdr", "sync")) + " |\n")
    f.write("\n## Log lines per variant\n")
    for r in rows: f.write(f"\n### {r['name']}\n```\n" + "\n".join(r["log"]) + "\n```\n")
print(f"\nreport: {A.out}")
