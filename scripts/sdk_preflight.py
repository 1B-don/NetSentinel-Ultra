#!/usr/bin/env python3
"""Check candidate firmware function names against a permitted local export table.

No downloads, SDK traversal, build, hardware access or compatibility certification.
"""
import argparse
import csv
import hashlib
import io
from pathlib import Path
import re
import sys

TARGET = 'd3f89dfe2ef6b01839201598e9be1590cba80322'
ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / 'builds' / 'sdk-input'
# Discover calls, not declarations. Exclude comments/string literals first.
C_TEXT = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
CALL = re.compile(r'\b((?:furi_|gui_|view_port_|canvas_|storage_)[A-Za-z0-9_]+)\s*\(')

def required_functions(root=ROOT, gui_storage_only=False):
    names = set()
    for path in sorted((root / 'src' / 'flipper').glob('*.c')):
        if gui_storage_only and path.name == 'ns_uart.c':
            continue  # NS_GUI_STORAGE_ONLY branch contains no firmware calls
        names.update(CALL.findall(C_TEXT.sub(' ', path.read_text())))
    if not names:
        raise ValueError('no firmware function calls discovered')
    return names

def read_exports(text, include_variables=False):
    reader = csv.DictReader(io.StringIO(text), strict=True)
    if reader.fieldnames != ['entry', 'status', 'name', 'type', 'params']:
        raise ValueError('unsupported export-table columns')
    exports, seen, version = set(), set(), None
    for row in reader:
        if None in row or any(value is None for value in row.values()):
            raise ValueError('malformed export row')
        kind, status, name = row['entry'], row['status'], row['name']
        if kind not in {'Version', 'Header', 'Function', 'Variable'} or status not in {'+', '-', '?'} or not name:
            raise ValueError('invalid export entry/status/name')
        key = (kind, name)
        if key in seen:
            raise ValueError('duplicate export entry')
        seen.add(key)
        if kind == 'Version':
            if version is not None or status != '+' or not re.fullmatch(r'\d+\.\d+', name):
                raise ValueError('invalid API version')
            version = name
        elif status == '+' and (kind == 'Function' or (include_variables and kind == 'Variable')):
            exports.add(name)
    if version is None:
        raise ValueError('missing API version')
    return version, exports

def read_undefined(text):
    """Parse nm --undefined-only --format=posix output, never execute nm.

    U is a required import; undefined weak w/v entries are reported separately.
    An SDK-linked intermediate ELF is needed: object files include imports which
    the SDK's private libraries may still resolve later.
    """
    required, weak = set(), set()
    for line in text.splitlines():
        fields = line.split()
        if len(fields) != 2 or fields[1] not in {'U', 'w', 'v'} or not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_.$]*', fields[0]):
            raise ValueError('unsupported undefined-symbol listing')
        symbol, kind = fields
        if symbol in required or symbol in weak:
            raise ValueError('duplicate undefined symbol')
        (required if kind == 'U' else weak).add(symbol)
    if not required and not weak:
        raise ValueError('empty undefined-symbol evidence')
    return required, weak

def permitted_input(path):
    # Do not read any firmware checkout. Resolve symlinks before opening data;
    # even a symlinked builds/sdk-input directory must remain inside this project.
    resolved = path.resolve()
    local = ROOT.resolve() / 'builds' / 'sdk-input'
    if not resolved.is_relative_to(local):
        raise ValueError('export table must be a regular local file under builds/sdk-input')
    if not resolved.is_file():
        raise ValueError('export table is absent')
    return resolved

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exports', type=Path)
    parser.add_argument('--firmware-revision', required=True,
                        help='owner-confirmed origin revision; not inferred from CSV')
    parser.add_argument('--gui-storage-only', action='store_true',
                        help='match manifest NS_GUI_STORAGE_ONLY; omit disabled UART HAL calls')
    parser.add_argument('--undefined-symbols', type=Path,
                        help='project-local POSIX nm listing from actual SDK-linked ARM ELF')
    args = parser.parse_args(argv)
    if args.firmware_revision != TARGET:
        print('BLOCK: export provenance revision does not match installed target')
        return 1
    try:
        path = permitted_input(args.exports)
        data = path.read_bytes()
        version, exports = read_exports(data.decode('utf-8'), include_variables=bool(args.undefined_symbols))
        weak = set()
        evidence = None
        if args.undefined_symbols:
            evidence = permitted_input(args.undefined_symbols).read_bytes()
            required, weak = read_undefined(evidence.decode('utf-8'))
        else:
            required = required_functions(gui_storage_only=args.gui_storage_only)
    except (ValueError, OSError, UnicodeError, csv.Error):
        print('BLOCK: invalid or unavailable local export input; no SDK was accessed')
        return 1
    print(f'API version: {version}; export-table SHA256: {hashlib.sha256(data).hexdigest()}')
    missing = sorted(required - exports)
    for name in missing:
        print(f'MISSING: {name}')
    if evidence is not None:
        print(f'ELF import-list SHA256: {hashlib.sha256(evidence).hexdigest()}')
        print(f'Required ELF imports checked: {len(required)}; undefined weak imports: {len(weak)}')
        for name in sorted(weak):
            print(f'WEAK (resolution not required by this check): {name}')
    else:
        print(f'Firmware function names checked: {len(required)}')
    print('Scope: names only; provenance is owner-supplied. Headers, signatures, enums, '
          'runtime exports, C helpers, board port and actual ARM compile/link remain unverified.')
    if missing:
        return 1
    print('PASS: checked names are enabled in supplied table; not a compatible FAP build')
    return 0

if __name__ == '__main__':
    sys.exit(main())
