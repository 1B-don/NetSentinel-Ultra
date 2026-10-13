"""Synthetic ELF metadata mutations; real ARM import check is a build-only tool."""
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'scripts'))
from validate_fap import metadata, EXPECTED_ICON

def elf():
    b = bytearray(320)
    b[:7] = b'\x7fELF\x01\x01\x01'
    struct.pack_into('<HH', b, 16, 1, 40)
    struct.pack_into('<I', b, 32, 52)
    struct.pack_into('<I', b, 36, 0x05000000)
    struct.pack_into('<HHH', b, 46, 40, 3, 1)
    names=b'\0.names\0.fapmeta\0'; b[172:172+len(names)]=names
    struct.pack_into('<10I', b, 92, 1,3,0,0,172,len(names),0,0,1,0)
    struct.pack_into('<10I', b, 132, 8,1,0,0,200,86,0,0,1,0)
    struct.pack_into('<IIHHHH', b, 200, 0x52474448,1,1,87,7,8192)
    struct.pack_into('<I',b,216,0x00010000)
    name=b'NetSentinel Ultra Dev'
    b[220:220+len(name)]=name
    return b

class FapMetadataTests(unittest.TestCase):
    def test_esp_status_metadata(self):
        b=elf(); b[220:252]=b'NetSentinel ESP Status'.ljust(32,b'\0')
        b[252]=1; b[253:285]=EXPECTED_ICON
        self.assertEqual(metadata(b)['name'],'NetSentinel ESP Status')
    def test_expected(self):
        self.assertEqual(metadata(elf()),dict(api='87.1',target=7,stack=8192,app_version='1.0 development',name='NetSentinel Ultra Dev',icon=False,flags=0))
    def test_bundled_icon(self):
        b=elf(); b[252]=1; b[253:285]=EXPECTED_ICON
        self.assertTrue(metadata(b)["icon"])
        for offset in (252, 253, 270, 284, 285):
            changed=bytearray(b); changed[offset] ^= 2
            with self.assertRaises(ValueError): metadata(changed)

    def test_candidate_metadata(self):
        b=elf(); b[220:252]=b'NetSentinel Ultra v1.0 RC1'.ljust(32,b'\0')
        b[252]=1; b[253:285]=EXPECTED_ICON
        result=metadata(b)
        self.assertEqual(result['name'],'NetSentinel Ultra v1.0 RC1')
        self.assertTrue(result['icon'])
        self.assertEqual((result['api'],result['target'],result['flags']),('87.1',7,0))

    def test_versions_and_identity(self):
        for offset,value in [(0,0),(4,2),(8,2),(10,89),(12,6),(14,4096)]:
            b=elf()
            struct.pack_into('<I' if offset<8 else '<H',b,200+offset,value)
            with self.assertRaises(ValueError): metadata(b)
    def test_truncation_and_machine(self):
        for length in (0,6,51,171,190,215):
            with self.assertRaises(ValueError): metadata(elf()[:length])
        for offset,value in [(16,2),(18,62),(46,32),(48,0),(50,3)]:
            b=elf();struct.pack_into('<H',b,offset,value)
            with self.assertRaises(ValueError): metadata(b)
    def test_extra_manifest_fields(self):
        for offset in (216,220,252,253,285):
            b=elf();b[offset]=255
            with self.assertRaises(ValueError): metadata(b)
    def test_section_bounds(self):
        for offset in (32,108,112,148,152):
            b=elf();struct.pack_into('<I',b,offset,0xffffffff)
            with self.assertRaises(ValueError): metadata(b)

if __name__=='__main__': unittest.main()
