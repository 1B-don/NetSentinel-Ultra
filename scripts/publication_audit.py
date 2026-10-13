#!/usr/bin/env python3
"""Conservative staged/current-tree audit. Complements manual privacy review."""
import re
import subprocess
import sys
from pathlib import PurePosixPath

FORBIDDEN_DIRS = {'logs', 'build', 'builds', 'dist', '.venv', '__pycache__', '.pytest_cache'}
FORBIDDEN_SUFFIXES = {'.fap', '.elf', '.o', '.bin', '.pcap', '.pcapng', '.csv', '.log', '.pem', '.key'}
PATTERNS = [
    re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
    re.compile(rb'\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[A-Z0-9]{16})\b'),
    re.compile(rb'https?://[^\s/:]+:[^\s/@]+@'),
    re.compile(rb'/' + rb'home/[^/\s]+/|/' + rb'Users/[^/\s]+/'),
]

def git(*args):
    return subprocess.check_output(['git', *args])

def main():
    problems = []
    # Audit entire index, not just the most recent diff. Never print matched data.
    for name in git('ls-files', '-z').split(b'\0'):
        if not name:
            continue
        path = name.decode()
        p = PurePosixPath(path)
        if any(part in FORBIDDEN_DIRS for part in p.parts) or p.suffix.lower() in FORBIDDEN_SUFFIXES or p.name.startswith('.env'):
            problems.append((path, 'excluded artifact/private path'))
            continue
        try:
            content = git('show', ':'+path)
        except subprocess.CalledProcessError:
            problems.append((path, 'unreadable staged file'))
            continue
        if b'\0' in content:
            problems.append((path, 'binary file requires review'))
        if any(pattern.search(content) for pattern in PATTERNS):
            problems.append((path, 'possible credential or personal workspace path'))
    for path, reason in problems:
        print(f'BLOCK: {path}: {reason}')
    if problems:
        return 1
    print('PASS: publication tree excludes logs/captures/build outputs and common sensitive patterns; manual review still required')
    return 0

if __name__ == '__main__':
    sys.exit(main())
