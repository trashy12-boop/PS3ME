#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run the production MIDP ZIP parser/inflater on LP64 and reject bad CRCs."""
from pathlib import Path
import subprocess,tempfile,unittest,zipfile,struct
ROOT=Path(__file__).resolve().parents[1]
class JarReaderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();cls.path=Path(cls.tmp.name)
        src=ROOT/'deps/phoneME/midp/src'
        cls.exe=cls.path/'reader'
        subprocess.run(['gcc','-std=gnu99','-O2',
            '-I'+str(src/'core/jarutil/include'),'-I'+str(src/'ams/ams_base/include'),
            str(ROOT/'tests/jar_reader_driver.c'),str(src/'core/jarutil/reference/native/jar.c'),
            str(src/'ams/ams_base/reference/native/midpInflate.c'),'-o',str(cls.exe)],check=True)
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_stored_and_deflated_entries(self):
        for method in (zipfile.ZIP_STORED,zipfile.ZIP_DEFLATED):
            p=self.path/('test%d.jar'%method)
            values={'META-INF/MANIFEST.MF':b'MIDlet-Name: CRC regression\r\n',
                    'all-bytes.bin':bytes(range(256))*257,'empty':b''}
            with zipfile.ZipFile(p,'w',compression=method) as z:
                for name,data in values.items():z.writestr(name,data)
            for name,data in values.items():
                got=subprocess.run([self.exe,p,name],capture_output=True)
                self.assertEqual(got.returncode,0,got.stderr)
                self.assertEqual(got.stdout,data)
    def test_diagnostic_jar(self):
        p=ROOT/'build/pkgroot/USRDIR/games/PS3ME_Diagnostic.jar'
        with zipfile.ZipFile(p) as z:
            self.assertIsNone(z.testzip())
            for name in z.namelist():
                if name.endswith('/'):continue
                got=subprocess.run([self.exe,p,name],capture_output=True)
                self.assertEqual(got.returncode,0,got.stderr)
                self.assertEqual(got.stdout,z.read(name))
    def test_crc_corruption_rejected(self):
        p=self.path/'corrupt.jar'
        with zipfile.ZipFile(p,'w') as z:z.writestr('entry',b'123456789')
        b=bytearray(p.read_bytes());b[b.index(b'123456789')]=ord('X');p.write_bytes(b)
        got=subprocess.run([self.exe,p,'entry'],capture_output=True)
        self.assertEqual(got.returncode,6)
        self.assertEqual(got.stdout,b'')
if __name__=='__main__':unittest.main()
