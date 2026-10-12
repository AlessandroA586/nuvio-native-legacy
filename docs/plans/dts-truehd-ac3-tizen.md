# Plan: DTS and TrueHD → AC3 5.1 on the fly, Samsung NuvioTpk (Tizen 8–9)

Scope: **only the full `NuvioTpk` package** (S95C, Tizen 9). Not 40/60/65/WGT.
Goal: when the chosen audio track is DTS (any variant), DTS-HD MA or TrueHD/MLP, the TV
plays it as **Dolby Digital 5.1 (AC3, 640 kbps)**, converted live in the app, with the
video untouched (HDR / Dolby Vision preserved). Native tracks (AC3/E-AC3/AAC) are never touched.

Work is split into **6 sessions**. Each session ends with a pushed branch, passing host tests
and a short hand-off note appended to this file, so the next session can start cold.

## What already exists (read before starting)

| Piece | Where | State |
|---|---|---|
| Converter engine (FFmpeg demux, DTS decode, AAC/AC3 encode, ranged HTTP reader with 4 prefetch workers, cancel, seek) | `src/dts/dts_engine.c`, `dts_engine.h`, `dts_media.h` | Platform-neutral C. Already has `dts_engine_set_ac3(on)` → AC3 5.1 @ 640 kbps, else stereo AAC 192 kbps |
| Playback loop (prepare, feed with backpressure, seek rebuild, stall recovery, subtitles) | `src/dts/dts_playback.c` | Calls `dts_pipeline_*`; webOS-flavoured but mostly generic |
| Output sink | `src/dts/dts_pipeline.c` + `adapter/starfish.cpp` | **LG-only** (Starfish). This is what must be replaced on Tizen |
| The setting | `AJ_DTS_AC3` in `src/ajustes.c` ("Converter DTS para a TV ouvir": Stereo AAC / 5.1 Dolby Digital), applied in `src/video.c:2832` | `dtsConversaoExiste()` returns 0 for `NV_TPK`, so the row is greyed out "Não disponível nesta TV" |
| Detection of tracks the TV can't play | `src/video_tpk.c`: `video_tpk_codec_recusado()` (A_DTS*, A_TRUEHD, A_MLP), `avaliarAudioTv()`, `[audio] TV recusou…` log | Detects and warns only; plays silent today (#313) |
| Tizen player host | `tizen-tpk/Video.cs`, `Program.cs` (Tizen.Multimedia.Player on the TV video plane) | Plays a URI; no packet-level feed |
| FFmpeg build | `tools/build-dts-ffmpeg.sh` (7.1.5, static LGPL, `dca` decoder + `aac`,`ac3` encoders, matroska/mov demuxers only) | Built for webOS ARM; **not** built for `tpk.sh` |
| Loopback HTTP server template | `src/proxyts.c` (127.0.0.1, free port, one session) | Not compiled on `.tpk` (`proxyts_disponivel`) |
| Docs | `docs/features/dts/README.md`, `tools/dts-pipeline/README.md` | LG behaviour, limits, license notes |

## Key design decision (settled in Session 0)

How do converted audio + original video reach the Samsung player?

* **Option A (recommended default): loopback remux.** libnuvio runs a one-session HTTP server on
  127.0.0.1 (copy `proxyts.c`). FFmpeg demuxes the source, **copies video packets unchanged**,
  converts the selected audio track to AC3 5.1, muxes to **fragmented MP4 (or MKV/TS if S0 shows
  Samsung prefers it)** and streams it. The existing `Tizen.Multimedia.Player` plays
  `http://127.0.0.1:P/…` exactly like any URL. Pros: HDR10/Dolby Vision, subtitles hooks, volume,
  pause, audio-out routing all stay on the proven path; no new Tizen API. Cons: no native seek on a
  live-generated stream → a seek restarts the remux at the target time (`?t=`) and the app adds the
  offset (same idea as LG "seeks recreate the pipeline").
