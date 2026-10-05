# Verification record

Prepared for patch **0.1.0** on **2026-10-05**, using Windows 11 x64 and MSVC 14.51 with an x86 target. These checks distinguish source/build evidence, component behavior, loader behavior, and actual user acceptance.

| Check | Result | Scope |
| --- | --- | --- |
| C++ build | Passed | Companion and probes compiled with `/W4 /O2 /MT`. |
| Runtime integer geometry | Passed | Six desktop sizes, including 1080p, 1600p, and 4K. |
| Nearest-neighbor presentation | Passed | All 2,764,800 pixels of a 3× memory-DIB image matched the exact source pixel. |
| Native window / logical client | Passed | Hidden test window used a native 2560 × 1600 client while VB6 saw 640 × 480. |
| Logical capture | Passed | All 307,200 captured pixels matched the source frame. |
| Desktop mode preservation | Passed | Hidden-window test left resolution, bit depth, and refresh rate unchanged. |
| Patched game Windows loader | Passed | Loaded the companion, ran initialization, and reached the original VB6 entry point. |
| PE structure / resources | Passed | Valid checksum, no pefile warnings, embedded resources byte-identical. |
| DirectX dependency probes | Passed | Local and automatic adjacent-manifest activation created DirectX7, DirectInput, and a mouse device. |
| Original input fix in actual game | User-confirmed | The game started successfully after the local dependency fix. |
| Native-display patch in actual game | Pending | Visual appearance, gameplay, mouse behavior, switching, and Alt+Tab still need confirmation. |

The loader probe terminates at the original entry point **before executing the original game code**. A hidden-window probe exercises the companion, not the original game. Compilation and these component checks do not demonstrate full-game, audio, network, or multi-monitor fidelity.

Sources for the checks are in `src/oxscale.cpp` (the `OXSCALE_TEST` build) and `tests/`. The build guide explains how to rerun them. Machine-readable records are in [`evidence/`](https://github.com/Buyukcaglar/Oxyd/tree/oxydextra-v0.1.0/OxydExtra/evidence/).

The publication build also reran the source build, geometry/pixel checks, hidden-window check, and local COM probe using the portable build script. The command-line executable patcher reproduced the published executable byte-for-byte from the hash-checked original. Release packaging verified all payload hashes; the archive was checked for the intended files and absence of settings/progress.

The released files preserve the already prepared patch payload. Updated documentation and portable build tools do not change that payload.
