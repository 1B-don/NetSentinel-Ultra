"""Synthetic export-table checks; no real SDK or radio measurements."""
import contextlib
import io
import importlib.util
import tempfile
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('sdk_preflight', Path(__file__).resolve().parents[1] / 'scripts/sdk_preflight.py')
sdk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdk)
HEADER = 'entry,status,name,type,params\nVersion,+,1.2,,\n'

class SdkPreflightTests(unittest.TestCase):
    def run_check(self, args):
        with contextlib.redirect_stdout(io.StringIO()) as output:
            result = sdk.main(args)
        return result, output.getvalue()

    def test_enabled_only(self):
        version, exports = sdk.read_exports(HEADER + 'Function,+,furi_get_tick,uint32_t,void\nFunction,-,gui_remove_view_port,void,"Gui*, ViewPort*"\nFunction,?,canvas_clear,void,Canvas*\n')
        self.assertEqual(version, '1.2')
        self.assertEqual(exports, {'furi_get_tick'})

    def test_malformed_and_duplicate(self):
        for text in ('bad,columns\n', HEADER + 'Function,+,x,void\n', HEADER + 'Function,!,x,void,void\n', HEADER + 'Function,+,x,void,void\nFunction,-,x,void,void\n', HEADER + 'Version,+,2.0,,\n', 'entry,status,name,type,params\nFunction,+,x,void,void\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                sdk.read_exports(text)

    def test_discovers_actual_adapter_calls(self):
        names = sdk.required_functions()
        self.assertIn('furi_hal_serial_async_rx_start', names)
        self.assertIn('gui_remove_view_port', names)
        self.assertIn('storage_file_sync', names)
        self.assertNotIn('ns_flipper_init', names)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'src/flipper').mkdir(parents=True)
            (root / 'src/flipper/app.c').write_text('/* furi_fake(); */ "storage_fake()"; // gui_fake()\nfuri_get_tick();')
            self.assertEqual(sdk.required_functions(root), {'furi_get_tick'})

    def test_rejects_external_and_symlinked_input(self):
        with tempfile.TemporaryDirectory() as temp:
            outside = Path(temp) / 'exports.csv'
            outside.write_text(HEADER)
            with self.assertRaises(ValueError):
                sdk.permitted_input(outside)
            sdk.INPUT.mkdir(parents=True, exist_ok=True)
            link = sdk.INPUT / 'test-symlink.csv'
            try:
                link.symlink_to(outside)
                with self.assertRaises(ValueError):
                    sdk.permitted_input(link)
            finally:
                link.unlink(missing_ok=True)

    def test_target_gate_precedes_file_access(self):
        result, output = self.run_check(['absent.csv', '--firmware-revision', 'wrong'])
        self.assertEqual(result, 1)
        self.assertIn('revision', output)

    def test_complete_and_missing_candidate_exports(self):
        sdk.INPUT.mkdir(parents=True, exist_ok=True)
        path = sdk.INPUT / 'test-synthetic.csv'
        names = sorted(sdk.required_functions())
        try:
            path.write_text(HEADER + ''.join(f'Function,+,{name},void,void\n' for name in names))
            args = [str(path), '--firmware-revision', sdk.TARGET]
            result, output = self.run_check(args)
            self.assertEqual(result, 0)
            self.assertIn('not a compatible FAP build', output)
            path.write_text(HEADER + ''.join(f'Function,+,{name},void,void\n' for name in names[1:]))
            result, output = self.run_check(args)
            self.assertEqual(result, 1)
            self.assertIn(f'MISSING: {names[0]}', output)
        finally:
            path.unlink(missing_ok=True)
