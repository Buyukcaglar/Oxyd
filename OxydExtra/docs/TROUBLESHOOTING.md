# Troubleshooting

## The game still switches to 640 × 480

Check the exact executable launched by your shortcut. With several installations, a shortcut can still point to an unpatched copy. The patched executable's SHA-256 is `4959c217a6dc45ab5847dc5db15d6d79c633af6d8a8d2868328e668a021f7028`. Keep `oxscale.dll` beside it. Start that executable directly from the patched folder.

`Oxyd-native-display.log`, when the folder is writable, should contain `Init`, `Fullscreen`, and `Present` entries. `Present` reports the actual native dimensions, integer scale, and viewport.

## Class not registered: DXINPUT.InitInput()

Check that both `dx7vb.dll` and `OxydEx.exe.manifest` are beside the executable, with those exact names. Windows Explorer can hide extensions; ensure the manifest has not become `OxydEx.exe.manifest.txt`. Extract the four files from the named portable ZIP rather than downloading individual HTML preview pages from GitHub.

The manifest and DLL work together. Global COM registration is unnecessary for this package. If the error persists, include the precise HRESULT and the payload hashes in an issue.

## oxscale.dll missing or wrong architecture

Extract all four payload files together. The game and DLL are x86 even on Windows x64. A self-built x64 DLL cannot be loaded by this executable. Use the provided build script's x86 target.

## Missing msvbvm60.dll or unsupported API

The original game needs the Windows x86 Visual Basic 6 runtime. This patch does not bundle that runtime. The companion also uses modern per-monitor DPI APIs and targets Windows 10 version 1703 or later / Windows 11. Older Windows versions have not been supported or tested by this patch.

## Picture is smaller than expected

The largest integer scale that fits the **whole** 640 × 480 frame is used. A 1920 × 1080 desktop therefore uses 2×, not 3×, because a 3× picture would be 1920 × 1440 and would not fit vertically. Black margins are expected. The game has no arbitrary fractional stretch setting in this patch.

If a visible border, clipped menu, or blurred image remains on a supported desktop, record the desktop resolution and Windows display scaling percentage, and attach the native-display log. Windows compatibility options such as reduced-color or forced 640 × 480 modes can interfere with native desktop behavior; inspect the executable's Compatibility tab if those options were previously enabled.

## Mouse, switching modes, or Alt+Tab behaves incorrectly

These need real-game validation. Record whether the issue appears in fullscreen, windowed mode, or after switching, and whether the marble uses relative mouse input or a menu uses pointer coordinates. Include the monitor layout and log. Use the rollback steps in `README.md` if needed, keeping your current `oxconfig.prf`.

## No log file

Logging is best-effort. A read-only game folder can prevent the log from being written. A complete copy of your installation in a writable folder makes diagnostics easier. A missing log alone does not prove that the patch failed to load.

## Report an issue

Open an [issue on GitHub](https://github.com/Buyukcaglar/Oxyd/issues) with the patch/game version, Windows version, desktop dimensions, display scaling percentage, monitor arrangement, exact error or symptom, whether windowed mode works, and `Oxyd-native-display.log` if available. State the steps that reproduce it. Exclude your `oxconfig.prf` unless you specifically intend to share your saved settings/progress.
