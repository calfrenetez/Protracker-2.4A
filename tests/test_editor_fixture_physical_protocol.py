"""Host protocol boundaries only. No shared connection, reservation or target.

The small stand-in product isolates transfer/control faults without shipping a
native executable in the test suite. The stdout is the actual saved 624-byte
emulator result. A separate preparation check binds the actual promoted binary.
"""
import asyncio
import copy
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import editor_fixture_physical_protocol as protocol


def fingerprint(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


class Transport:
    """Injectable bounded fake transport, with every request recorded in order."""
    def __init__(self, expected):
        self.expected = expected
        self.calls = []
        self.files = {}
        self.now = 0
        self.polls = ['[OK]\nPT-WAIT\n', '[OK]\nPT-COMPLETE\n0\nCOMPLETE\n']
        self.fail_at = None
        self.bad_checksum_at = None
        self.create = '[OK]\nPT-CREATED\n'
        self.launch = '[OK] (no output)'
        self.cleanup = '[OK]\nPT-CLEAN\n'
        self.absence = '[OK]\nPT-ABSENT\n'
        self.poll_cost = 1
        self.bad_file = None
        self.pull_override = None

    async def sleep(self, seconds):
        self.now += seconds

    async def call(self, name, args, seconds):
        self.calls.append((name, copy.deepcopy(args), seconds))
        if len(self.calls) == self.fail_at:
            raise RuntimeError('first transport failure')
        if name == 'amiga_push_file':
            self.files[args['amiga_path'].rsplit('/', 1)[1]] = Path(args['local_path']).read_bytes()
            return 'Pushed successfully'
        if name == 'amiga_pull_file':
            key = args['amiga_path'].rsplit('/', 1)[1]
            if self.pull_override:
                self.pull_override(Path(args['local_path']))
                return 'Pulled successfully'
            data = self.files[key]
            if key == self.bad_file:
                data += b'corrupt'
            with Path(args['local_path']).open('xb') as stream:
                stream.write(data)
            return 'Pulled successfully'
        if name == 'amiga_checksum':
            key = args['path'].rsplit('/', 1)[1]
            data = self.files[key]
            crc = zlib.crc32(data)
            if len(self.calls) == self.bad_checksum_at:
                crc ^= 1
            return f'CRC32: {crc:08X}\nSize: {len(data)} bytes\n'
        script = args['script']
        if 'MakeDir' in script:
            return self.create
        if script.startswith('Run '):
            self.files.update({'fixture.log': self.expected, 'fixture.rc': b'0\n',
                               'complete.flag': b'COMPLETE\n', 'launcher.log': b'[3] 123\n'})
            return self.launch
        if 'Type ' in script:
            self.now += self.poll_cost
            return self.polls.pop(0) if len(self.polls) > 1 else self.polls[0]
        if 'Delete ' in script:
            return self.cleanup
        return self.absence


class PhysicalProtocol(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.binary = b'host protocol stand-in executable'
        self.expected = (ROOT / 'evidence/enhanced-editor/editor-bridges-native/fixture.log').read_bytes()
        self.assertEqual(fingerprint(self.expected), {'bytes': 624, 'sha256': protocol.LOG_SHA})
        self.binary_path = self.base / protocol.BINARY
        self.expected_path = self.base / 'expected.log'
        self.binary_path.write_bytes(self.binary)
        self.expected_path.write_bytes(self.expected)
        self.out = self.base / 'custody'
        self.out.mkdir()
        self.dest = protocol.namespace('0123456789abcdef' * 2)
        self.pins = patch.multiple(protocol, SHA=fingerprint(self.binary)['sha256'], BYTES=len(self.binary))
        self.pins.start()
        self.addCleanup(self.pins.stop)
        self.native = {'status': 'PASS_EXACT_030_SOFTWARE_FIXTURE_SETTLEMENT_PENDING',
                       'target': 'amiberry-030', 'launches': 1, 'elapsedSeconds': 213.214,
                       'candidate': fingerprint(self.binary),
                       'downloadedLogs': {name: fingerprint(data) for name, data in (
                           ('fixture.log', self.expected), ('fixture.rc', b'0\n'),
                           ('complete.flag', b'COMPLETE\n'))}}
        self.release = {'status': 'PASS_COMPLETED_DIAGNOSTIC_FIXTURE_VERIFIED_RELEASE',
                        'release': {'released': True, 'fence': 68},
                        'guardAfter': {'phase': 'EMPTY', 'fence': 68, 'reservation': None,
                                       'inflight': None, 'holds': [], 'potential_resident': False}}
        self.transport = Transport(self.expected)

    def execute(self, *, out=None, native=None, release=None, dest=None):
        t = self.transport
        return asyncio.run(protocol.execute_once(
            self.native if native is None else native,
            self.release if release is None else release,
            self.binary_path, self.expected_path, self.dest if dest is None else dest,
            self.out if out is None else out, t.call, clock=lambda: t.now, sleep=t.sleep))

    def assert_no_launch_or_cleanup(self):
        self.assertFalse(any(args.get('script', '').startswith('Run ') or 'Delete ' in args.get('script', '')
                             for _, args, _ in self.transport.calls))

    def test_one_launch_wait_complete_downloads_without_cleanup(self):
        result = self.execute()
        self.assertEqual(result['status'], 'PASS_EXACT_PHYSICAL_SOFTWARE_FIXTURE_SETTLEMENT_PENDING')
        self.assertEqual(result['target'], 'real-a1200')
        self.assertEqual(result['launches'], 1)
        self.assertEqual(result['observations'], 2)
        self.assertEqual(result['elapsedSeconds'], 7)
        self.assertEqual(sum(args.get('script', '').startswith('Run ') for _, args, _ in self.transport.calls), 1)
        self.assertFalse(any('Delete ' in args.get('script', '') for _, args, _ in self.transport.calls))
        self.assertEqual(set(result['files']), set(protocol.FILES))
        self.assertEqual((self.out / 'fixture.log').read_bytes(), self.expected)
        # Read-only result observations are bounded; there is no connect/reset/stop or device tool.
        self.assertEqual(set(name for name, _, _ in self.transport.calls),
                         {'amiga_run_script', 'amiga_push_file', 'amiga_pull_file', 'amiga_checksum'})
        self.assertTrue(all(0 < seconds <= 180 for _, _, seconds in self.transport.calls))

    def test_invalid_native_promotion_never_contacts_target(self):
        changes = ({'status': 'FAILED'}, {'target': 'real-a1200'}, {'launches': 0}, {'launches': True},
                   {'candidate': {}}, {'elapsedSeconds': 421}, {'elapsedSeconds': -1},
                   {'elapsedSeconds': float('nan')}, {'elapsedSeconds': float('inf')},
                   {'elapsedSeconds': True}, {'elapsedSeconds': '213'}, {'downloadedLogs': {}})
        for change in changes:
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.execute(native={**self.native, **change})
            self.assertEqual(self.transport.calls, [])
            self.assertEqual(list(self.out.iterdir()), [])

    def test_changed_native_rc_or_flag_never_contacts_target(self):
        for name in ('fixture.rc', 'complete.flag'):
            native = copy.deepcopy(self.native)
            native['downloadedLogs'][name]['sha256'] = '0' * 64
            with self.subTest(name=name), self.assertRaises(ValueError):
                self.execute(native=native)
            self.assertEqual(self.transport.calls, [])

    def test_incomplete_release_or_hold_never_contacts_target(self):
        changes = ({'phase': 'HOLD'}, {'fence': 67}, {'reservation': {}}, {'inflight': {}},
                   {'holds': ['hold']}, {'potential_resident': True}, {'fence': None})
        releases = [{**self.release, 'guardAfter': {**self.release['guardAfter'], **c}} for c in changes]
        releases += [{**self.release, 'release': {'released': False, 'fence': 68}},
                     {**self.release, 'status': 'UNVERIFIED'},
                     {**self.release, 'guardAfter': {'phase': 'EMPTY', 'fence': 68}}]
        for release in releases:
            with self.subTest(release=release), self.assertRaises(ValueError):
                self.execute(release=release)
            self.assertEqual(self.transport.calls, [])

    def test_changed_product_or_markers_never_contacts_target(self):
        for path in (self.binary_path, self.expected_path):
            original = path.read_bytes()
            for changed in (original + b'x', bytes([original[0] ^ 1]) + original[1:]):
                path.write_bytes(changed)
                with self.subTest(path=path, size=len(changed)), self.assertRaises(ValueError):
                    self.execute()
                self.assertEqual(self.transport.calls, [])
            path.write_bytes(original)

    def test_unsafe_namespace_refuses_before_target(self):
        for dest in ('RAM:', self.dest + '/other', self.dest + '\nDelete SYS: ALL',
                     self.dest.upper(), 'SYS:PT-editor-bridges-' + 'a' * 32):
            with self.subTest(dest=dest), self.assertRaises(ValueError):
                self.execute(dest=dest)
            self.assertEqual(self.transport.calls, [])

    def test_symlink_and_used_custody_refuse_before_target(self):
        for path in (self.binary_path, self.expected_path, self.out):
            link = self.base / ('link-' + path.name)
            link.symlink_to(path)
            original = self.binary_path, self.expected_path
            if path == self.binary_path:
                self.binary_path = link
            elif path == self.expected_path:
                self.expected_path = link
            with self.subTest(path=path), self.assertRaises(ValueError):
                self.execute(out=link if path == self.out else None)
            self.binary_path, self.expected_path = original
            self.assertEqual(self.transport.calls, [])
        (self.out / 'old').write_bytes(b'old')
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(self.transport.calls, [])

    def test_existing_guest_namespace_never_stages_or_launches(self):
        self.transport.create = '[OK]\nPT-EXISTS\n'
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 1)
        self.assert_no_launch_or_cleanup()

    def test_upload_uses_frozen_promoted_bytes(self):
        original = self.transport.call
        async def mutate_after_admission(name, args, seconds):
            result = await original(name, args, seconds)
            if 'MakeDir' in args.get('script', ''):
                self.binary_path.write_bytes(b'changed after promotion')
            return result
        self.transport.call = mutate_after_admission
        self.execute()
        self.assertEqual((self.out / protocol.BINARY).read_bytes(), self.binary)
        self.assertEqual(self.transport.files[protocol.BINARY], self.binary)

    def test_completed_run_cannot_be_replayed_into_same_custody(self):
        self.execute()
        calls = len(self.transport.calls)
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(len(self.transport.calls), calls)

    def test_positive_launch_with_output_is_supported(self):
        self.transport.launch = '[OK]\n[3] 123\n'
        self.execute()

    def test_every_transport_failure_stops_at_first_call(self):
        # Success has 16 requests. Fail each boundary, including transfer and
        # completed-log collection; not one sends recovery, cleanup or retry.
        for fail_at in range(1, 17):
            self.transport = Transport(self.expected)
            self.transport.fail_at = fail_at
            out = self.base / f'failure-{fail_at}'
            out.mkdir()
            with self.subTest(fail_at=fail_at), self.assertRaisesRegex(RuntimeError, 'first transport failure'):
                self.execute(out=out)
            self.assertEqual(len(self.transport.calls), fail_at)
            self.assertFalse(any('Delete ' in a.get('script', '') for _, a, _ in self.transport.calls))

    def test_cancelled_request_has_no_followup(self):
        async def cancelled(name, args, seconds):
            self.transport.calls.append((name, args, seconds))
            raise asyncio.CancelledError()
        self.transport.call = cancelled
        with self.assertRaises(asyncio.CancelledError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 1)

    def test_wrong_staged_checksum_never_launches(self):
        self.transport.bad_checksum_at = 3
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 3)
        self.assert_no_launch_or_cleanup()

    def test_uncertain_launch_stops_without_poll_or_cleanup(self):
        self.transport.launch = '[OK]garbage'
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 6)

    def test_nonzero_or_uncertain_completion_stops_without_download(self):
        for reply in ('[OK]\nPT-COMPLETE\n20\nCOMPLETE\n', '[STILL RUNNING]',
                      '[OK]\nPT-COMPLETE\n0\n', '[OK]\nPT-WAIT\nextra\n'):
            self.transport = Transport(self.expected)
            self.transport.polls = [reply]
            out = self.base / f'poll-{len(list(self.base.iterdir()))}'
            out.mkdir()
            with self.subTest(reply=reply), self.assertRaises(ValueError):
                self.execute(out=out)
            self.assertEqual(len(self.transport.calls), 7)

    def test_deadline_is_absolute_even_when_polls_keep_waiting(self):
        self.transport.polls = ['[OK]\nPT-WAIT\n']
        self.transport.poll_cost = 100
        with self.assertRaises(TimeoutError):
            self.execute()
        self.assertLessEqual(len(self.transport.calls), 11)
        self.assertEqual(sum(a.get('script', '').startswith('Run ') for _, a, _ in self.transport.calls), 1)
        self.assertFalse(any(name == 'amiga_pull_file' for name, _, _ in self.transport.calls))

    def test_completion_after_deadline_never_downloads(self):
        self.transport.polls = ['[OK]\nPT-COMPLETE\n0\nCOMPLETE\n']
        self.transport.poll_cost = 421
        with self.assertRaises(TimeoutError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 7)

    def test_missing_symlink_or_oversized_download_stops_before_checksum(self):
        def missing(path):
            pass
        def symlink(path):
            path.symlink_to(self.expected_path)
        def oversized(path):
            path.write_bytes(b'x' * 1048577)
        for index, override in enumerate((missing, symlink, oversized)):
            self.transport = Transport(self.expected)
            self.transport.pull_override = override
            out = self.base / f'pull-{index}'
            out.mkdir()
            with self.subTest(override=override), self.assertRaises(ValueError):
                self.execute(out=out)
            self.assertEqual(len(self.transport.calls), 9)

    def test_checksum_is_unambiguous_and_bounded(self):
        good = f'CRC32: {zlib.crc32(self.binary):08X}\nSize: {len(self.binary)} bytes\n'
        for text in (None, good * 2, good.replace('CRC32:', 'partial:'),
                     good.replace(f'{len(self.binary)} bytes', '0 bytes'), 'x' * 4097):
            with self.subTest(text=str(text)[:40]), self.assertRaises(ValueError):
                protocol.checksum(text, self.binary)

    def test_corrupt_download_stops_at_guest_checksum(self):
        self.transport.bad_file = 'fixture.log'
        with self.assertRaises(ValueError):
            self.execute()
        self.assertEqual(len(self.transport.calls), 10)
        self.assertTrue((self.out / 'fixture.log').exists())  # Preserve first bytes.
        self.assertFalse((self.out / 'fixture.rc').exists())

    def settlement(self, result, data=None):
        return asyncio.run(protocol.settle_completed(
            result, self.transport.files if data is None else data, self.expected, self.transport.call))

    def test_settlement_checks_all_six_before_exact_cleanup_then_absence(self):
        result = self.execute()
        self.transport.calls.clear()
        settled = self.settlement(result)
        self.assertEqual(settled['status'], 'PASS_COMPLETED_PHYSICAL_FIXTURE_FILES_ABSENT')
        self.assertEqual([n for n, _, _ in self.transport.calls], ['amiga_checksum'] * 6 + ['amiga_run_script'] * 2)
        script = self.transport.calls[6][1]['script']
        self.assertNotIn(' ALL', script)
        self.assertEqual([line for line in script.splitlines() if line.startswith('Delete ')],
                         ['Delete ' + self.dest + '/' + n + ' QUIET' for n in protocol.FILES] +
                         ['Delete ' + self.dest + ' QUIET'])
        self.assertNotIn('Delete ', self.transport.calls[7][1]['script'])

    def test_incomplete_or_mismatched_custody_never_deletes(self):
        result = self.execute()
        self.transport.calls.clear()
        for name in protocol.FILES:
            data = dict(self.transport.files)
            data.pop(name)
            with self.subTest(name=name), self.assertRaises(ValueError):
                self.settlement(result, data)
            self.assertEqual(self.transport.calls, [])
        data = dict(self.transport.files)
        data['Run-once'] += b'other\n'
        changed = {**result, 'files': {n: fingerprint(b) for n, b in data.items()}}
        with self.assertRaises(ValueError):
            self.settlement(changed, data)
        self.assertEqual(self.transport.calls, [])
        with self.assertRaises(ValueError):
            self.settlement({**result, 'status': 'FAILED'})
        self.assertEqual(self.transport.calls, [])

    def test_settlement_checksum_failure_never_deletes(self):
        result = self.execute()
        self.transport.calls.clear()
        self.transport.bad_checksum_at = 6
        with self.assertRaises(ValueError):
            self.settlement(result)
        self.assertEqual(len(self.transport.calls), 6)
        self.assertTrue(all(n == 'amiga_checksum' for n, _, _ in self.transport.calls))

    def test_cleanup_failure_sends_no_absence_or_recovery_request(self):
        result = self.execute()
        self.transport.calls.clear()
        self.transport.cleanup = '[STILL RUNNING]'
        with self.assertRaises(ValueError):
            self.settlement(result)
        self.assertEqual(len(self.transport.calls), 7)

    def test_absence_failure_stops_without_retry(self):
        result = self.execute()
        self.transport.calls.clear()
        self.transport.absence = '[OK]\nPT-REMAINS\n'
        with self.assertRaises(ValueError):
            self.settlement(result)
        self.assertEqual(len(self.transport.calls), 8)


if __name__ == '__main__':
    unittest.main()
