from pathlib import Path
import importlib.util
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Filter(unittest.TestCase):
    def test_spectrum_precision_and_boundaries(self):
        spec=importlib.util.spec_from_file_location('sinc',ROOT/'tools/generate_sinc_kernel.py')
        generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
        self.assertEqual((ROOT/'src/core/sinc_kernel.h').read_text(),generator.table())
        with tempfile.TemporaryDirectory() as tmp:
            binary=str(Path(tmp)/'filter')
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core','tests/filter_test.c','src/core/pcm_filtered.c','src/core/pcm.c','-o',binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True)
