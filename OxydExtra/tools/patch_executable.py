"""Create the native-display executable from the exact original Oxyd Extra 2.0."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import pefile

ORIGINAL_SHA256 = '3a12b02496eb5b013d69ce1dd62cc5beb6a155a58ae6af9eccb9ed00db59e02f'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', type=Path, required=True, help='Unmodified OxydEx.exe (SHA-256 checked)')
parser.add_argument('--output', type=Path, required=True, help='New executable; must not already exist')
parser.add_argument('--report', type=Path, help='Optional JSON byte-change report; must not already exist')
args = parser.parse_args()
source = args.input.resolve()
output = args.output.resolve()
if output == source or output.exists():
    parser.error('Output must be a new file; the original is never overwritten.')
if args.report and (args.report.exists() or args.report.resolve() in (source, output)):
    parser.error('Report must be a separate new file.')
original = source.read_bytes()
if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
    raise SystemExit('Original executable hash mismatch; nothing patched.')
data = bytearray(original)
pe = pefile.PE(data=original)
image_base = pe.OPTIONAL_HEADER.ImageBase
assert pe.FILE_HEADER.Machine == 0x14c and image_base == 0x400000
assert not (pe.OPTIONAL_HEADER.DllCharacteristics & 0x40), 'Unexpected ASLR configuration'

def align(value, boundary):
    return (value + boundary - 1) // boundary * boundary

section_alignment = pe.OPTIONAL_HEADER.SectionAlignment
file_alignment = pe.OPTIONAL_HEADER.FileAlignment
last = pe.sections[-1]
assert last.PointerToRawData + last.SizeOfRawData == len(original), 'Unexpected file overlay'
data_rva = align(last.VirtualAddress + max(last.Misc_VirtualSize, last.SizeOfRawData), section_alignment)
data_raw = align(len(data), file_alignment)
code_rva = data_rva + section_alignment
code_raw = data_raw + file_alignment
new_data = bytearray(file_alignment)
new_code = bytearray(file_alignment)
changes = []

def replace(offset, expected, replacement, purpose):
    expected = bytes.fromhex(expected) if isinstance(expected, str) else expected
    assert len(expected) == len(replacement)
    if data[offset:offset+len(expected)] != expected:
        raise SystemExit(f'Unexpected bytes at 0x{offset:X}; nothing installed.')
    data[offset:offset+len(expected)] = replacement
    changes.append({'file_offset': f'0x{offset:X}', 'original': expected.hex(),
                    'patched': replacement.hex(), 'purpose': purpose})

imports_rva = pe.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress
imports_raw = pe.get_offset_from_rva(imports_rva)
descriptor_count = len(pe.DIRECTORY_ENTRY_IMPORT)
assert descriptor_count == 1
descriptors = original[imports_raw:imports_raw+20*descriptor_count]
new_data[:len(descriptors)] = descriptors
# Discard the obsolete VB6 runtime binding; the loader resolves its current
# exports from the preserved original lookup table on each system.
struct.pack_into('<II', new_data, 4, 0, 0)
dll_offset, ilt_offset, iat_offset = 0x80, 0x100, 0x120
new_data[dll_offset:dll_offset+12] = b'oxscale.dll\0'
names = ['OxScaleInit', 'OxEnterFullscreen', 'OxEnterWindowed']
cursor = 0x160
for i, name in enumerate(names):
    encoded = b'\0\0' + name.encode('ascii') + b'\0'
    new_data[cursor:cursor+len(encoded)] = encoded
    for offset in (ilt_offset, iat_offset):
        struct.pack_into('<I', new_data, offset+4*i, data_rva+cursor)
    cursor = align(cursor+len(encoded), 2)
struct.pack_into('<IIIII', new_data, 20*descriptor_count,
    data_rva+ilt_offset, 0, 0, data_rva+dll_offset, data_rva+iat_offset)

# Preserve every startup register and flag, initialize the companion, and
# continue at the original VB6 entry point. The executable has a fixed base.
old_entry = pe.OPTIONAL_HEADER.AddressOfEntryPoint
bootstrap = b'\x9c\x60\xff\x15' + struct.pack('<I', image_base+data_rva+iat_offset) + b'\x61\x9d\xe9'
bootstrap += struct.pack('<i', old_entry-(code_rva+len(bootstrap)+4))
new_code[:len(bootstrap)] = bootstrap
for code_offset, import_index in [(0x20, 1), (0x30, 2)]:
    stub = b'\xff\x25' + struct.pack('<I', image_base+data_rva+iat_offset+4*import_index)
    new_code[code_offset:code_offset+len(stub)] = stub

def call_stub(offset, stub_offset):
    target = image_base+code_rva+stub_offset
    instruction_va = image_base+pe.get_rva_from_offset(offset)
    return b'\xe8' + struct.pack('<i', target-instruction_va-5)

# VB DirectDraw7 wrapper: SetCooperativeLevel(This, hwnd, flags),
# SetDisplayMode(This, width, height, bpp, refresh, flags), RestoreDisplayMode(This).
# Replace call + fnclex together; each replacement preserves stdcall cleanup.
replace(0x4534b, 'ff5074dbe2', call_stub(0x4534b, 0x20), 'Enter native borderless fullscreen')
replace(0x453c3, 'ff5278dbe2', bytes.fromhex('83c41831c0'), 'Suppress 640x480 32-bpp SetDisplayMode; return S_OK')
replace(0x4542b, 'ff5178dbe2', bytes.fromhex('83c41831c0'), 'Suppress 640x480 16-bpp SetDisplayMode; return S_OK')
replace(0x45c59, 'ff5170dbe2', bytes.fromhex('83c40431c0'), 'Suppress RestoreDisplayMode; desktop mode remains untouched')
replace(0x45caa, 'ff5074dbe2', call_stub(0x45caa, 0x30), 'Retain original window-mode toggle')
for descriptor_offset, library_offset, name in [
        (0x15768, 0x1574c, 'AlphaBlend'),
        (0x158dc, 0x15010, 'BitBlt'),
        (0x15e18, 0x15010, 'StretchBlt')]:
    replace(descriptor_offset, struct.pack('<I', image_base+library_offset),
            struct.pack('<I', image_base+data_rva+dll_offset),
            f'Redirect game {name} to logical-frame presentation; offscreen calls pass through')

section_table = pe.sections[0].get_file_offset()
next_header = section_table + 40*len(pe.sections)
assert next_header+80 <= pe.OPTIONAL_HEADER.SizeOfHeaders
bound = pe.OPTIONAL_HEADER.DATA_DIRECTORY[11]
assert bound.VirtualAddress == next_header and bound.Size <= 80
assert not any(data[next_header+bound.Size:next_header+80]), 'Unexpected section-header data'
struct.pack_into('<II', data, bound.get_file_offset(), 0, 0)
for header, name, rva, raw, used, flags in [
        (next_header, b'.oxdata', data_rva, data_raw, cursor, 0xc0000040),
        (next_header+40, b'.oxcode', code_rva, code_raw, 0x36, 0x60000020)]:
    struct.pack_into('<8sIIIIIIHHI', data, header, name.ljust(8,b'\0'), used,
                     rva, file_alignment, raw, 0, 0, 0, 0, flags)
struct.pack_into('<H', data, pe.FILE_HEADER.get_field_absolute_offset('NumberOfSections'), len(pe.sections)+2)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('AddressOfEntryPoint'), code_rva)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'), code_rva+section_alignment)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfCode'), pe.OPTIONAL_HEADER.SizeOfCode+file_alignment)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfInitializedData'), pe.OPTIONAL_HEADER.SizeOfInitializedData+file_alignment)
struct.pack_into('<II', data, pe.OPTIONAL_HEADER.DATA_DIRECTORY[1].get_file_offset(), data_rva, 20*(descriptor_count+2))
data.extend(b'\0'*(data_raw-len(data)))
data.extend(new_data)
data.extend(new_code)
patched_pe = pefile.PE(data=data)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'), patched_pe.generate_checksum())
patched_pe = pefile.PE(data=data)
assert patched_pe.verify_checksum()
assert [x.name.decode() for x in patched_pe.DIRECTORY_ENTRY_IMPORT[-1].imports] == names
assert patched_pe.get_data(pe.sections[2].VirtualAddress, pe.sections[2].Misc_VirtualSize) == pe.get_data(pe.sections[2].VirtualAddress, pe.sections[2].Misc_VirtualSize)

output.parent.mkdir(parents=True, exist_ok=True)
with output.open('xb') as stream:
    stream.write(data)
report = {
    'original_sha256': ORIGINAL_SHA256,
    'patched_sha256': hashlib.sha256(data).hexdigest(),
    'original_entry_rva': hex(old_entry), 'patched_entry_rva': hex(code_rva),
    'companion_imports': names, 'patches': changes,
    'resources_byte_identical': True, 'pe_checksum_valid': True,
    'runtime_scaling': 'max(1, min(monitor_client_width // 640, monitor_client_height // 480))',
    'output': output.name
}
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open('x', encoding='utf-8') as stream:
        stream.write(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
