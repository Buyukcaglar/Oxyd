# Oxyd Extra: native desktop and integer scaling patch

**Patch version 0.1.0 · Game version 2.0.0 · Windows x86 executable**

Fullscreen uses the monitor's current desktop resolution. The game continues to draw its original 640 × 480 frame, enlarged by the largest whole-number scale that fits, centered with black margins. Pixels remain sharp through nearest-neighbor scaling. The scale is calculated at runtime on each computer; no desktop resolution is hard-coded.

This package also fixes `DXINPUT.InitInput(): -2147221164 / Class not registered` by loading Microsoft's DirectX 7 Visual Basic wrapper from the game folder using registration-free COM.

## Download and requirements

Download **`OxydExtra-native-desktop-v0.1.0.zip`** from the [release page](https://github.com/Buyukcaglar/Oxyd/releases/tag/oxydextra-v0.1.0). GitHub's automatic source archives are intended for developers; the named portable ZIP contains the ready-to-use patch.

Use an existing installation of **Oxyd Extra 2.0.0** with its other game files intact. The release supplies a replacement executable and its three companion files; it does not include the original installer, help file, or your settings/progress.

The intended platform is Windows 10 version 1703 or later, or Windows 11, with x86 application support and the Windows Visual Basic 6 runtime (`msvbvm60.dll`). The patch was built and checked on Windows 11 x64. Other Windows versions and Windows on ARM have not been verified. Normal use requires no compiler, Python, extra C++ runtime installer, or COM registration.

Before replacing the executable, confirm that your original `OxydEx.exe` has this SHA-256:

```text
3a12b02496eb5b013d69ce1dd62cc5beb6a155a58ae6af9eccb9ed00db59e02f
```

In PowerShell, run:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\Games\Oxyd Extra\OxydEx.exe'
```

Change that example path to your actual installation. A different hash means the executable is outside the version verified for this patch. The source patcher also enforces this exact hash.

## Install

1. Close the game. Make a backup copy of its folder, or at least its original `OxydEx.exe` and any existing copies of the three companion files listed below.
2. Extract the portable ZIP to a temporary folder.
3. Copy these **four files** into the existing Oxyd Extra game folder, beside its other files. Replace `OxydEx.exe` when prompted:

   ```text
   OxydEx.exe
   oxscale.dll
   dx7vb.dll
   OxydEx.exe.manifest
   ```

4. Keep your existing `oxconfig.prf`; it holds settings and progress. The patch ZIP does not contain that file.
5. Start **the replaced `OxydEx.exe` in that folder**. Check shortcut targets if you have several installations. Leave all three companion files beside it.

A folder under `Program Files` may require permission when copying replacement files. You can instead work with a complete copy of your installation in a user-writable folder. No `regsvr32`, DirectX installer, Windows system DLL replacement, or launcher is required by this patch.

The release includes `SHA256SUMS.txt`. Compare the four extracted files against those hashes with `Get-FileHash`. The release page also provides a separate checksum file for the ZIP download.

## Use and scaling

Start the game normally and use its fullscreen mode. Its existing fullscreen/windowed toggle remains available. Integer scaling applies to fullscreen; windowed mode uses the game's own sizing.

The current monitor client size is read during presentation:

```text
scale = max(1, min(width // 640, height // 480))
left  = (width  - 640 * scale) // 2
top   = (height - 480 * scale) // 2
```

Here `//` means integer division. These are examples, not configuration values:

| Desktop | Scale | Picture size | Left/top margin |
| --- | --- | --- | --- |
| 640 × 480 | 1× | 640 × 480 | 0 / 0 |
| 1366 × 768 | 1× | 640 × 480 | 363 / 144 |
| 1920 × 1080 | 2× | 1280 × 960 | 320 / 60 |
| 1280 × 1024 | 2× | 1280 × 960 | 0 / 32 |
| 2560 × 1600 | 3× | 1920 × 1440 | 320 / 80 |
| 3840 × 2160 | 4× | 2560 × 1920 | 640 / 120 |

The largest scale that fits preserves the entire frame. A desktop smaller than 640 × 480 stays at 1× and can clip the picture; downscaling is not implemented. The patch follows the monitor selected from the game window and updates its bounds on display-change events. It does not select a new resolution, color depth, or refresh rate.

## First-run check

This is a **prerelease**. Automated component checks and the Windows loader probe passed. The input dependency fix was user-confirmed to start the game. Visual and gameplay acceptance of the native-display patch remains pending.

On your first run, check that the menu and a level are centered and sharp, the entire frame fits, mouse control works, the fullscreen/windowed toggle and Alt+Tab behave correctly, and quitting leaves the desktop at its original display mode. Audio, network play, and a full-game playthrough have not been validated for this release.

When the folder is writable, `Oxyd-native-display.log` records startup, detected monitor bounds, and the first presented frame's integer scale and viewport. Keep that log when reporting display problems. More help is in [Troubleshooting](docs/TROUBLESHOOTING.md).

## Roll back

Close the game and restore your backed-up original `OxydEx.exe`. Remove `oxscale.dll`, or restore its previous copy if one existed. Keep `dx7vb.dll` and `OxydEx.exe.manifest` if you still need the input fix. To remove that fix too, restore the prior versions of those files or remove the two added files if none existed before. Keep your current `oxconfig.prf` to retain progress.

## Source and documentation

- [`src/`](src/): complete C++ source and export definition for `oxscale.dll`.
- [`tools/`](tools/): portable build script, executable patcher, and release packaging script.
- [`tests/`](tests/): scaling/window/loader checks and DirectX COM inspection probes.
- [`portable/`](portable/): the four ready-to-use patch files and their checksums.
- [`evidence/`](evidence/): exact byte changes, provenance, and verification results.
- [Build guide](docs/BUILD.md) · [Architecture](docs/ARCHITECTURE.md) · [Verification](docs/VERIFICATION.md).

These are the sources of the compatibility patch. The original game's VB6 source and Microsoft's DLL source are not available in this repository. See [license](LICENSE) and [third-party notices](docs/THIRD_PARTY.md).
