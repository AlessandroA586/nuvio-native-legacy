# Brief for the research/architecture session (use the biggest model)

Paste the "PROMPT" block below as your first message in a new Claude Code session opened in this repo
(`git checkout docs/plan-dts-truehd-ac3-tizen` first, so the plan files are present).

## Commands to run first

```sh
git clone https://github.com/Namer03/nuvio-native-legacy-tizen-fixes.git && cd nuvio-native-legacy-tizen-fixes
git checkout docs/plan-dts-truehd-ac3-tizen
claude            # then paste the PROMPT below
```

## PROMPT

You are the architect for one feature in my fork of the Nuvio native app. Do NOT write feature code yet.
Your job is to find the **most optimal, lowest-risk design** and give me a verified, session-by-session
build plan. Work only on the full `NuvioTpk` package (Samsung Tizen 8–9; my TV is a Samsung S95C, Tizen 9).

**Goal.** Whenever the selected audio track is DTS (any variant incl. DTS-HD MA / DTS:X core), TrueHD or
MLP, play it as Dolby Digital (AC3) 5.1 converted on the fly, with the video untouched. Native AC3/E-AC3/AAC
must stay untouched. Today those tracks play silent on Samsung.

**Hard facts (verified, do not re-derive).**
- S95C plays AC3/E-AC3; cannot play any DTS, TrueHD, or Dolby Vision (DV is out of scope; HDR10 only).
- `Tizen.TV.Multimedia.ESPlayer` needs the Partner privilege `esplay` → unavailable to our developer-signed .tpk
  (previous Partner privileges broke install, error 118014). Ruled out.
- `AJ_DTS_AC3` setting exists (Stereo AAC / 5.1 Dolby Digital); `dtsConversaoExiste()` returns 0 for `NV_TPK`.
- Converter engine exists for LG: `src/dts/dts_engine.c` (FFmpeg demux, DTS decode, AAC/AC3 encode, ranged
  HTTP reader), `src/dts/dts_playback.c`; LG-only sink `src/dts/adapter/starfish.cpp`.
- Detection exists: `src/video_tpk.c` `video_tpk_codec_recusado()`, `avaliarAudioTv()` (#313).
- Player on Samsung: `Tizen.Multimedia.Player` in `tizen-tpk/Video.cs` plays a URI on the TV video plane.
- Loopback server template: `src/proxyts.c` (not built on .tpk). FFmpeg build script: `tools/build-dts-ffmpeg.sh`
  (dca only; no TrueHD decoder yet). Build: `NV_TPK_PACOTES=NuvioTpk bash tools/tpk.sh` (Docker).
- Test hook: write a URL into `/tmp/nuvio-video` (polled by `src/main.c` `videoSeSolicitado`) to play a file
  from a PC web server without any new feature.
- Current draft plan and decisions: `docs/plans/dts-truehd-ac3-tizen.md` (6 sessions). Challenge it.

**Read first:** `docs/plans/dts-truehd-ac3-tizen.md`, `docs/features/dts/README.md`, `.claude/skills/samsung-tpk/SKILL.md`,
`src/dts/*`, `src/video_tpk.c`, `src/video.c` (around line 2832), `tizen-tpk/Video.cs`, `tizen-tpk/Program.cs`,
`src/proxyts.c`, `tools/tpk.sh`, `tools/build-dts-ffmpeg.sh`, `docs/issues/MAPA.md` (Samsung DTS/TrueHD entries, #313).

**Questions you must answer with evidence (cite file/line or official docs; search the web where needed):**
1. Best way to get converted audio + original video into the Samsung player WITHOUT Partner privileges.
   Compare at least: (a) loopback remux (fMP4 / MKV / MPEG-TS), (b) loopback HLS with on-demand segments
   (native seeking and track switching), (c) a locally generated MKV/MP4 file on disk with a growing tail,
   (d) anything else in public Tizen.Multimedia / `Tizen.Multimedia.Player` / MediaStreamSource-like APIs.
   For each: seek behaviour, startup latency, A/V sync, HDR10 retention, memory/CPU on S95C, failure modes.
2. Which container/muxer settings does Samsung's player accept for HEVC/H.264 + AC3 5.1 over HTTP loopback
   (use Samsung/Tizen docs and known-good reports; mark what still needs a TV test).
3. Static FFmpeg for the .tpk: minimal component list for DTS/DTS-HD core+XLL, TrueHD/MLP decode, 7.1→5.1
   downmix, AC3 encode, chosen muxer; binary size; LGPL implications of static linking in libnuvio.so.
4. Seek model: restart-at-offset vs HLS. How progress bar, resume, Trakt scrobble and subtitles keep correct time.
5. Audio-track UX: how to list DTS/TrueHD tracks the TV hides, label them, and prefer a native AC3 track.
6. Risks, unknowns, and the cheapest experiments that retire them. Order experiments by risk/cost.

**Deliverable.** Commit to `docs/plans/dts-truehd-ac3-tizen-DESIGN.md` on a new branch from this one:
1. Recommended architecture with a comparison table and the reasoning for rejecting the others.
2. Updated session-by-session plan (each session: goal, files touched, tests, exit criteria, what I must do on the TV).
3. A TV test checklist with exact commands (sdb, the `/tmp/nuvio-video` hook, ffmpeg commands to build test files).
4. Open questions that need my TV, ranked.
Be skeptical of the current draft; if a better idea exists, say so. Mark every claim as VERIFIED (source) or
ASSUMED. Do not invent API names: check them. Push the branch when done and tell me the file name.
