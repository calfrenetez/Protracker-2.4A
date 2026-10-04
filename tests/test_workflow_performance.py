from pathlib import Path
import subprocess
import tempfile
import unittest
from test_editor import SOURCES, ROOT
class WorkflowPerformance(unittest.TestCase):
    def test_long_stereo_master_observations(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'workflow-performance-test'
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-Isrc/core','tests/workflow_performance_test.c',
                            *SOURCES[1:],'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],cwd=ROOT,check=True)
