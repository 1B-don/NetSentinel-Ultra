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

def metadata(data):
    if len(data) < 52 or data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('expected little-endian ELF32')
    kind, machine = struct.unpack_from('<HH', data, 16)
    if kind != 1 or machine != 40:
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
    if len(matches) != 1 or len(matches[0]) < 16:
        raise ValueError('missing/duplicate FAP metadata')
    magic, version, minor, major, target, stack = struct.unpack_from('<IIHHHH', matches[0])
    if magic != 0x52474448 or version != 1 or (major, minor, target) != (87, 1, 7) or stack != 8192:
        raise ValueError('unexpected FAP API/target/stack metadata')
    return {'api': f'{major}.{minor}', 'target': target, 'stack': stack}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('artifact', type=Path)
    args = parser.parse_args()
    artifact = args.artifact.resolve()
    if not artifact.is_relative_to(ROOT/'dist') or not artifact.is_file():
        raise ValueError('artifact must be a project-local dist file')
    data = artifact.read_bytes(); result = metadata(data)
    table = sdk_preflight.permitted_input(ROOT/'builds/sdk-input/api_symbols.csv').read_bytes()
    if hashlib.sha256(table).hexdigest() != 'a2bdb9893ba57ac6ccc4eae2e66ab201fa84f48508347f4c52685cfb3c3a8d4f':
        raise ValueError('unexpected permitted SDK export-table checksum')
    api, exports = sdk_preflight.read_exports(table.decode(), include_variables=True)
    if api != '87.1':
        raise ValueError('SDK export API mismatch')
    nm = Path.home()/'.ufbt/toolchain/x86_64-linux/bin/arm-none-eabi-nm'
    listing = subprocess.check_output([str(nm), '--undefined-only', '--format=posix', str(artifact)], text=True)
    required, weak = sdk_preflight.read_undefined(listing)
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
