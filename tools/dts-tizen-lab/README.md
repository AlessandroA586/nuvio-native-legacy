# DTS/TrueHD → AC3 lab (PC side)

Test tooling for `docs/plans/dts-truehd-ac3-tizen-DESIGN.md`. Nothing here ships in
the app. It does two jobs:

1. **Host tests** that prove, on a PC, everything that does not depend on the
   Samsung player: bit-exact video, HDR10 kept, AC3 5.1 channel map, A/V offset,
   seamless on-demand segments, and the HTTP delivery models.
2. **The TV test kit**: a server that converts a real movie on the fly in every
   candidate delivery model, so the S95C's own player can be tested before any
   feature code exists, plus a runner that drives the S0 probe build.

| File | What it is |
|---|---|
| `build.sh` | Builds FFmpeg 7.1.5 with the **proposed .tpk component set** (host target) and `tvconv` |
| `tvconv.c` | Prototype of the converter core: copies video, converts one DTS/TrueHD/MLP track to AC3, muxes TS/MKV/MP4, `--start/--end` for segments |
| `make_fixtures.sh` | Synthetic HEVC Main10 HDR10 + DTS 5.1 / TrueHD 5.1 / AC3 / beep files |
| `host_tests.py` | The host suite (T1–T7). Exit 0 = all passed |
| `nvlab_server.py` | Serves a movie as original / static converted / progressive / HLS-on-demand |
| `tv_runner.py` | Drives the S0 probe build on the TV through the matrix, writes `tv-report.md` |
| `Dockerfile` | Same thing in a container (Linux or macOS with OrbStack) |

## Host tests

```sh
sh tools/dts-tizen-lab/build.sh                 # ~1-2 min, needs cc/make/git or curl
python3 tools/dts-tizen-lab/host_tests.py       # needs ffmpeg/ffprobe CLI with libx265
```

Or with Docker: `docker build -t nuvio-dts-lab tools/dts-tizen-lab && docker run --rm --entrypoint python3 nuvio-dts-lab host_tests.py`.

## Real files (what the synthetic fixtures cannot cover)

FFmpeg cannot encode DTS-HD MA (XLL), 7.1 DTS/TrueHD or Atmos, so run the
converter on your own remuxes too. Use one of each you have: DTS 5.1, DTS-HD MA 7.1,
DTS:X, TrueHD 7.1, TrueHD Atmos, plus one 4K HDR10 and one HDR10+ title.

```sh
T=tools/dts-tizen-lab/tvconv
$T Movie.mkv x --index | head                      # keyframe index from Cues (HLS needs it)
$T Movie.mkv /tmp/out.ts --fmt mpegts --stats       # prints decoder path and audio CPU % of realtime
$T Movie.mkv /tmp/seg.ts --fmt mpegts --start 3600 --end 3606 --offset 10 --stats
ffprobe -v error -show_streams /tmp/out.ts | grep -E "codec_name|channel_layout|color_transfer"
```

`--stats` shows `source truehd 8 ch -> AC3 6 ch, decoder 5.1 presentation` (TrueHD
substream) or `DTS core only`. Report the line and the `audio-cpu` percentage.

## TV test kit

Start the server on the PC with a real file (the TV must reach the PC):

```sh
python3 tools/dts-tizen-lab/nvlab_server.py /movies/Movie.mkv --port 8000
# or: docker run --rm -p 8000:8000 -v /movies:/media nuvio-dts-lab /media/Movie.mkv
```

It prints the name used in URLs. Every request the TV makes is logged with its
`Range` header and User-Agent; keep that log, it answers how the Samsung player
fetches segments.

| URL | Model |
|---|---|
| `/file/NAME.mkv` | original (control, expected silent) |
| `/static/NAME.{mkv,mp4,ts}` | whole file converted once, Range + Content-Length |
| `/prog/NAME.{ts,mkv,mp4}?t=SECONDS` | progressive conversion, no length (restart-on-seek) |
| `/hls/NAME/index.m3u8` | HLS VOD, each segment generated on request (recommended) |

`--transfer chunked|close|length` changes how generated bodies are framed (default
chunked). If HLS fails with chunked, try `length` before anything else.

Playing these URLs on the TV needs the S0 probe build (retail Samsung TVs refuse
`sdb shell`, so the old `/tmp/nuvio-video` hook cannot be reached from the PC).
With the probe installed:

```sh
python3 tools/dts-tizen-lab/tv_runner.py --tv <TV-IP> --token <TOKEN> --pc <PC-IP>:8000 --name NAME --seek 600
```

It plays each variant, records prepare / first-frame / seek times and the app's
`[audio]`/`[video]` log lines, asks you four questions per variant (sound, Dolby
Digital on the receiver, HDR badge, lip sync) and writes `tv-report.md`.
