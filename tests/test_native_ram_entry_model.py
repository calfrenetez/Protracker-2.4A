"""Focused future host model: exact eight entry C TUs, two model TUs, fresh process per case."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ENTRY_MODEL_SOURCES = [
    'tests/native_ram_entry_model_test.c',
    'tests/native_ram_entry_host_stubs.c',
    'src/diagnostic/native_ram_entry.c',
    'src/native/readers_ram/native_ram_port.c',
    'src/native/readers_ram/native_ram_irq_dispatch.c',
    'src/native/readers_ram/native_cia_ram_adapter.c',
    'src/native/native_checked_memory.c',
    'src/core/elapsed_clock.c',
    'src/core/scheduled_readers.c',
    'src/core/readers_activation.c',
]
ENTRY_MODEL_MARKER = ('NATIVE ENTRY HOST MODEL CASE PASS: software resource oracle only; '
                      'no native ABI, placement, IRQ, timer or stack qualification')

class NativeEntryModelTest(unittest.TestCase):
    def test_bounded_resource_failures(self):
        root = Path(__file__).resolve().parents[1]
        # Fixed source-only recipe. Root pins its actual compiler/interceptor and
        # source before the separately authorized first invocation.
        with tempfile.TemporaryDirectory(prefix='pt-native-entry-host-') as work:
            product = Path(work) / 'entry-host-model'
            command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g',
                       '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                       '-fno-omit-frame-pointer', '-UNDEBUG',
                       '-Isrc/core', '-Isrc/native', '-Itests/native_entry_host_sdk',
                       '-include', 'tests/native_ram_entry_host_stubs.h',
                       '-Dmain=pt_entry_model_main', *ENTRY_MODEL_SOURCES, '-o', str(product)]
            subprocess.run(command, cwd=root, check=True)
            cases = [(mode, 0) for mode in range(34) if mode not in (2, 3, 4, 5)]
            cases += [(2, n) for n in range(1, 19)]
            cases += [(mode, n) for mode in (3, 4, 5) for n in (1, 2)]
            self.assertEqual(len(cases), 54)
            # New independent stack-admission matrix; original54 vectors stay exact.
            cases += [(34, n) for n in range(22)]
            self.assertEqual(len(cases), 76)
            # Original76 vectors/oracles above remain unchanged. New fixed
            # system-span and sampled-boundary cases use their own two modes.
            cases += [(35, n) for n in range(17)]
            cases += [(36, n) for n in range(21)]
            self.assertEqual(len(cases), 114)
            # Stop at the first failed fresh-process case; preserve that result
            # before root considers any separately scoped source correction.
            for mode, ordinal in cases:
                run = subprocess.run([str(product), str(mode), str(ordinal)], cwd=root,
                                     check=True, capture_output=True, text=True, timeout=20)
                self.assertEqual(run.stdout.count(ENTRY_MODEL_MARKER), 1)
                self.assertNotIn('AddressSanitizer', run.stderr)
                self.assertNotIn('runtime error:', run.stderr)
                if mode == 34:
                    self.assertEqual(run.stdout.count('NATIVE ENTRY TASK STACK MODEL PASS:'), 1)
                if mode == 35:
                    self.assertEqual(run.stdout.count('NATIVE ENTRY SYSTEM STACK MODEL PASS:'), 1)
                if mode == 36:
                    self.assertEqual(run.stdout.count('NATIVE ENTRY SAMPLED STACK MODEL PASS:'), 1)
                print(run.stdout, end='')

if __name__ == '__main__':
    unittest.main()
