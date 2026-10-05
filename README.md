# Oxyd

Windows compatibility patches for the Oxyd games, with complete source for the patches and downloadable portable releases.

The first patch targets **Oxyd Extra 2.0.0**. It keeps the monitor at its current desktop resolution, displays the original 640 × 480 picture with runtime integer scaling, and supplies a local DirectX 7 input dependency fix.

- **[Download the portable patch](https://github.com/Buyukcaglar/Oxyd/releases/tag/oxydextra-v0.1.0)**
- **[Install, use, and roll back](OxydExtra/README.md)**
- [Build from source](OxydExtra/docs/BUILD.md)
- [Implementation](OxydExtra/docs/ARCHITECTURE.md) · [Troubleshooting](OxydExtra/docs/TROUBLESHOOTING.md)
- [Verification and limitations](OxydExtra/docs/VERIFICATION.md) · [Release notes](OxydExtra/releases/oxydextra-v0.1.0.md)

All Oxyd Extra patch files, source, tools, tests, and documentation are in [`OxydExtra/`](OxydExtra/). The initial release is a **prerelease**: component and Windows loader checks passed; a complete visual and gameplay check of the native-display patch remains pending. The earlier input dependency fix was confirmed to start the game successfully.

This is an unofficial compatibility project. The original game was developed by [Jens Duttke](https://www.duttke.de/) and belongs to its respective rights holders. Patch source and documentation use the [MIT license](OxydExtra/LICENSE); the bundled original game content and Microsoft DLL retain their own rights. See [third-party notices and provenance](OxydExtra/docs/THIRD_PARTY.md).
