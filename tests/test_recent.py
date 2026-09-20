from pathlib import Path
import subprocess,tempfile,unittest,zlib
ROOT=Path(__file__).resolve().parents[1]
class Recent(unittest.TestCase):
    def test_preferences_recovery(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'recent-file'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/recent_file_test.c','src/platform/recent_file.c','src/core/recent.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(Path(tmp)/'prefs')],check=True)
    def test_history_and_codec(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'recent';record=Path(tmp)/'prefs'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/recent_test.c','src/core/recent.c','-o',str(exe)],cwd=ROOT,check=True)
            subprocess.run([str(exe),str(record)],check=True)
            data=bytearray(record.read_bytes());checksum=int.from_bytes(data[12:16],'big');data[12:16]=bytes(4)
            self.assertEqual(zlib.crc32(data),checksum)
            self.assertEqual(data[:8],b'PTRC\0\1\0\x0a');self.assertEqual(int.from_bytes(data[8:12],'big'),len(data))