* **Option B (fallback): packet feed through `Tizen.Multimedia.ESPlayer`.** Mirrors the LG design
  (`dts_pipeline_*` implemented over P/Invoke callbacks into the .NET host). Pros: real seek,
  closest to existing `dts_playback.c`. Cons: API availability/privileges on Tizen 9 unverified,
  HDR/DV signalling and subtitles must be redone, bigger blast radius.

Session 0 produces a written decision. Sessions 3–5 below are written for Option A; the
B variant is listed under Session 3.

## Fixed behaviour rules (all sessions)

1. Trigger: selected audio codec ∈ {DTS, DTS-HD HRA/MA, DTS Express, TrueHD, MLP} **and** the TV
   did not list it. Reuse `video_tpk_codec_recusado()`. If the file also has a playable AC3/E-AC3/AAC
   track in the preferred language, keep native playback and do not convert.
2. Output is always AC3 5.1 on this build (stereo sources → AC3 2.0 at 192 kbps; mono → 1.0).
   7.1 and Atmos/DTS:X are **downmixed to 5.1 and lose objects**; say so in the setting help text.
3. Setting: reuse `AJ_DTS_AC3` (do not add a new enum entry). On `NV_TPK`: row enabled; values stay
   "Stereo (AAC)" / "5.1 (Dolby Digital)"; **default on this build = 5.1**. Positional-array rules in
   `ajustes.c` still apply (no enum changes needed; if a value must be added it goes at the END).
4. Fail safe: any converter failure falls back to the current behaviour (play, log `[dts]` reason,
   show the existing "TV recusou o áudio" notice). Never crash or hang the player.
5. Every user-visible string goes through `i18n()` with Portuguese keys, **all 28 tables updated**
   (`python3 tools/idiomas.py` must pass).
