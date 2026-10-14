#!/usr/bin/env python3
"""DTS/TrueHD -> AC3 delivery lab server (PC side, no app code needed).

Serves every candidate delivery model for a real movie file, converting on the
fly with ./tvconv, so the S95C's own player can be tested through the app's
video request file (/tmp/nuvio-video) before any feature code exists.

  python3 nvlab_server.py MOVIE.mkv [--port 8000] [--seg 6] [--tvconv ./tvconv]

URLs (NAME is the file stem, printed at start):
  /file/NAME.mkv                     the original file (control: plays silent on the S95C)
  /static/NAME.(mkv|mp4|ts)          whole file converted once (cached), served with Range + Content-Length
  /prog/NAME.ts?t=SECONDS            progressive MPEG-TS, video copied + AC3, no length
  /prog/NAME.mkv?t=SECONDS           progressive Matroska, same content
  /prog/NAME.mp4?t=SECONDS           progressive fragmented MP4, same content
  /hls/NAME/index.m3u8               HLS VOD, every segment generated on demand (TS)
  /hls/NAME/segN.ts                  one segment (keyframe-aligned, ~--seg seconds)
Options: --audio N picks the source stream index; --no-downmix decodes all channels.
"""
import argparse, os, subprocess, sys, threading, time, re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

ap = argparse.ArgumentParser()
ap.add_argument("movie"); ap.add_argument("--port", type=int, default=8000)
ap.add_argument("--seg", type=float, default=6.0); ap.add_argument("--tvconv", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "tvconv"))
ap.add_argument("--audio", type=int, default=-1); ap.add_argument("--no-downmix", action="store_true")
ap.add_argument("--offset", type=float, default=10.0, help="fixed timestamp offset for TS/HLS (s)")
ap.add_argument("--transfer", choices=["chunked", "close", "length"], default="chunked",
                help="how generated bodies are framed: chunked (HTTP/1.1), close (no length, close at end), "
                     "length (whole body generated first, then sent with Content-Length)")
A = ap.parse_args()
NAME = re.sub(r"[^A-Za-z0-9_.-]", "_", os.path.splitext(os.path.basename(A.movie))[0])
EXTRA = (["--audio", str(A.audio)] if A.audio >= 0 else []) + (["--no-downmix"] if A.no_downmix else [])

def keyframes():
    out = subprocess.check_output([A.tvconv, A.movie, "x", "--index"] + EXTRA, text=True)
    dur = float(re.search(r"duration=([\d.]+)", out).group(1))
    kf = [float(l.split()[1]) for l in out.splitlines() if l.startswith("kf ")]
    return dur, kf
DUR, KF = keyframes()
if len(KF) < 2:
    sys.exit("No keyframe index (no Cues?): HLS mode needs it; progressive URLs still work. Re-mux with mkvmerge to add Cues.")
BOUND = [KF[0]]
for k in KF[1:]:
    if k - BOUND[-1] >= A.seg: BOUND.append(k)
SEGS = [(BOUND[i], BOUND[i + 1] if i + 1 < len(BOUND) else None) for i in range(len(BOUND))]
LOCK = threading.Lock()
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".build", "cache"); os.makedirs(CACHE, exist_ok=True)
LOG = []

def log(msg):
    line = time.strftime("%H:%M:%S ") + msg
    print(line, flush=True); LOG.append(line)

