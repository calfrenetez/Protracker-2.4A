from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ['tests/event_resource_test.c', 'src/core/event_resource.c',
           'src/core/flow.c', 'src/core/pitch.c', 'src/core/project.c',
           'src/core/channels.c', 'src/core/pcm.c']


class EventResource(unittest.TestCase):
    def test_bounded_engine_inheritance_and_navigation_refusal(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'event-resource'
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-fsanitize=address,undefined',
                            '-DPT_EVENT_RESOURCE_PROFILE=1', '-Isrc/core',
                            *SOURCES, '-o', str(binary)], cwd=ROOT, check=True)
            result = subprocess.run([str(binary)], check=False, capture_output=True,
                                    text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('EVENT RESOURCE PASS:', result.stdout)
