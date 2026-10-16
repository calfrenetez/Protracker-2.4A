"""Raw-header equivalence; no forged topology or authority is traversed."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base


class HeaderRanges(unittest.TestCase):
    def test_every_byte_bit_and_combined_mutable_fields_against_original_mask(self):
        before=base.preservation_snapshot();base.evidence('header-before-preservation.json',before)
        original_sources=base.SOURCES;pins={}
        try:
            base.SOURCES=('tests/sampler_establish_header_test.c',)
            with tempfile.TemporaryDirectory(prefix='pt-header-ranges-') as name:
                root=Path(name);pins=base.source_snapshot(root);fixture=root/'fixture'
                env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(root),
                     'ASAN_OPTIONS':'halt_on_error=1:abort_on_error=1',
                     'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
                for mode,extra,expected in (
                    ('host',(),b'ESTABLISH HEADER BYTE EQUIVALENCE PASS\n'),
                    ('native-entry',('-DPT_ESTABLISH_HEADER_NATIVE=1',),b'ESTABLISH HEADER NATIVE BYTE EQUIVALENCE PASS\n')):
                    compiled=subprocess.run(['/usr/bin/cc',*base.CFLAGS,*extra,*base.SOURCES,'-o',str(fixture)],cwd=root,env=env,capture_output=True,timeout=60)
                    base.evidence('header-'+mode+'-compile.stdout',compiled.stdout);base.evidence('header-'+mode+'-compile.stderr',compiled.stderr)
                    self.assertEqual((compiled.returncode,compiled.stdout,compiled.stderr),(0,b'',b''))
                    progress=root/'progress.log';progress.write_bytes(b'NOT STARTED\n')
                    run=subprocess.run([str(fixture),str(progress)],cwd=root,env=env,capture_output=True,timeout=30)
                    base.evidence('header-'+mode+'-stdout',run.stdout);base.evidence('header-'+mode+'-stderr',run.stderr)
                    self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))
                    self.assertEqual(progress.read_bytes(),b'ESTABLISH HEADER DONE\n')
                    progress.write_bytes(b'FOREIGN\n')
                    refused=subprocess.run([str(fixture),str(progress)],cwd=root,env=env,capture_output=True,timeout=30)
                    self.assertEqual((refused.returncode,refused.stdout,refused.stderr),(20,b'',b''));self.assertEqual(progress.read_bytes(),b'FOREIGN\n')
                    missing=root/'absent'
                    for args in ([],[str(missing)]):
                        refused=subprocess.run([str(fixture),*args],cwd=root,env=env,capture_output=True,timeout=30)
                        self.assertEqual((refused.returncode,refused.stdout,refused.stderr),(20,b'',b''))
                    self.assertFalse(missing.exists());base.verify_sources(pins)
        finally:
            base.SOURCES=original_sources;base.verify_sources(pins)
            after=base.preservation_snapshot();base.evidence('header-after-preservation.json',after)
            self.assertEqual(base.stable_fact(before),base.stable_fact(after))


if __name__=='__main__':unittest.main()
