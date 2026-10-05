# Build and reproduce the patch

Run these commands from the repository root in Windows PowerShell. Building needs Visual Studio 2022 or newer (including Build Tools) with **Desktop development with C++**, an x86/x64 MSVC toolset, a Windows SDK, and Python 3.10 or later. Running the VB6/input probes also needs the Windows x86 `msvbvm60.dll` runtime.

## Build the companion and tests

```powershell
& .\OxydExtra\tools\build.ps1
```

The script discovers Visual Studio with `vswhere`, selects an x86 compiler, and writes outputs to `OxydExtra/build/`. It also supports an existing x86 Native Tools environment. No machine-specific compiler or SDK version is hard-coded.

If your PowerShell execution policy blocks local scripts, run this once in the build terminal before the command above: `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`. This affects only that terminal process.

It builds `oxscale.dll`, the scaling test, window and loader probes, and the DirectX input and type-library inspection tools. By default it runs the memory-only scaling test, hidden-window test, and explicitly activated local COM input probe. The original game is not started by these tests. `-SkipTests` builds the programs without running them. `-OutputDirectory 'C:\Temp\OxydBuild'` selects a different output folder.

The DLL is compiled with `/O2 /MT /W4 /std:c++17`. `/MT` links the C/C++ runtime statically. The DLL and tests are x86 because the original game is x86. `src/oxscale.def` exposes undecorated names for the stdcall functions used by VB6.

## Reproduce the executable byte patch

Use your **unmodified** game executable, not the prepatched copy from the release. The patcher checks its SHA-256 and refuses any unsupported executable, an existing output file, or an attempt to overwrite the original.

```powershell
python -m venv .\OxydExtra\build\venv
.\OxydExtra\build\venv\Scripts\python.exe -m pip install -r .\OxydExtra\tools\requirements.txt
.\OxydExtra\build\venv\Scripts\python.exe .\OxydExtra\tools\patch_executable.py `
  --input 'C:\Games\Oxyd Extra\OxydEx.original.exe' `
  --output .\OxydExtra\build\portable\OxydEx.exe `
  --report .\OxydExtra\build\patch-report.json
```

Adjust the original input path. Required original SHA-256:

```text
3a12b02496eb5b013d69ce1dd62cc5beb6a155a58ae6af9eccb9ed00db59e02f
```

The reproduced executable should have SHA-256:

```text
4959c217a6dc45ab5847dc5db15d6d79c633af6d8a8d2868328e668a021f7028
```

Compare the generated report with [`evidence/patch-report.json`](../evidence/patch-report.json). The PE checksum is recalculated, the original resource section is checked byte-for-byte, and the three new companion imports are checked.

Prepare a rebuilt portable folder by copying `build/oxscale.dll`, `portable/dx7vb.dll`, and `portable/OxydEx.exe.manifest` beside the new executable. A rebuilt DLL's hash can differ with compiler version and PE timestamp; the prebuilt DLL's hash is the published artifact identifier, not a promise of byte-for-byte reproducible MSVC output.

## Optional probes

The loader probe starts a debugger-controlled process and terminates it when it reaches the original VB6 entry point. It does **not** execute the original game entry point:

```powershell
.\OxydExtra\build\verify_loader.exe 'C:\absolute\path\to\rebuilt\OxydEx.exe'
```

All three companion files must be adjacent to that executable. The probe expects the verified game's original entry RVA (`0xEAD8`). It is not a general-purpose executable debugger.

To verify automatic adjacent-manifest activation:

```powershell
Copy-Item .\OxydExtra\build\probe_input.exe .\OxydExtra\build\probe_auto.exe
Copy-Item .\OxydExtra\portable\OxydEx.exe.manifest .\OxydExtra\build\probe_auto.exe.manifest
.\OxydExtra\build\probe_auto.exe
```

The build script already copies `dx7vb.dll` beside the probes. You can inspect the DLL's COM type library without registration:

```powershell
.\OxydExtra\build\inspect_typelib.exe .\OxydExtra\build\dx7vb.dll
```

## Package the published portable files

```powershell
python .\OxydExtra\tools\package_release.py
```

This verifies the four checked-in payload hashes against `portable/SHA256SUMS.txt`, then writes `dist/OxydExtra-native-desktop-v0.1.0.zip` and `dist/SHA256SUMS.txt`. The ZIP contains those four files plus usage documentation, verification notes, release notes, and licensing notices. It never reads your installed game's settings or includes local backups.

Packaging uses fixed timestamps and entry order. Repeated packaging of the same checkout with the same Python/zlib implementation produces the same archive. The packaging script describes this release; a new release requires updating the version, payload hashes, and notes together.