class H(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def log_message(self, *a): pass
    def stream(self, args, ctype, what):
        t0 = time.time()
        p = subprocess.Popen([A.tvconv, A.movie, "-"] + args + EXTRA, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        first, sent, mode = None, 0, A.transfer
        def head(length=None):
            self.send_response(200); self.send_header("Content-Type", ctype); self.send_header("Cache-Control", "no-store")
            if length is not None: self.send_header("Content-Length", str(length))
            elif mode == "chunked": self.send_header("Transfer-Encoding", "chunked")
            else: self.send_header("Connection", "close")
            self.end_headers()
        try:
            if mode == "length":
                body = p.stdout.read(); first = time.time() - t0; head(len(body)); self.wfile.write(body); sent = len(body)
            else:
                head()
                while True:
                    b = p.stdout.read(65536)
                    if not b: break
                    if first is None: first = time.time() - t0
                    if mode == "chunked": self.wfile.write(b"%x\r\n" % len(b) + b + b"\r\n")
                    else: self.wfile.write(b)
                    sent += len(b)
                if mode == "chunked": self.wfile.write(b"0\r\n\r\n")
        except (BrokenPipeError, ConnectionResetError):
            log(f"{what}: client closed after {sent >> 20} MiB"); p.kill()
        p.wait()
        err = p.stderr.read().decode(errors="replace").strip()
        log(f"{what}: first byte {first if first is None else round(first, 3)} s, {sent / 1048576:.1f} MiB in {time.time() - t0:.2f} s ({mode})"
            + (f" | {err}" if err and p.returncode else ""))
        if mode == "close": self.close_connection = True
    def do_HEAD(self): self.do_GET(head=True)
    def do_GET(self, head=False):
        u = urlparse(self.path); q = parse_qs(u.query); path = u.path
        rng = self.headers.get("Range")
        log(f"GET {self.path}" + (f" Range: {rng}" if rng else "") + f" UA: {self.headers.get('User-Agent', '-')[:60]}")
        if path == f"/file/{NAME}.mkv":
            return self.file(head, A.movie, "video/x-matroska")
        m = re.fullmatch(rf"/static/{re.escape(NAME)}\.(ts|mkv|mp4)", path)
        if m:
            ext = m.group(1); fmt = {"ts": "mpegts", "mkv": "matroska", "mp4": "mp4"}[ext]
            cache = os.path.join(CACHE, f"{NAME}.{ext}")
            with LOCK:
                if not os.path.exists(cache):
                    t0 = time.time()
                    subprocess.run([A.tvconv, A.movie, cache + ".part", "--fmt", fmt] + (["--offset", str(A.offset)] if ext == "ts" else []) + EXTRA, check=True)
                    os.replace(cache + ".part", cache); log(f"static {ext}: converted in {time.time() - t0:.1f} s")
            return self.file(head, cache, {"ts": "video/mp2t", "mkv": "video/x-matroska", "mp4": "video/mp4"}[ext])
        m = re.fullmatch(rf"/prog/{re.escape(NAME)}\.(ts|mkv|mp4)", path)
        if m:
            t = float(q.get("t", ["0"])[0]); fmt = {"ts": "mpegts", "mkv": "matroska", "mp4": "mp4"}[m.group(1)]
            ctype = {"ts": "video/mp2t", "mkv": "video/x-matroska", "mp4": "video/mp4"}[m.group(1)]
            if head: return self.head_only(ctype)
            args = ["--fmt", fmt, "--start", str(t)] + (["--offset", str(A.offset)] if fmt == "mpegts" else [])
            return self.stream(args, ctype, f"prog {m.group(1)} t={t}")
        if path == f"/hls/{NAME}/index.m3u8":
            body = ["#EXTM3U", "#EXT-X-VERSION:3", "#EXT-X-PLAYLIST-TYPE:VOD", "#EXT-X-INDEPENDENT-SEGMENTS",
                    f"#EXT-X-TARGETDURATION:{int(max((e or DUR) - s for s, e in SEGS) + 1)}", "#EXT-X-MEDIA-SEQUENCE:0"]
            for i, (s, e) in enumerate(SEGS):
                body += [f"#EXTINF:{(e if e is not None else DUR) - s:.6f},", f"seg{i}.ts"]
            body.append("#EXT-X-ENDLIST")
            data = ("\n".join(body) + "\n").encode()
            self.send_response(200); self.send_header("Content-Type", "application/vnd.apple.mpegurl")
            self.send_header("Content-Length", str(len(data))); self.end_headers()
            if not head: self.wfile.write(data)
            return
        m = re.fullmatch(rf"/hls/{re.escape(NAME)}/seg(\d+)\.ts", path)
        if m and int(m.group(1)) < len(SEGS):
            s, e = SEGS[int(m.group(1))]
            if head: return self.head_only("video/mp2t")
            args = ["--fmt", "mpegts", "--offset", str(A.offset)] + (["--start", str(s)] if s > 0 else []) + (["--end", str(e)] if e else [])
            return self.stream(args, "video/mp2t", f"hls seg{m.group(1)} [{s:.3f},{e if e else DUR:.3f})")
        self.send_response(404); self.send_header("Content-Length", "0"); self.end_headers()
    def head_only(self, ctype):
        self.send_response(200); self.send_header("Content-Type", ctype); self.send_header("Connection", "close"); self.end_headers()
        self.close_connection = True
    def file(self, head, fpath, ctype):
        size = os.path.getsize(fpath); start, end = 0, size - 1
        r = self.headers.get("Range")
        if r and r.startswith("bytes="):
            a, b = r[6:].split(",")[0].split("-"); start = int(a) if a else size - int(b); end = int(b) if (a and b) else size - 1
            self.send_response(206); self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
        else: self.send_response(200)
        self.send_header("Content-Type", ctype); self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Length", str(end - start + 1)); self.end_headers()
        if head: return
        with open(fpath, "rb") as f:
            f.seek(start); left = end - start + 1
            try:
                while left > 0:
                    b = f.read(min(1 << 20, left))
                    if not b: break
                    self.wfile.write(b); left -= len(b)
            except (BrokenPipeError, ConnectionResetError): pass

if __name__ == "__main__":
    print(f"{A.movie}: {DUR:.1f} s, {len(KF)} keyframes in the index, {len(SEGS)} HLS segments of ~{A.seg:g} s")
    print(f"Name in URLs: {NAME}   (port {A.port})")
    ThreadingHTTPServer(("0.0.0.0", A.port), H).serve_forever()
