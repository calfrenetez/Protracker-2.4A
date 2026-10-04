from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import SOURCES, ROOT
class Workflow(unittest.TestCase):
    def test_integrated_controller_ownership_and_silent_navigation(self):
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp)/'workflow-test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/editor_workflow_test.c',
                            *SOURCES[1:],'-o',str(out)],cwd=ROOT,check=True)
            subprocess.run([str(out)],cwd=ROOT,check=True)
