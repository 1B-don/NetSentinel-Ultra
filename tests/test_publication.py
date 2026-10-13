"""Publication policy uses invented test content in disposable local Git repos."""
import subprocess
import tempfile
import unittest
from pathlib import Path

AUDIT = Path(__file__).resolve().parents[1] / 'scripts/publication_audit.py'

class PublicationAudit(unittest.TestCase):
    def run_case(self, name, content, success):
        with tempfile.TemporaryDirectory(prefix='netsentinel-audit-') as directory:
            root = Path(directory)
            subprocess.run(['git', 'init', '-q', directory], check=True)
            file = root / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(content)
            subprocess.run(['git', '-C', directory, 'add', '--', name], check=True)
            result = subprocess.run(['python3', str(AUDIT)], cwd=root, capture_output=True)
            self.assertEqual(result.returncode, 0 if success else 1, result.stdout.decode())
            if not success:
                self.assertNotIn(content, result.stdout)  # never echo suspect data

    def test_clean_source(self):
        self.run_case('source.c', b'int app(void) { return 0; }', True)

    def test_excluded_files(self):
        for name in ['logs/trace.txt', '.env', 'radio.pcap', 'build/app.fap', 'private.key']:
            with self.subTest(name=name):
                self.run_case(name, b'invented restricted file content', False)

    def test_sensitive_content(self):
        samples = [b'ghp_' + b'A'*36, b'https://' + b'example:invented@host.invalid',
                   b'-----BEGIN ' + b'PRIVATE KEY-----', b'/' + b'home/invented/project/file']
        for content in samples:
            with self.subTest():
                self.run_case('config.txt', content, False)

    def test_reviewed_artwork(self):
        root = AUDIT.parents[1]
        for name in ("netsentinel_icon.png", "netsentinel_icon_source.png"):
            content = (root/"assets"/name).read_bytes()
            self.run_case("assets/"+name, content, True)
            self.run_case("assets/"+name, content+b"unexpected metadata", False)
        self.run_case("assets/unreviewed.png", b"unknown\x00image", False)

    def test_binary(self):
        self.run_case('payload.dat', b'invented\x00binary', False)

if __name__ == '__main__':
    unittest.main()
