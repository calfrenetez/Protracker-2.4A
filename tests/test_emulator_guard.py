import sys
from pathlib import Path
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from emulator_ipc import Emulator


class EmulatorGuard(unittest.TestCase):
    def test_exact_profile_required(self):
        with patch.object(Emulator, 'command', return_value='OK\tConfig=ProTracker isolated baseline'):
            Emulator('test.sock')
        for status in ['OK\tConfig=Another project',
                       'OK\tConfig=ProTracker isolated baseline backup',
                       'OK\tOther=Config=ProTracker isolated baseline']:
            with patch.object(Emulator, 'command', return_value=status):
                with self.assertRaises(RuntimeError):
                    Emulator('test.sock')
