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

Because configuration secrets (such as Supabase backend variables) are excluded from the source repository, you will need a valid `local.properties` file mapped via `tools/env.sh` to package and build successful binaries.

```bash
# To build the native Samsung .tpk targets (requires Docker + .NET 8 SDK)
bash tools/tpk.sh

# To build and check all native Samsung packages for a release
bash tools/release-samsung.sh
```

> **Note:** Packages ship unsigned on purpose. You will need to use a tool like **Apps2Samsung** to sign the generated `.tpk` file with your specific TV's DUID profile before deploying it to your hardware.

---

## 📜 License & Credits

*   **Original Project:** All credit for the underlying native client engine goes to `iqui27` and the `CraaazyDevs` community. 
*   **License:** This fork is licensed under the **GNU General Public License v3.0**. 
*   "Nuvio" branding and logos belong entirely to their respective trademark owners.
