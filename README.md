# Nuvio Native Legacy (Tizen Fixes Fork)

This is a custom fork of [iqui27/nuvio-native-legacy](https://github.com/iqui27/nuvio-native-legacy), an unofficial native TV client for LG webOS, Samsung Tizen, and Android TV built using C/SDL2/GLES2.

This specific fork focuses on delivering stability hotfixes and new features for **Samsung Tizen OS** devices, specifically targeting Tizen 6 and Tizen 9 (tested on S95C hardware).

> ⚠️ **Development Status: Active Testing & Setup**
> Application logic is being actively integrated. Live feature validation and runtime testing are currently pending local Nuvio Supabase backend key configuration.

---

## 🛠️ Main Application Fixes

*   **Regex Engine Fixes:** Core regex stability adjustments have been successfully implemented directly into the main application logic. `(Status: Integrated / Awaiting Validation)`

## ✨ Features & Roadmap

*   **QR Code Local Log Exporter:** Generates a local QR code on-screen to quickly export application logs for debugging directly from real TV hardware. `(Status: Built / Awaiting Validation)`
*   **Microphone Voice Search:** 🧑‍💻 *Current Focus.* Integrating microphone input capabilities to allow voice-activated content searching directly within the Tizen interface.
*   **DTS / Dolby TrueHD Downmixing:** ⏳ *Up Next.* Implementing audio downmixing logic to convert high-definition audio tracks down to compatible stereo formats, preventing silent playback or black-screen crashes on native TV speakers.

---

## 🚀 Upstream Project Context

This fork inherits the core capabilities of the upstream `nuvio-native-legacy` client:
*   **High Performance:** Built natively in C for high frame rates and low memory overhead on TV hardware.
*   **Multi-Platform Engine:** Native `.tpk` support for older and newer Tizen versions, running directly over OpenGL ES without an underlying web engine.
*   **Feature Rich:** Deep integration with Trakt, Simkl, debrid services, and custom ASS/SSA subtitle rendering.

For full architecture details, compilation toolchains, and the original implementation guidelines, refer to the upstream [README.md](https://github.com/iqui27/nuvio-native-legacy#readme).

---

## 🛠️ Building & Development

Configuration secrets (such as the Supabase backend variables) are excluded from the source repository. Builds read them from a `local.properties` file through `tools/env.sh`:

```properties
NUVIO_SUPABASE_URL=...
NUVIO_SUPABASE_ANON_KEY=...
TV_LOGIN_WEB_BASE_URL=...
```

### Build on GitHub (no local setup)

The workflow `.github/workflows/build-tpk.yml` builds the full `NuvioTpk` package (Tizen 8+/9) on GitHub's servers. It runs on every push, and you can also start it by hand.

1. **Add the keys once:** repo **Settings → Secrets and variables → Actions → New repository secret**, name it `LOCAL_PROPERTIES` and paste the `local.properties` lines above. The value is hidden after saving and masked in logs. Without it the build still runs, but without API keys.
2. **Start a build:** push to any branch, or use **Actions → Build TPK → Run workflow** (choose the branch under "Use workflow from"). The button only appears once the workflow is on `master`, and a branch needs the workflow file to build on push, so merge `master` into older branches first.
3. **Download the result:** open the finished run, scroll to **Artifacts** and download `NuvioTpk`. Unzip it to get the `.tpk`. Artifacts are kept for 90 days.

The first build is slow (roughly 1–2 hours) because the C code is compiled under ARM emulation; later runs reuse the cached libraries. If a build fails, the last 60 lines of the log are shown at the top of the run page.

> **Note:** If the repository is public, any logged-in GitHub user can download the artifact, and the keys are compiled into the `.tpk`. Make the repository private if only you should get the package.

### Build locally

```bash
# To build the native Samsung .tpk targets (requires Docker + .NET 8 SDK)
bash tools/tpk.sh

# Only the Tizen 8+/9 package (needs the Samsung .NET workload)
NV_TPK_PACOTES=NuvioTpk bash tools/tpk.sh

# To build and check all native Samsung packages for a release
bash tools/release-samsung.sh
```

On machines where Docker rejects the `linux/arm/v5` platform name (this includes the GitHub workflow), use plain `linux/arm`:

```bash
sed -i 's#linux/arm/v5#linux/arm#g' tools/tpk.sh
```

Converting the app art needs `ffmpeg` (or `sips` on macOS) on the build machine.

> **Note:** Packages ship unsigned on purpose. You will need to use a tool like **Apps2Samsung** to sign the generated `.tpk` file with your specific TV's DUID profile before deploying it to your hardware.

---

## 📜 License & Credits

*   **Original Project:** All credit for the underlying native client engine goes to `iqui27` and the `CraaazyDevs` community. 
*   **License:** This fork is licensed under the **GNU General Public License v3.0**. 
*   "Nuvio" branding and logos belong entirely to their respective trademark owners.
autiful Nuvio experience on my LG C9. It now reaches more TVs, with help from people testing builds and sharing feedback.

If you’d like to support the time and care behind it, visit [CraaazyDevs on Patreon](https://www.patreon.com/cw/CraaazyDevs). Support is entirely optional. Bug reports, testing, and helping other users matter too.

This is an independent, unofficial project, not affiliated with NuvioMedia. Credit for Nuvio goes to its original creators.

---

## Reporting a problem

1. Right after the problem happens, send the log from the app: **Settings ›
   About › Send log** (on LG you can also open the log panel from the remote).
2. Open an [issue](https://github.com/iqui27/nuvio-native-legacy/issues) with your
   TV model, the app version and what you did.

The log never carries credentials. Logs are kept for 30 days and are only used
to fix bugs. You can also turn on automatic log sending in Settings.

---

## Building

```bash
bash tools/mac.sh              # build and run on macOS (UI only, no video)
bash tools/linux.sh            # build and run on Linux (UI only, no video)
bash tools/linux.sh --build    # compile the Linux desktop preview only
bash tools/linux.sh --preview  # browse the Linux UI without signing in
bash tools/arm.sh              # cross-compile for LG in Docker and deploy over ssh
bash tools/arm.sh --ipk        # also produce the .ipk
bash tools/tizen.sh            # build the Samsung target (WebAssembly)
bash tools/tizen-wgt.sh        # package it as an unsigned .wgt
bash tools/tpk.sh              # native Samsung: the four .tpk (Docker + .NET 8)
bash tools/release-samsung.sh  # every Samsung package for a release, checked
bash tools/hb-repo.sh <ipk> <dir>   # Homebrew Channel repo files for a release
```

On an x86_64 Linux host with Podman, build the SDK with its x86_64 toolchain
and select the runtime and packaging CLI explicitly:

```bash
podman build --platform linux/amd64 \
  --build-arg SDK_URL=https://github.com/openlgtv/buildroot-nc4/releases/download/webos-a38c582/arm-webos-linux-gnueabi_sdk-buildroot-x86_64.tar.gz \
  -t nuvio-webos-sdk tools/
NUVIO_CONTAINER_RUNTIME=podman NUVIO_BUILD_PLATFORM=linux/amd64 \
  NUVIO_ARES_PACKAGE=/path/to/ares-package bash tools/arm.sh --ipk --build
```

`--ipk --build` creates the package without deploying to a TV. The target
binary is ARMv7 regardless of the build host architecture.

webOS builds include DTS audio fallback by default. TVs that accept DTS keep
native playback; when the selected DTS track is unsupported, the app converts
its audio to stereo AAC and keeps video on the TV's hardware pipeline. See the
[DTS guide](docs/features/dts/README.md) for limits and verification, and
[DTS Debug build commands](tools/dts-pipeline/DEBUG.md) for a separate diagnostic
app that can be installed alongside the normal one.

For Linux UI previews, install the SDL2, SDL2_image, SDL2_ttf, GLES2, EGL and
zlib development packages, a C compiler and pkg-config. `--preview` skips login
and uses a separate data directory. Arrow keys and Enter navigate; Escape or
Backspace goes back. Video playback is unavailable in this desktop preview.

Builds read `local.properties` from the repository root or the legacy neighboring
web project; `NUVIO_PROPERTIES` selects another file.

Tests are shell scripts in `tests/` (`bash tests/<name>.sh`); most of them build
a piece of `src/` on the host with the network or the TV faked.

Server URLs and client ids are **not in the source**. They come from a
`local.properties` file through `tools/env.sh`. The build refuses to package any
credential file and deletes the `.ipk` if one shows up (`tools/testa-ipk.sh`
checks by extracting the `ar` archive).

---

## Things I learned porting to webOS

Each one of these cost me at least a day:

- The video plane sits **behind** the GL surface and shows through an alpha
  hole. `glReadPixels` and the TV's screenshot never see it, so a capture during
  playback is black. That's the design, not a bug.
- `StarfishMediaAPIs` calls `exit(0)` when the process doesn't match `exeName` in
  its LS2 role. Silent, no crash. Talking to `com.webos.media` over LS2 directly
  is what works.
- webOS 5 removed `libAcbAPI`. The app carries both paths: ACB on webOS 2–4,
  SDL's exported window on 5+, picked at startup.
- When SAM launches a native app, stdout and stderr go to `/dev/null`. Nothing you
  print survives until you `freopen` a log file.
- The Back button doesn't arrive until the window asks for it, through the
  `SDL_WEBOS_ACCESS_POLICY_KEYS_BACK` hint, and only if it's set **before** the
  window is created. It then arrives as scancode 482.
- Don't link libcurl. The SDK has `.so.4`, the TV has `.so.5`, and the binary won't
  start. `dlopen` it at runtime and try both.
- The TV's libcurl (7.53) is old: reusing connections helps a lot, but only after
  a transfer finished cleanly.
- Firmware symbol dumps from [webosbrew](https://github.com/webosbrew/dev-toolbox-cli)
  tell you whether a binary will **load** on a TV you don't own. They don't tell
  you it works. That's how webOS 3 support was checked: exactly one missing
  symbol (`SDL_CreateRGBSurfaceWithFormat`, SDL 2.0.5), replaced in
  `src/sdlcompat.h`, so the same package runs on webOS 3 and up.

And on Samsung, where the app is WebAssembly in the TV's browser: one busy main
thread and the whole app waits. Most of the Samsung speedups came from moving
work off it (image decoding in a Worker, no synchronous storage writes, no
per-frame file checks).

---

## Documentation

Most design notes are in Portuguese, because I wrote them for myself:
[PORT-LEGACY.md](PORT-LEGACY.md), [PLANO-CONTA-SYNC.md](PLANO-CONTA-SYNC.md),
[MEDIDAS-WEB.md](MEDIDAS-WEB.md), [FERRAMENTAS.md](FERRAMENTAS.md).
Release notes live in `docs/releases/`. [INSTALL.md](INSTALL.md) and this file are
in English.

## Disclaimer

Nuvio Legacy is a **media player**. It does not host, store, index, upload or
distribute any movies, series, channels or other content, and it ships with no
content sources. Everything it plays comes from add-ons, playlists, servers and
accounts that **you** choose to install or connect. Those are run by third
parties that have nothing to do with this project.

- You are responsible for what you add and watch, and for making sure you have
  the right to access it where you live.
- This project does not control, endorse or verify any add-on, playlist or
  service, and does not provide links to unlicensed content.
- There is nothing hosted here to take down. Copyright notices about a stream
  should go to whoever hosts it (the add-on, playlist or server), not to this
  repository.
- Nuvio Legacy is not affiliated with Stremio, the official Nuvio app, any
  streaming service, debrid service or TV maker.

## License

[GNU General Public License v3.0](LICENSE).

The C in `src/` is written from scratch, but the interface icons in
`deploy/app/art/icones/` come from the SVGs of the upstream web app, which is
GPLv3, as is the [web fork](https://github.com/iqui27/NuvioTVSmart-legacy-webos)
this grew alongside. So the whole thing goes out under the same terms.

Settings icons: [Lucide](https://lucide.dev), ISC license. The unmodified SVGs
and the license text are in `deploy/app/art/icones/lucide/`; the `aj_*.png` next
to them are rasterized by `tools/icones-lucide.sh`.

DTS-enabled builds use libraries from the [FFmpeg project](https://ffmpeg.org/)
under **LGPL 2.1 or later**. `tools/build-dts-ffmpeg.sh` records the exact release
source URL and checksum; `tools/arm.sh` includes its license and provenance in
`licenses/dts/` inside the app package. The
[DTS distribution notes](docs/features/dts/README.md#dependency-provenance-and-distribution)
describe the corresponding source and static relinking materials needed with
binary releases.

"Nuvio", the logo and the wordmark belong to the original authors. The GPL covers
the code, **not** the name or the branding. They appear here only to say which
project this one comes from.

Artwork fetched at runtime (TMDB, Trakt, Apple TV, addon CDNs) belongs to its
owners and isn't covered by this license.
