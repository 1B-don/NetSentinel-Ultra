#!/usr/bin/env python3
"""Read-only strict ARM FAP checks against the permitted local export table."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sdk_preflight
ROOT = Path(__file__).resolve().parents[1]
# Reviewed SDK file2image output for the bundled 10x10 one-bit PNG.
EXPECTED_ICON = bytes.fromhex("00fc000201310249022102710212012a01c4007800").ljust(32, b"\0")

def metadata(data):
    if len(data) < 52 or data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('expected little-endian ELF32')
    kind, machine = struct.unpack_from('<HH', data, 16)
    if kind != 1 or machine != 40 or struct.unpack_from('<I', data, 36)[0] != 0x05000000:
        raise ValueError('expected relocatable ARM ELF')
    offset = struct.unpack_from('<I', data, 32)[0]
    width, count, names_index = struct.unpack_from('<HHH', data, 46)
    if width != 40 or not count or names_index >= count or offset+count*width > len(data):
        raise ValueError('invalid ELF section table')
    sections = [struct.unpack_from('<10I', data, offset+i*width) for i in range(count)]
    def contents(section):
        start, length = section[4:6]
        if start+length > len(data):
            raise ValueError('truncated section')
        return data[start:start+length]
    names = contents(sections[names_index]); matches = []
    for section in sections:
        start = section[0]
        if start >= len(names) or b'\0' not in names[start:]:
            raise ValueError('invalid section name')
        if names[start:].split(b'\0', 1)[0] == b'.fapmeta':
            matches.append(contents(section))
    if len(matches) != 1 or len(matches[0]) != 86:
        raise ValueError('missing/duplicate FAP metadata')
    magic, version, minor, major, target, stack = struct.unpack_from('<IIHHHH', matches[0])
    if magic != 0x52474448 or version != 1 or (major, minor, target) != (87, 1, 7) or stack != 8192:
        raise ValueError('unexpected FAP API/target/stack metadata')
    manifest = matches[0]
    app_version = struct.unpack_from('<I', manifest, 16)[0]
    name = manifest[20:52]
    if b'\0' not in name:
        raise ValueError('unterminated application name')
    raw_name, padding = name.split(b'\0', 1)
    allowed = (b'NetSentinel Ultra', b'NetSentinel ESP Info', b'ESP Diagnostics v0.2', b'NetSentinel Ultra Dev', b'NetSentinel Ultra v1.0 RC1')
    if raw_name not in allowed or any(padding) or app_version != 0x00010000 or manifest[85] != 0:
        raise ValueError('unexpected name/app version/icon/flags metadata')
    if manifest[52] not in (0, 1) or manifest[53:85] != (EXPECTED_ICON if manifest[52] else bytes(32)):
        raise ValueError('unexpected name/app version/icon/flags metadata')
    return {'api': f'{major}.{minor}', 'target': target, 'stack': stack,
            'app_version': '1.0 development', 'name': raw_name.decode('ascii'), 'icon': bool(manifest[52]), 'flags': 0}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('artifact', type=Path)
    parser.add_argument('--require-icon', action='store_true', help='reject legacy no-icon metadata for new builds')
    args = parser.parse_args()
    artifact = args.artifact.resolve()
    if not artifact.is_relative_to(ROOT/'dist') or not artifact.is_file():
        raise ValueError('artifact must be a project-local dist file')
    data = artifact.read_bytes(); result = metadata(data)
    if args.require_icon and not result['icon']:
        raise ValueError('new application build must contain the approved icon')
    if artifact.name == 'netsentinel_ultra_v1_0_rc1.fap' and \
       (result['name'] != 'NetSentinel Ultra v1.0 RC1' or not result['icon']):
        raise ValueError('release candidate identity/icon mismatch')
    table = sdk_preflight.permitted_input(ROOT/'builds/sdk-input/api_symbols.csv').read_bytes()
    if hashlib.sha256(table).hexdigest() != 'a2bdb9893ba57ac6ccc4eae2e66ab201fa84f48508347f4c52685cfb3c3a8d4f':
        raise ValueError('unexpected permitted SDK export-table checksum')
    api, exports = sdk_preflight.read_exports(table.decode(), include_variables=True)
    if api != '87.1':
        raise ValueError('SDK export API mismatch')
    nm = Path.home()/'.ufbt/toolchain/x86_64-linux/bin/arm-none-eabi-nm'
    listing = subprocess.check_output([str(nm), '--undefined-only', '--format=posix', str(artifact)], text=True)
    required, weak = sdk_preflight.read_undefined(listing)
    defined = subprocess.check_output([str(nm), '--defined-only', '--format=posix', str(artifact)], text=True)
    entry = [line.split() for line in defined.splitlines() if line.split() and line.split()[0] == 'netsentinel_app']
    if len(entry) != 1 or len(entry[0]) != 4 or entry[0][1] != 'T' or \
       (int(entry[0][2], 16) | 1) != struct.unpack_from('<I', data, 24)[0]:
        raise ValueError('entry point does not match netsentinel_app Thumb function')
    if weak or required-exports:
        raise ValueError('unresolved or weak firmware imports')
    blocked = ('furi_hal_spi', 'furi_hal_subghz', 'furi_hal_power', 'furi_hal_gpio', 'furi_hal_usb', 'subghz_devices_')
    if any(name.startswith(blocked) or name == 'furi_hal_serial_tx_wait_complete' for name in required):
        raise ValueError('unexpected radio/power/USB or unbounded wait import')
    result.update(artifact=artifact.name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                  import_count=len(required), imports=sorted(required), physical_test=False)
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
