"""Build contracts only; never invokes a SDK, device or network operation."""
import ast
from pathlib import Path
import struct
import sys
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'scripts'))
from build_esp_info import VARIANTS, FEATURE_DEFINES, check_protected_output, source_paths, variant_manifest


class BuildContracts(unittest.TestCase):
    def test_owner_status_filename_frozen(self):
        with self.assertRaises(RuntimeError):
            check_protected_output(Path('netsentinel_esp_status.fap'))
        check_protected_output(Path('netsentinel_esp_stable.fap'))
    def test_all_variant_manifests(self):
        original = (ROOT/'application.fam').read_text()
        for variant, (appid, title, gate) in VARIANTS.items():
            generated = variant_manifest(original, variant)
            call = ast.parse(generated).body[0].value
            fields = {item.arg: item.value for item in call.keywords}
            self.assertEqual(ast.literal_eval(fields['appid']), appid)
            self.assertEqual(ast.literal_eval(fields['name']), title)
            self.assertEqual(ast.literal_eval(fields['cdefines']), [gate, *FEATURE_DEFINES.get(variant, [])])
            self.assertFalse(any('NS_TEST_' in d or 'PASSIVE_START_VERIFIED' in d for d in ast.literal_eval(fields['cdefines'])))
            self.assertEqual(ast.literal_eval(fields['entry_point']), 'netsentinel_app')
            self.assertEqual(ast.literal_eval(fields['fap_icon']), 'assets/netsentinel_icon.png')
            self.assertEqual(ast.literal_eval(fields['fap_version']), '1.0')
            self.assertEqual(ast.literal_eval(fields['sources']),
                             ['ap_parser.c', 'ns_analysis.c', 'src/core/*.c', 'src/flipper/*.c'])
        self.assertEqual((ROOT/'application.fam').read_text(), original)

    def test_changed_baseline_rejected(self):
        original = (ROOT/'application.fam').read_text()
        for old, new in [('NS_GUI_STORAGE_ONLY', 'NS_ENABLE_BOARD_PORT'),
                         ('assets/netsentinel_icon.png', 'missing.png'),
                         ('appid="netsentinel"', 'appid="other"'),
                         ('name="NetSentinel Ultra"', 'name="other"')]:
            with self.assertRaises(RuntimeError):
                variant_manifest(original.replace(old, new), 'rc')

    def test_physical_board_gate_requires_evidence_or_explicit_host_mock(self):
        header = (ROOT/'src/flipper/ns_flipper.h').read_text()
        self.assertIn('NS_TEST_BOARD_PORT', header)
        self.assertIn('NS_VERIFIED_ULTRA_V3_NRF24_ROUTE', header)
        self.assertIn('NS_VERIFIED_ULTRA_V3_CC1101_ROUTE', header)
        self.assertIn('NS_VERIFIED_MOMENTUM_CC1101_PROVIDER', header)
        for _, (_, _, gate) in VARIANTS.items():
            self.assertNotEqual(gate, 'NS_ENABLE_BOARD_PORT')
        manifest = (ROOT/'application.fam').read_text()
        self.assertIn('cdefines=["NS_GUI_STORAGE_ONLY"]', manifest)
        self.assertNotIn('NS_ENABLE_BOARD_PORT', manifest)

    def test_complete_sources_and_icon(self):
        sources = source_paths()
        self.assertEqual(len(sources), len(set(sources)))
        self.assertTrue(all(p.is_file() and not p.is_symlink() for p in sources))
        expected = set((ROOT/'src').glob('core/*.c')) | set((ROOT/'src').glob('flipper/*.c'))
        expected |= {ROOT/'ap_parser.c', ROOT/'ns_analysis.c'}
        self.assertEqual({p for p in sources if p.suffix == '.c'}, expected)
        data = (ROOT/'assets/netsentinel_icon.png').read_bytes()
        self.assertIn(ROOT/'assets/netsentinel_icon.png', sources)
        self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
        self.assertEqual(struct.unpack_from('>IIBB', data, 16), (10, 10, 1, 0))
        offset = 8; chunks = []
        while offset < len(data):
            length = struct.unpack_from('>I', data, offset)[0]
            end = offset+12+length
            self.assertLessEqual(end, len(data))
            chunk = data[offset+4:offset+8+length]
            self.assertEqual(zlib.crc32(chunk), struct.unpack_from('>I', data, end-4)[0])
            chunks.append(chunk[:4]); offset = end
        self.assertEqual(chunks, [b'IHDR', b'IDAT', b'IEND'])


if __name__ == '__main__':
    unittest.main()
