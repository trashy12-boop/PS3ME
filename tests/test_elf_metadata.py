#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Regress the real missing-process and missing-postlink startup defects."""
from pathlib import Path
import struct, sys, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from validate_elf import validate

class Metadata(unittest.TestCase):
    def setUp(self):
        self.data=bytearray((ROOT/'build/ps3/release_native.elf').read_bytes())
        self.temp=tempfile.TemporaryDirectory();self.path=Path(self.temp.name)/'app.elf'
    def tearDown(self):self.temp.cleanup()
    def check_bad(self):
        self.path.write_bytes(self.data)
        with self.assertRaises(AssertionError):validate(self.path)
    def test_current(self):
        self.path.write_bytes(self.data);self.assertEqual(validate(self.path)['status'],'passed')
    def test_missing_lv2_abi(self):
        self.data[7]=0;self.check_bad()
    def test_missing_process_parameters(self):
        phoff=struct.unpack_from('>Q',self.data,32)[0];size,count=struct.unpack_from('>HH',self.data,54)
        for i in range(count):
            offset=phoff+i*size
            if struct.unpack_from('>I',self.data,offset)[0]==0x60000001:
                struct.pack_into('>Q',self.data,offset+32,0);break
        self.check_bad()
    def test_unfixed_function_descriptor(self):
        off=struct.unpack_from('>Q',self.data,40)[0];size,count,strings=struct.unpack_from('>HHH',self.data,58)
        sections=[struct.unpack_from('>IIQQQQIIQQ',self.data,off+i*size) for i in range(count)]
        s=sections[strings];names=self.data[s[4]:s[4]+s[5]]
        for s in sections:
            if names[s[0]:].split(b'\0',1)[0]==b'.opd':
                self.data[s[4]+16:s[4]+24]=bytes(8);break
        self.check_bad()

if __name__=='__main__':unittest.main()
