from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/project_validation_test.c', 'src/core/project.c',
           'src/core/channels.c', 'src/core/pcm.c']


class ProjectValidation(unittest.TestCase):
    def test_bounded_semantic_parity_and_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'project-validation-test'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-fsanitize=address,undefined', '-Isrc/core',
                            *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30)
