import asyncio
from pathlib import Path
import sys
import unittest
from unittest.mock import AsyncMock, Mock
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import shared_infra_timing_physical as timing


class PhysicalGuards(unittest.TestCase):
    def run_case(self, *, promote=None, execute=None, finish=None, connect=None):
        self.connect = connect or AsyncMock()
        self.execute = execute or AsyncMock()
        self.finish = finish or AsyncMock()
        self.restored = Mock()
        return timing.coordinated_run({'test_cases': []}, Path('/unused'), promote or Mock(),
                                      self.connect, self.execute, self.finish, self.restored)

    def test_promotion_failure_never_selects_or_executes(self):
        with self.assertRaisesRegex(RuntimeError, 'refused'):
            asyncio.run(self.run_case(promote=Mock(side_effect=RuntimeError('refused'))))
        self.connect.assert_not_called()
        self.execute.assert_not_called()
        self.finish.assert_not_called()

    def test_execution_failure_retains_files_and_restores(self):
        with self.assertRaisesRegex(RuntimeError, 'timeout'):
            asyncio.run(self.run_case(execute=AsyncMock(side_effect=RuntimeError('timeout'))))
        self.assertEqual([c.args[0] for c in self.connect.call_args_list], ['real-a1200', 'amiberry-030'])
        self.finish.assert_not_called()
        self.restored.assert_called_once()

    def test_selection_failure_still_restores(self):
        connect = AsyncMock(side_effect=[RuntimeError('offline'), None])
        with self.assertRaisesRegex(RuntimeError, 'offline'):
            asyncio.run(self.run_case(connect=connect))
        self.execute.assert_not_called()
        self.restored.assert_called_once()

    def test_cleanup_failure_still_restores(self):
        with self.assertRaisesRegex(RuntimeError, 'cleanup'):
            asyncio.run(self.run_case(finish=AsyncMock(side_effect=RuntimeError('cleanup'))))
        self.restored.assert_called_once()

    def test_success_finishes_then_restores(self):
        asyncio.run(self.run_case())
        self.finish.assert_awaited_once()
        self.restored.assert_called_once()
        self.assertEqual(self.connect.call_args_list[-1].args, ('amiberry-030',))

    def test_cleanup_cannot_expand_beyond_exact_owned_paths(self):
        text = timing.cleanup_script('20260927T120335293381Z')
        self.assertNotIn(' ALL', text)
        self.assertEqual(text.count('Delete '), 4)
        with self.assertRaises(ValueError):
            timing.cleanup_script('other\nDelete SYS: ALL')
        record = {'passed': True, 'tested_artifacts': {timing.BINARY: 'hash'}, 'completed_cases': [timing.BINARY]}
        markers = ['NATIVE ECLOCK PASS:', 'NATIVE ALARM PASS:', 'NATIVE SIGNAL PASS:',
                   'NATIVE SONG GATE PASS:', 'NATIVE COST PASS:', 'PROJECT SNAPSHOT PASS:',
                   'zero owned bytes, budget refusal without Chip fallback', timing.EXPECTED]
        log = '\n'.join(markers)
        timing.validate_completion(record, log, 'hash')
        for marker in markers:
            with self.subTest(marker=marker), self.assertRaises(RuntimeError):
                timing.validate_completion(record, log.replace(marker, ''), 'hash')
        with self.assertRaises(RuntimeError):
            timing.validate_completion(record, log, 'changed-hash')
        with self.assertRaises(RuntimeError):
            timing.validate_completion({**record, 'completed_cases': []}, log, 'hash')

if __name__ == '__main__':
    unittest.main()