6. Logs: `[dts-tpk]` prefix, no URLs/keys (the new "Exportar registro" row scrubs but don't rely on it).
7. Add tests next to the existing ones in `tests/` (`bash tests/<name>.sh`).

## Session 0: feasibility on the S95C (no feature code, ~1 session)

Deliverable: `docs/plans/dts-truehd-ac3-tizen-S0.md` with measured answers.
1. Prove the Player accepts AC3 5.1: build a throwaway test hook (or use the "tizen-tpk-spike"
   harness) that plays a small MKV/MP4 with AC3 5.1 and E-AC3 and HEVC HDR10 + DV P8 video.
   Record codec/channel info from `[audio]` logs and what the TV/receiver shows.
2. Prove the Player plays from **loopback HTTP**: the app serves the file from 127.0.0.1 with and
   without `Content-Length`, with Range, and as chunked/live fMP4 and TS. Which containers start
   quickly, keep HDR/DV, and keep A/V sync? Does `Player` seek work on Range-capable loopback?
3. ESPlayer probe: does `Tizen.Multimedia.ESPlayer` exist in the `net8.0-tizen8.0` binding and
   open on the S95C (needed privileges, tizen-manifest changes, HEVC/AC3 stream info support)?
4. Capture what the Player does today with a DTS-only and a TrueHD-only MKV (the unproven part of
   #313): exact error text, whether it lists an empty audio list.
5. Use **Settings › About › Export logs** (branch `feat/export-logs-qr`) or `sdb pull` to bring logs back.
Exit: pick A or B; list container + muxer flags that worked.

## Session 1: FFmpeg for the .tpk build (~1 session)

Files: `tools/build-dts-ffmpeg.sh`, `tools/tpk.sh`, Dockerfile/SDK image used by tpk.sh, `NOTICE`/licenses.
* Add a Tizen target to the script (same pinned 7.1.5 + SHA, LGPL only, `--disable-everything`):
  decoders `dca,truehd,mlp,ac3,eac3`; encoder `ac3`; parsers `dca,mlp,ac3,eac3,hevc,h264,aac`;
  demuxers `matroska,mov`; muxers `mp4` (+`matroska`/`mpegts` if S0 needs); bsfs `dca_core,
  hevc_mp4toannexb,h264_mp4toannexb,eac3_core` as needed; swresample (7.1→5.1 downmix).
  Keep the webOS build output identical (guard with a variable; webOS fixtures must still pass).
* Cross-compile with the same arm-linux-gnueabi toolchain/sysroot as `libnuvio.so`; link statically
  into `libnuvio.so` with `--exclude-libs,ALL` (already used). Check final size and no TLS/segment
  issues (`readelf -lW`, the TLS check already in tpk.sh).
* Compliance: LGPL static link means the project must ship relink material. Either keep the fork
  private/personal or add `licenses/dts/` notes as the LG build does; document the choice.
* Host build + fixtures: `CC=gcc NUVIO_DTS_HOST=1 sh tools/build-dts-ffmpeg.sh`.
Exit: `NV_TPK_PACOTES=NuvioTpk bash tools/tpk.sh` links with FFmpeg and still starts on the TV.

## Session 2: Engine: TrueHD/MLP + DTS-HD → AC3 5.1 (~1 session)

Files: `src/dts/dts_engine.c/.h`, `tests/dts_engine.*`.
* Decode TrueHD/MLP (and keep DTS/DTS-HD); select by codec id, not only DTS.
* Downmix to 5.1 via swresample (explicit matrix, headroom to avoid clipping; keep LFE; 7.1 side/back
  → surround). Resample to 48 kHz. Encode AC3 (`ac3` encoder: 5.1 @ 640 kbps, 2.0 @ 192 kbps).
  Remove the "AAC unless asked" gating: add `dts_engine_set_target()` (AAC stereo | AC3 5.1) and keep
  `dts_engine_set_ac3()` as a wrapper so webOS callers don't change.
* Track list (`DtsMediaInfo.tracks`) must report TrueHD/MLP/DTS variants with channels and language so
  the audio sheet can show them (see Session 4).
* Fixtures: generate small DTS, DTS-HD (core+XLL if encoder available) and TrueHD 7.1 test clips with the
  FFmpeg CLI (`truehd` encoder is experimental: use `-strict -2`); assert output codec = ac3, 6 channels,
  duration within 20 ms, no clipping (peak < 0 dBFS), correct channel mapping via tone-per-channel clip.
Exit: `bash tests/dts_engine.sh` (extended) passes on host, webOS tests unchanged.

## Session 3: Tizen output path (~1–2 sessions)

**Option A (remux):** new `src/dts/dts_remux_tpk.c/.h` (+ `tpk` guard):
* Loopback server cloned from `proxyts.c`: one session, token in the path, 127.0.0.1 only, Host
  check, cancel on new session / close.
* Pipeline: engine → video packets copied (HEVC/H.264 with correct extradata, DV RPU kept) + AC3
  packets → muxer writing to the socket. Interleave by DTS, bounded queue, ~6 s lead like
  `dts_playback.c`. Subtitles are not muxed: stay on the app's existing subtitle path.
* API: `dts_remux_open(src_url, headers, audio_stream, start_seconds, out_url, n)`, `dts_remux_close()`.
  Seek = close + open with new `start_seconds`; the returned URL carries `?t=`.
* Host test with `curl` + `ffprobe` on the stream (video bit-exact vs source, audio = ac3 6ch).
**Option B (ESPlayer):** implement `dts_pipeline_*` over callbacks registered from `tizen-tpk/Video.cs`
  (same pattern as `nv_tpk_video_registrar`), keeping `dts_playback.c` as is; HDR/DV metadata and
  subtitle path to be re-verified on the TV.
Exit: a command-line harness plays a DTS test file through the loopback and verifies the stream.

## Session 4: Wire into the player and Settings (~1 session)

Files: `src/video_tpk.c`, `src/video.c`, `tizen-tpk/Video.cs`, `src/ajustes.c` (+ `ajustes_ux_*.inc`).
* When `avaliarAudioTv()` finds only refused codecs (or the user picks a DTS/TrueHD track in the
  audio sheet), start the remux and hand its URL to the host player; keep position/offset bookkeeping
  for seeks (`start_seconds`) so the progress bar, resume, Trakt scrobbling and subtitles stay correct.
* Audio sheet: show DTS/TrueHD tracks from the engine info even though the Player doesn't list them,
  labelled e.g. "DTS-HD MA 7.1 → Dolby Digital 5.1" (use `audioinfo.c`); choosing another track
  restarts the remux (or leaves it if native).
* Settings: `dtsConversaoExiste()` returns 1 for `NV_TPK`; update the help/greyed-out texts that say
  "só a LG" (`ajustes.c` ~5177, ~5463, ~5679, ~6954); default 5.1 on this build; translate in 28 tables.
* Remove or reword the silent-playback warning when conversion is active; keep it when the converter is
  unavailable or fails.
Exit: end-to-end on the S95C with the test matrix below.

## Session 5: Hardening, regression and release prep (~1 session)

* Test matrix on the TV (log each with Export logs): DTS 5.1 MKV, DTS-HD MA 7.1, DTS:X, TrueHD 7.1,
  TrueHD Atmos, MKV with DTS + AC3 (must stay native), MP4, 4K HDR10, DV P5/P8, 80 GB remux over a
  slow link, long episode (2 h, memory/CPU steady), seek ×20, pause/resume, audio track switch,
  subtitle track switch, channel/volume via eARC, back-to-menu cleanup (no leaked port/thread).
* Failure injection: kill the source mid-stream, 403/401 source, encoder missing → fall back cleanly.
* Performance budget: converter < 15 % of one core average, start-to-first-frame < 4 s on LAN,
  no dropped frames at 4K; memory cap documented.
* Update `docs/features/dts/README.md` (Tizen section), `tizen-tpk/README.md`, the guide entry if any,
  and `tests/*` listing. Open the PR(s).

## Copy-paste prompts for each Claude session

Start every session with: "Read `docs/plans/dts-truehd-ac3-tizen.md` and the skill `.claude/skills/samsung-tpk`.
Work only on the full NuvioTpk variant. Branch from the previous session's branch. Follow the fixed
behaviour rules. At the end push, and append a hand-off note to the plan."

* **S0:** "Do Session 0. Do not write feature code. Build the smallest throwaway test hook needed, tell me exactly
  what to install and which test files to put on the TV, and write the decision memo."
* **S1:** "Do Session 1 (FFmpeg for the .tpk build). Show size and `readelf` results; keep webOS unchanged."
* **S2:** "Do Session 2 (engine). Extend tests with generated fixtures; do not touch webOS adapter code."
* **S3:** "Do Session 3 using the option decided in S0. Provide a host harness I can run without the TV."
* **S4:** "Do Session 4 (wire-up and Settings, 28 translations). List every file touched."
* **S5:** "Do Session 5. Give me the TV test checklist first, then fix what the logs show."

## Risks / open questions

| Risk | Mitigation |
|---|---|
| Samsung Player may reject fMP4/TS from loopback or lose DV | S0 measures; fallback container list; Option B |
| Seek on a live remux is slow on huge remuxes | Range-prefetch already in engine; restart near keyframe; show buffering state |
| Inbound/loopback socket rules on Tizen 9 | S0 test (also decides the "Export logs" server question) |
| TrueHD/DTS-HD decoding CPU on the TV | Measured in S5; budget above |
| LGPL obligations with static FFmpeg | Decide in S1 (private fork vs shipping relink material) |
| 8-channel sources lose height/objects | Documented in help text; downmix matrix tested in S2 |
| `AJ_DTS_AC3` positional setting migration | No new enum entries; default only for this build |
