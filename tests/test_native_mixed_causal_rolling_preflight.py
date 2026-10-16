"""HOST sanitizer qualification of original preflight/cancellation only."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import test_native_mixed_causal_rolling_entry as base

SOURCES = ('tests/native_exec_mixed_causal_rolling_preflight_test.c', *base.SOURCES[1:])
ORACLE = b'ROLLING PREFLIGHT NATIVE PASS:original preflight and cancellation;MODELED_ONLY\n'


class PreflightHost(unittest.TestCase):
    EXTRA_FLAGS = ()
    EXPECTED = ORACLE
    DONE = b'ROLLING PREFLIGHT DONE phase=0\n'
    EVIDENCE_PREFIX = 'preflight-'

    def test_original_preflight_cancellation_disposal_and_foreign_file_refusals(self):
        def evidence(name,value):base.evidence(self.EVIDENCE_PREFIX+name,value)
        before=base.preservation_snapshot();evidence('before-preservation.json',before)
        pins={};original_sources=base.SOURCES
        try:
            base.SOURCES=SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-rolling-preflight-') as directory:
                private=Path(directory);pins=base.source_snapshot(private)
                font_pin,font=base.ordinary_pin(base.ROOT/'vendor/pt23f/raw/ptfont.raw')
                self.assertEqual(font_pin['sha256'],base.RAW_FONT_SHA256)
                header=private/'pt_font.h';header.write_text('static const unsigned char pt_font[580] = {'+','.join(str(x) for x in font)+'};\n')
                self.assertEqual(base.ordinary_pin(header)[0]['sha256'],base.FONT_HEADER_SHA256)
                env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(private),
                     'ASAN_OPTIONS':'halt_on_error=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
                fixture=private/'preflight'
                compiled=subprocess.run(['/usr/bin/cc',*base.CFLAGS,*self.EXTRA_FLAGS,*SOURCES,'-o',str(fixture)],cwd=private,env=env,capture_output=True,timeout=120)
                evidence('compile.stdout',compiled.stdout);evidence('compile.stderr',compiled.stderr)
                self.assertEqual((compiled.returncode,compiled.stdout,compiled.stderr),(0,b'',b''))
                progress=private/'progress.log';progress.write_bytes(b'NOT STARTED\n')
                actual=subprocess.run([str(fixture),str(progress)],cwd=private,env=env,capture_output=True,timeout=30)
                evidence('fixture.stdout',actual.stdout);evidence('fixture.stderr',actual.stderr);evidence('progress.log',progress.read_bytes())
                self.assertEqual((actual.returncode,actual.stdout,actual.stderr),(0,self.EXPECTED,b''))
                self.assertEqual(progress.read_bytes(),self.DONE)
                progress.write_bytes(b'FOREIGN CONTENT\n')
                foreign=subprocess.run([str(fixture),str(progress)],cwd=private,env=env,capture_output=True,timeout=30)
                self.assertEqual((foreign.returncode,foreign.stdout,foreign.stderr),(20,b'',b''))
                self.assertEqual(progress.read_bytes(),b'FOREIGN CONTENT\n')
                absent=private/'not-created'
                for arguments in ([],[str(absent)]):
                    rejected=subprocess.run([str(fixture),*arguments],cwd=private,env=env,capture_output=True,timeout=30)
                    self.assertEqual((rejected.returncode,rejected.stdout,rejected.stderr),(20,b'',b''))
                self.assertFalse(absent.exists());base.verify_sources(pins)
        finally:
            base.SOURCES=original_sources;base.verify_sources(pins)
            after=base.preservation_snapshot();evidence('after-preservation.json',after)
            self.assertEqual(base.stable_fact(before),base.stable_fact(after))


class PreflightPrefixHost(PreflightHost):
    EXTRA_FLAGS = ('-DPT_ROLLING_PREFLIGHT_PREFIX=64',)
    EXPECTED = b'ROLLING PREFIX NATIVE PASS:64 original steps and cancellation;MODELED_ONLY\n'
    DONE = b'ROLLING PREFIX DONE steps=64 phase=0\n'
    EVIDENCE_PREFIX = 'prefix-'


class PreflightTraceHost(PreflightHost):
    EXTRA_FLAGS = ('-DPT_ROLLING_PREFLIGHT_TRACE=1',)
    EXPECTED = b'ROLLING TRACE NATIVE PASS:original preflight and cancellation;MODELED_ONLY\n'
    DONE = b'ROLLING TRACE DONE phase=0\n'
    EVIDENCE_PREFIX = 'trace-'


class PreflightWork256Host(PreflightHost):
    EXTRA_FLAGS = ('-DPT_ROLLING_PREFLIGHT_WORK256=1',)
    EXPECTED = b'ROLLING WORK256 NATIVE PASS:complete preflight and cancellation;MODELED_ONLY\n'
    DONE = b'ROLLING WORK256 DONE phase=0\n'
    EVIDENCE_PREFIX = 'work256-'


if __name__ == '__main__':unittest.main()
