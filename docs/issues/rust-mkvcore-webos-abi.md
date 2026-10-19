# webOS Rust mkvcore activation blocked by ARM float ABI

2026-10-19, branch `agente/205-mkvwebos`. Product activation was requested, but the installed Rust target cannot be linked safely with this SDK. `tools/arm.sh` remains unchanged, using C.

The SDK compiler reports `-mfloat-abi=softfp`. The installed `armv7-unknown-linux-gnueabihf` target produces hardfp objects. A normal archive link against a C caller of `nv_mkvcore_contentor` fails with `uses VFP register arguments`; the linker names both the mkvcore object and a compiler_builtins object. This fails before a Rust product binary exists. `rust_eh_personality` is already defined by the crate; it does not resolve the ABI mismatch.

Reproduce offline from the worktree root:

```sh
bash tests/mkvcore_webos_abi.sh
```

Current result: rc 1. This is an activation gate, not an expected-failure test. Do not suppress the linker ABI check, rewrite ELF attributes, or switch the entire C application to hardfp: the SDK/vendor ABI must remain softfp. `-C llvm-args=-float-abi=soft` did not make the archive linkable; `-C soft-float=yes` is rejected because the compiler removed that option. Neither a softfp Rust target nor rust-src is installed. No network, installation, push, deployment, or SSH was used.

The C high-cache product build completed through its environment checks. SDK objdump reports required versions `GLIBC_2.4`, `GLIBC_2.6`, `GLIBC_2.7`, `GLIBC_2.8`, `GLIBC_2.9`, `GLIBC_2.11`, `GLIBC_2.12`. SDK libc also tops out at `GLIBC_2.12`. C binary size: 57,987,068 bytes. Rust GLIBC maximum and binary size: unavailable because linking fails.

Host checks also expose existing test harness link failures: mkvass variants omit `dados_gravar` or `i18n`; `mkvcore_paridade.sh` defines `mkv_tracks_alem` twice. These failures occur without source changes in this branch. Raw logs and individual command results are in ignored `build/mkvcore-webos-proof/`.

Next prerequisite: provide an ABI-compatible softfp Rust core/compiler_builtins through an authorized offline toolchain. Then enable Rust in the shared ARM build path, retain the C opt-out, and run the product GLIBC/size comparison and host gates. Do not mark webOS ON before those gates pass.
