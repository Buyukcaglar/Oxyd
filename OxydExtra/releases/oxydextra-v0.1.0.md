# Oxyd Extra native desktop patch v0.1.0

Initial **prerelease** for the verified Windows **Oxyd Extra 2.0.0** executable.

## Changes

- Fullscreen covers the current native desktop without switching to 640 × 480 or changing color depth/refresh rate.
- The original 640 × 480 frame is enlarged with nearest-neighbor pixels at the largest integer scale that fits, centered with black margins.
- Scale and monitor dimensions are calculated at runtime on each system, with no fixed resolution or multiplier.
- `oxscale.dll` loads automatically through the patched executable; the existing window-mode toggle remains available.
- Local `dx7vb.dll` and registration-free COM manifest address `DXINPUT.InitInput(): Class not registered` without system-wide DLL registration.
- Complete patch source, portable build/patch/package tools, verification probes, provenance, checksums, usage and rollback documentation are published under `OxydExtra/`.

## Download and install

Download **`OxydExtra-native-desktop-v0.1.0.zip`** below, plus **`SHA256SUMS.txt`** if you want to verify the archive. The automatic GitHub source archives contain repository source; use the named ZIP for the ready-to-use patch.

Close the game and back up its original executable. Verify the supported original SHA-256 in the [usage guide](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/README.md), then copy `OxydEx.exe`, `oxscale.dll`, `dx7vb.dll`, and `OxydEx.exe.manifest` into the existing game folder. Keep your current `oxconfig.prf`. Launch the replaced executable directly.

The portable package requires an existing Oxyd Extra 2.0.0 installation, modern Windows with x86/VB6 runtime support, and no Python or compiler for normal use. See [requirements, installation, use, and rollback](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/README.md) and [troubleshooting](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/docs/TROUBLESHOOTING.md).

## Validation and known limitations

The x86 companion and probes compiled successfully. Tests passed for six desktop geometries, every pixel of a 3× nearest-neighbor test frame, a hidden native desktop window and logical framebuffer capture, unchanged desktop mode, local DirectX COM activation, PE integrity, and the actual executable's bootstrap reaching its original VB6 entry point.

The earlier input dependency fix was user-confirmed to start the game. **The native-display patch still needs a complete visual/gameplay check.** The loader probe stops before original game code runs. Audio, network play, multi-monitor transitions, Windows on ARM, and full-game behavior have not been verified. A desktop smaller than 640 × 480 can clip the 1× frame. See the [verification record](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/docs/VERIFICATION.md).

## Source and attribution

[All patch source and documentation](https://github.com/Buyukcaglar/Oxyd/tree/oxydextra-v0.1.0/OxydExtra) · [Build guide](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/docs/BUILD.md) · [Exact byte changes](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/evidence/patch-report.json).

Patch source and documentation use the MIT license. The replacement executable contains original game content and the package includes Microsoft's DirectX wrapper, which retain their own rights. See [third-party notices](https://github.com/Buyukcaglar/Oxyd/blob/oxydextra-v0.1.0/OxydExtra/docs/THIRD_PARTY.md). This is an unofficial compatibility project.
