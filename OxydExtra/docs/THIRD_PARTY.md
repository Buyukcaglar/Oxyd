# Third-party notices and provenance

## Oxyd Extra 2.0.0

`OxydEx.exe` contains the original game's code and embedded artwork, plus the documented compatibility changes. Oxyd Extra v2 was developed by **Jens Duttke**; his [official portfolio](https://www.duttke.de/) describes the release and credits the original Oxyd games to Dongleware. Oxyd and original game content retain their respective rights holders' rights. This repository is an unofficial compatibility project and does not claim ownership of the original game or trademark.

The MIT license supplied here covers the newly authored patch source, scripts, tests, documentation, and manifest. It does not relicense the original game content in the replacement executable. Original VB6 game source is not included. The `.rsrc` section was verified unchanged by the executable patcher.

Verified original executable SHA-256:

```text
3a12b02496eb5b013d69ce1dd62cc5beb6a155a58ae6af9eccb9ed00db59e02f
```

## Microsoft DirectX 7 Visual Basic wrapper

`dx7vb.dll` is Microsoft's x86 DirectX 7 Visual Basic wrapper, version **4.07.00.0700**, 605,456 bytes. It is proprietary Microsoft software; it is not built from the MIT-licensed patch source.

The DLL was extracted from the original DirectX 7 redistribution package `DX70eng.exe`, retrieved from [the preserved package at Tallinn University](https://www.cs.tlu.ee/~meelis/download/DX70eng.exe). Windows `Get-AuthenticodeSignature` reported **Valid**, with a Microsoft Corporation signer, for that container. The DLL itself has no individual Authenticode signature. Only the wrapper was extracted; the installer was not executed and is not included in the portable release or repository.

Container SHA-256:

```text
7c2e73a4eb06a61c61c7fa141b9d34838feaa7f155fa62f5793218fef76f0742
```

DLL SHA-256:

```text
1e710502e2fc3f70ce37618c5acf82b9ae75c0e5e43325a8b0a495d0a8e559d4
```

The adjacent manifest selects this DLL for this game's COM activation. Windows system DLLs and global COM registration are not modified. The manifest source is included as `portable/OxydEx.exe.manifest`.

## Development tools

The executable patcher uses [`pefile`](https://github.com/erocarrera/pefile), pinned to 2024.8.26 in `tools/requirements.txt`. pefile uses the MIT license and is installed separately for development; it is not bundled in the portable patch.

The helper is built with Microsoft Visual C++ and Windows SDK headers/libraries. It links the C/C++ runtime statically. These development tools are not included. No original installer, locally downloaded analysis library, uninstaller, personal configuration, or diagnostic backup is published.
