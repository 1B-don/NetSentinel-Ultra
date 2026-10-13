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
        minimal = sdk.required_functions(gui_storage_only=True)
        self.assertIn('storage_file_sync', minimal)
        self.assertIn('gui_remove_view_port', minimal)
        self.assertFalse(any(name.startswith('furi_hal_serial_') for name in minimal))
        self.assertTrue(minimal < names)
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

    def test_registry_and_plugin_calls_are_inventoried(self):
        # Synthetic source only; never initializes a real registry/provider.
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'src/flipper').mkdir(parents=True)
            (root / 'src/flipper/app.c').write_text(
                '/* subghz_devices_begin(device); */\n'
                '"subghz_devices_set_rx(device)";\n'
                'subghz_devices_init();\n'
                'subghz_devices_get_by_name("cc1101_ext");\n'
                'subghz_devices_get_name(device);\n'
                'subghz_devices_deinit();\n'
                'plugin_manager_load_single(manager, path);\n')
            self.assertEqual(sdk.required_functions(root), {
                'subghz_devices_init', 'subghz_devices_get_by_name',
                'subghz_devices_get_name', 'subghz_devices_deinit',
                'plugin_manager_load_single'})
        self.assertFalse(any(name.startswith(('subghz_', 'plugin_manager_'))
                             for name in sdk.required_functions()))

    def test_import_listing(self):
        required, weak = sdk.read_undefined('__aeabi_uldivmod U\nfuri_hal_spi_bus_handle_external U\noptional_hook w\noptional_data v\n')
        self.assertEqual(required, {'__aeabi_uldivmod', 'furi_hal_spi_bus_handle_external'})
        self.assertEqual(weak, {'optional_hook', 'optional_data'})
        for text in ('', 'symbol T', 'file.o: symbol U', 'symbol U 0', 'x U\nx w', 'x U\n\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                sdk.read_undefined(text)

    def test_import_check_includes_variables(self):
        text = HEADER + 'Variable,+,furi_hal_spi_bus_handle_external,const void*,\nFunction,+,__aeabi_uldivmod,void,void\n'
        _, functions = sdk.read_exports(text)
        _, imports = sdk.read_exports(text, include_variables=True)
        self.assertEqual(functions, {'__aeabi_uldivmod'})
        self.assertEqual(imports, {'__aeabi_uldivmod', 'furi_hal_spi_bus_handle_external'})
        sdk.INPUT.mkdir(parents=True, exist_ok=True)
        table = sdk.INPUT / 'test-imports.csv'
        listing = sdk.INPUT / 'test-imports.txt'
        try:
            table.write_text(text)
            listing.write_text('furi_hal_spi_bus_handle_external U\n__aeabi_uldivmod U\noptional_hook w\n')
            args = [str(table), '--firmware-revision', sdk.TARGET, '--undefined-symbols', str(listing)]
            result, output = self.run_check(args)
            self.assertEqual(result, 0)
            self.assertIn('Required ELF imports checked: 2', output)
            self.assertIn('ELF import-list SHA256:', output)
            self.assertIn('WEAK', output)
            listing.write_text('missing_runtime U\n')
            result, output = self.run_check(args)
            self.assertEqual(result, 1)
            self.assertIn('MISSING: missing_runtime', output)
            listing.write_text('malformed evidence')
            self.assertEqual(self.run_check(args)[0], 1)
        finally:
            table.unlink(missing_ok=True)
            listing.unlink(missing_ok=True)

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
