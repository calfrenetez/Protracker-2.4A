from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tools/pt24g_convert.c', 'src/platform/file_save.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/safe_save.c',
           'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c',
           'src/core/channels.c', 'src/core/pcm.c']
class ConvertCLI(unittest.TestCase):
    def test_new_file_publication_round_trip_and_existing_destination(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp); binary = str(d / 'PT24GConvert')
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-Isrc/core', *SOURCES, '-o', binary], cwd=ROOT, check=True)
            source = d / 'source.mod'; source.write_bytes((ROOT / 'evidence/baseline/mod.baseline').read_bytes())
            project = d / 'song.ptg'; restored = d / 'restored.mod'
            subprocess.run([binary, 'project', str(source), str(project)], check=True, capture_output=True)
            subprocess.run([binary, 'inspect', str(project)], check=True, capture_output=True)
            subprocess.run([binary, 'mod', str(project), str(restored)], check=True, capture_output=True)
            self.assertEqual(source.read_bytes(), restored.read_bytes())
            previous = project.read_bytes()
            result = subprocess.run([binary, 'project', str(source), str(project)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 20)
            self.assertEqual(previous, project.read_bytes())
            result = subprocess.run([binary, 'mod', str(project), str(source)], capture_output=True)
            self.assertEqual(result.returncode, 20)
            self.assertEqual(source.read_bytes(), restored.read_bytes())
            damaged = bytearray(previous); damaged[-1] ^= 1; project.write_bytes(damaged)
            result = subprocess.run([binary, 'mod', str(project), str(d / 'invalid.mod')], capture_output=True)
            self.assertEqual(result.returncode, 20)
            self.assertFalse((d / 'invalid.mod').exists())
            self.assertFalse(list(d.glob('*.pttmp-*')))
