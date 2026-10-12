# Nuvio Native Legacy (Tizen Fixes Fork)

This is a custom fork of [iqui27/nuvio-native-legacy](https://github.com), an unofficial native TV client for LG webOS, Samsung Tizen, and Android TV built using C/SDL2/GLES2.

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

For full architecture details, compilation toolchains, and the original implementation guidelines, refer to the upstream [README.md](https://github.com/blob/master/README.md).

---

## 🛠️ Building & Development

Configuration secrets (such as the Supabase backend variables) are excluded from the source repository. Builds read them from a `local.properties` file through `tools/env.sh`:

```properties
NUVIO_SUPABASE_URL=...
NUVIO_SUPABASE_ANON_KEY=...
TV_LOGIN_WEB_BASE_URL=...
```

### Build on GitHub (no local setup)

The workflow `.github/workflows/build-tpk.yml` builds the full `NuvioTpk` package (Tizen 8+/9) on GitHub's servers.

1. **Add the keys once:** repo **Settings → Secrets and variables → Actions → New repository secret**, name it `LOCAL_PROPERTIES` and paste the `local.properties` lines above. The value is hidden after saving and masked in logs. Without it the build still runs, but without API keys.
2. **Start a build:** push to `feat/microfone-busca-por-voz` or `native-microphone`, or use **Actions → Build TPK → Run workflow** (this button appears once the workflow is on the default branch).
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
