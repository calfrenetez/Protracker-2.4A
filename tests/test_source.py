from pathlib import Path
import importlib.util
import subprocess
import tempfile
import unittest
from test_editor import SOURCES
ROOT=Path(__file__).resolve().parents[1]
class Source(unittest.TestCase):
    def test_preview_and_atomic_import(self):
        spec=importlib.util.spec_from_file_location('donor',ROOT/'tools/make_mod_sample_fixture.py');maker=importlib.util.module_from_spec(spec);spec.loader.exec_module(maker)
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);binary=tmp/'source';donor=tmp/'donor.mod';donor.write_bytes(maker.make((ROOT/'evidence/baseline/mod.baseline').read_bytes()))
            sources=['tests/source_test.c',*[p for p in SOURCES if p not in ('tests/editor_test.c','src/editor/view.c')]]
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*sources,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(donor)],check=True)
