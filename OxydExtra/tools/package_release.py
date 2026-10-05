"""Package the verified portable payload and standalone usage documentation."""
import argparse
import hashlib
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = ('OxydEx.exe', 'oxscale.dll', 'dx7vb.dll', 'OxydEx.exe.manifest')
DOCS = ('README.md', 'LICENSE', 'docs/TROUBLESHOOTING.md',
        'docs/THIRD_PARTY.md', 'docs/VERIFICATION.md',
        'releases/oxydextra-v0.1.0.md')

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=ROOT / 'dist')
args = parser.parse_args()
expected = {}
for line in (ROOT / 'portable' / 'SHA256SUMS.txt').read_text(encoding='utf-8').splitlines():
    digest, name = line.split('  ', 1)
    expected[name] = digest
if set(expected) != set(PAYLOAD):
    raise SystemExit('Payload checksum list must contain exactly the four release files.')
entries = {}
for name in PAYLOAD:
    data = (ROOT / 'portable' / name).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected[name]:
        raise SystemExit(f'Payload hash mismatch: {name}; no archive written.')
    entries[name] = data
entries['SHA256SUMS.txt'] = (ROOT / 'portable' / 'SHA256SUMS.txt').read_text(encoding='utf-8').encode('utf-8')
for name in DOCS:
    entries[name] = (ROOT / name).read_bytes()
# The portable copy is standalone. Its source links point to the release tag.
base = 'https://github.com/Buyukcaglar/Oxyd/tree/oxydextra-v0.1.0/OxydExtra/'
readme = entries['README.md'].decode('utf-8')
for target in ('src/', 'tools/', 'tests/', 'portable/', 'evidence/',
               'docs/BUILD.md', 'docs/ARCHITECTURE.md'):
    readme = readme.replace(f']({target})', f']({base}{target})')
entries['README.md'] = readme.encode('utf-8')
args.output_dir.mkdir(parents=True, exist_ok=True)
archive = args.output_dir / 'OxydExtra-native-desktop-v0.1.0.zip'
# Fixed archive timestamps and order make repeated packaging deterministic.
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as output:
    for name, data in sorted(entries.items()):
        info = zipfile.ZipInfo(name, date_time=(2026, 10, 5, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        output.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
sums = args.output_dir / 'SHA256SUMS.txt'
sums.write_text(f'{digest}  {archive.name}\n', encoding='utf-8')
print(f'{digest}  {archive.name} ({archive.stat().st_size:,} bytes)')
