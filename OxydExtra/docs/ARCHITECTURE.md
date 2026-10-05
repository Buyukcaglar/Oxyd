# Patch implementation

The original executable is a 32-bit native Visual Basic 6 program. Its visible frame is drawn through GDI; the DirectDraw 7 wrapper is used to enter an exclusive 640 × 480 display mode. The patch replaces the display-mode path and presents a logical framebuffer through a small x86 companion DLL.

## Executable changes

[`patch_executable.py`](../tools/patch_executable.py) accepts only the exact verified original binary. It adds `.oxdata` (read/write import data) and `.oxcode` (executable startup and call stubs), and imports `OxScaleInit`, `OxEnterFullscreen`, and `OxEnterWindowed` from `oxscale.dll`.

The new entry saves registers and flags, calls `OxScaleInit`, restores them, and jumps to the original VB6 entry. Existing VB6 imports are preserved; the obsolete bound-import metadata is removed so the loader resolves the current runtime. The game retains its original fixed image base.

The following offsets are file offsets in the original executable:

| Offset | Change |
| --- | --- |
| `0x4534B` | Route fullscreen cooperative-level setup to the companion. |
| `0x453C3` | Suppress the 640 × 480, 32-bpp display-mode call. |
| `0x4542B` | Suppress the 640 × 480, 16-bpp fallback call. |
| `0x45C59` | Suppress the display-mode restore call. |
| `0x45CAA` | Route windowed cooperative-level setup to the companion. |
| `0x15768` | Redirect the game's AlphaBlend library descriptor. |
| `0x158DC` | Redirect the game's BitBlt library descriptor. |
| `0x15E18` | Redirect the game's StretchBlt library descriptor. |

Suppressed calls perform their original stdcall stack cleanup and return `S_OK`. The patched file grows by 8192 bytes. Headers and checksum are updated. The game's embedded `.rsrc` data is unchanged. [`patch-report.json`](../evidence/patch-report.json) records the original and replacement bytes, entry points, hashes, and checks.

## Native desktop window

At startup `OxScaleInit` requests per-monitor DPI awareness v2 before VB6 creates the game window. It redirects four entries in this process's loaded `msvbvm60.dll` import table: `BitBlt`, `StretchBlt`, `StretchDIBits`, and `GetClientRect`. These changes are in process memory and do not modify the Windows runtime on disk.

Fullscreen setup saves the window's styles and original window procedure, finds its monitor with `MonitorFromWindow`, and sizes a borderless popup to that monitor's current rectangle. The subclass keeps the fullscreen bounds in place, supplies logical 640 × 480 size notifications to VB6, and responds to display-change events. The runtime's client rectangle hook reports 640 × 480 only for the fullscreen game window. The actual Windows client remains at native size.

Windowed setup restores the saved styles and lets the original game continue its window sizing. There are no `SetDisplayMode` or `ChangeDisplaySettings` requests in the companion.

## Logical frame and presentation

GDI writes targeting the fullscreen game window compose into a 640 × 480 top-down, 32-bit DIB. Alpha blending happens at that logical resolution. Reads from that game window capture the logical framebuffer. Offscreen sprite and layer operations pass through to their normal GDI functions.

Each presentation reads the actual native client size and computes `max(1, min(width / 640, height / 480))` using integer division. It fills the margins black and stretches the logical frame with GDI `COLORONCOLOR`. At an exact integer scale, each source pixel occupies a square of that many output pixels; no smoothing filter is used.

Ordinary window mouse message coordinates are translated back to the logical viewport and clamped to its edges. DirectInput polling is unchanged. The original game logic, timing, artwork, audio, and network code are outside the byte patch.

## Local DirectX dependency

The adjacent `OxydEx.exe.manifest` maps the DirectX 7 COM class and type library to `dx7vb.dll` in the game folder:

- DirectX7 class: `{E1211353-8E94-11D1-8808-00C04FC2C602}`.
- IDirectX7 interface used by the probe: `{FAFA3599-8B72-11D2-90B2-00C04FC2C602}`.
- Type library: `{E1211242-8E94-11D1-8808-00C04FC2C602}`, version 1.0.

This uses Windows registration-free COM. It supplies the missing wrapper without registering it globally or replacing Windows DLLs.

## References

- [Microsoft: StretchBlt](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchblt).
- [Microsoft: SetStretchBltMode / COLORONCOLOR](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setstretchbltmode).
- [Microsoft: assembly manifests](https://learn.microsoft.com/en-us/windows/win32/sbscs/assembly-manifests).
- [Microsoft: COM error codes, including REGDB_E_CLASSNOTREG](https://learn.microsoft.com/en-us/windows/win32/com/com-error-codes-1).
- [Microsoft: per-monitor DPI awareness v2](https://learn.microsoft.com/en-us/windows/win32/hidpi/dpi-awareness-context).
