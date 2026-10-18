"""HOST27 regression of exact selected private native audio lifecycle C.

Fake Exec/audio shapes and SP/Wait boundary only; never native ABI/device/Task
retirement or a default frontend stop certificate. Production hard guards stay
unchanged. Run qualification under the existing owned HOST supervisor; this
focused repository test adds no target or process-control mechanism.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import stat
import struct
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = None
SOURCES = (
    'src/native/native_audio_reservation.c',
    'src/native/native_audio_reservation.h',
    'tests/native_audio_reservation_host27.c',
    'tests/native_audio_reservation_host27/host_exec_model.h',
    'tests/native_audio_reservation_host27/expected-oracles.json',
    'tests/native_audio_reservation_host27/extraction.json',
    'tests/test_native_audio_reservation.py',
)
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
          '-fsanitize=address,undefined', '-fno-omit-frame-pointer')
CASES = tuple('%02d' % value for value in range(1, 28))
STAT_KEYS = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid',
             'st_size','st_mtime_ns','st_ctime_ns','st_flags')


def source_bytes(relative):
    relative = PurePosixPath(relative)
    if relative.is_absolute() or '..' in relative.parts:
        raise AssertionError('source escapes repository')
    path = ROOT / relative
    before = path.lstat()
    fields = lambda item: tuple(getattr(item,key,0) for key in STAT_KEYS)
    if (not stat.S_ISREG(before.st_mode) or before.st_nlink != 1 or
            getattr(before,'st_flags',0) & 0x40000000 or before.st_size > 1024*1024):
        raise AssertionError('bounded ordinary resident source required: '+str(relative))
    fd = os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        if fields(os.fstat(fd)) != fields(before):
            raise AssertionError('opened source identity changed')
        data = bytearray()
        while len(data) <= before.st_size:
            block = os.read(fd,min(65536,before.st_size+1-len(data)))
            if not block:
                break
            data.extend(block)
        if len(data) != before.st_size or fields(os.fstat(fd)) != fields(before):
            raise AssertionError('source changed while read')
    finally:
        os.close(fd)
    if fields(path.lstat()) != fields(before):
        raise AssertionError('source changed after read')
    return bytes(data),(hashlib.sha256(data).hexdigest(),fields(before))


class NativeAudioReservationHost(unittest.TestCase):
    def test_exact_lifecycle_and_twenty_seven_boundary_cases(self):
        self.assertEqual(struct.calcsize('P'),8,'wide-mask oracles require a64-bit HOST')
        self.assertEqual((len(SOURCES),len(CASES)),(7,27))
        observed,children = {},[]
        receipt = {'state':'NOT_RUN','children':children,'HOSTCases':27,
                   'sanitizers':'ASAN_UBSAN','leakSanitizer':'UNAVAILABLE_NOT_REQUESTED',
                   'scope':'Actual nine selected production functions under fake Exec/audio/SP/Wait only',
                   'nativeRuntime':'NOT_RUN','targetOperations':0}
        if EVIDENCE is not None:
            EVIDENCE.mkdir()
        started = time.monotonic()
        deadline = started + 240

        def read(relative):
            data,facts = source_bytes(relative)
            if relative in observed:
                self.assertEqual(observed[relative],facts,relative)
            observed[relative] = facts
            return data

        def digest(data, expected):
            self.assertEqual(len(data),expected['bytes'])
            self.assertEqual(hashlib.sha256(data).hexdigest(),expected['sha256'])

        def child(label,argv,private,environment,limit):
            remaining = deadline-time.monotonic()
            self.assertGreater(remaining,0,'whole240s HOST bound expired')
            actual = subprocess.run(argv,cwd=private,env=environment,stdin=subprocess.DEVNULL,
                                    capture_output=True,timeout=min(limit,remaining))
            row = {'label':label,'argv':list(argv),'returncode':actual.returncode,
                   'stdoutBytes':len(actual.stdout),'stdoutSHA256':hashlib.sha256(actual.stdout).hexdigest(),
                   'stderrBytes':len(actual.stderr),'stderrSHA256':hashlib.sha256(actual.stderr).hexdigest()}
            children.append(row)
            self.assertLessEqual(len(actual.stdout),1048576)
            self.assertLessEqual(len(actual.stderr),1048576)
            if EVIDENCE is not None:
                (EVIDENCE/(label+'.stdout')).write_bytes(actual.stdout)
                (EVIDENCE/(label+'.stderr')).write_bytes(actual.stderr)
                (EVIDENCE/'children.json').write_text(json.dumps(children,indent=2)+'\n')
            return actual

        try:
            inputs = {name:read(name) for name in SOURCES}
            contract = json.loads(inputs[SOURCES[5]])
            expected = json.loads(inputs[SOURCES[4]])
            production,header = inputs[SOURCES[0]],inputs[SOURCES[1]]
            digest(production,contract['production']);digest(header,contract['header'])
            self.assertEqual(contract['functionNames'],[
                'ordinary','held','request_storage','keyed_requests','refuse_unissued',
                'pt_private_audio_enroll','pt_private_audio_retire','overlaps','pt_private_audio_observe'])
            preamble,sp = contract['nativePreamble'],contract['nativeSP']
            self.assertEqual(preamble['byteOffset'],0)
            native_prefix = production[:preamble['bytes']]
            native_sp = production[sp['byteOffset']:sp['byteOffset']+sp['bytes']]
            digest(native_prefix,preamble);digest(native_sp,sp)
            replacement = sp['replacement'].encode('ascii')
            self.assertEqual(replacement,b'static uintptr_t current_stack_pointer(void);\n\n')
            before_sp = production[preamble['bytes']:sp['byteOffset']]
            selected = before_sp + replacement + production[sp['byteOffset']+sp['bytes']:]
            digest(selected,contract['selected'])
            inverse = native_prefix + selected[:len(before_sp)] + native_sp + selected[len(before_sp)+len(replacement):]
            self.assertEqual(inverse,production,'full native-source inverse must stay byte exact')
            self.assertEqual(expected['caseCount'],27)
            self.assertEqual([row['case'] for row in expected['cases']],list(CASES))
            # The qualified fixture is copied unchanged, including HOST24 legacy
            # labels. Its27 cases and stdout oracles are never renamed here.
            with tempfile.TemporaryDirectory(prefix='pt-native-audio-host27-') as temporary:
                private = Path(temporary)/'source';work = Path(temporary)/'tmp'
                private.mkdir();work.mkdir()
                for name,data in (
                    ('audio_reservation_host24.c',inputs[SOURCES[2]]),
                    ('native_audio_reservation.h',header),
                    ('host_exec_model.h',inputs[SOURCES[3]]),
                    ('selected_native_audio_algorithm.h',selected)):
                    (private/name).write_bytes(data)
                environment = {'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(work),
                    'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
                executable = Path(temporary)/'fixture'
                compiled = child('compile',['/usr/bin/cc',*CFLAGS,
                    str(private/'audio_reservation_host24.c'),'-o',str(executable)],private,environment,90)
                self.assertEqual(compiled.returncode,0,compiled.stderr.decode(errors='replace'))
                self.assertEqual(compiled.stdout,b'');self.assertEqual(compiled.stderr,b'')
                for name,want in zip(CASES,expected['cases']):
                    actual = child('case-'+name,[str(executable),name],private,environment,2)
                    self.assertEqual(actual.returncode,want['returncode'],actual.stderr.decode(errors='replace'))
                    self.assertEqual(actual.stdout,want['stdoutUTF8'].encode('utf8'))
                    self.assertEqual(len(actual.stdout),want['stdoutBytes'])
                    self.assertEqual(hashlib.sha256(actual.stdout).hexdigest(),want['stdoutSHA256'])
                    self.assertEqual(actual.stderr,b'')
                for relative,facts in observed.items():
                    self.assertEqual(source_bytes(relative)[1],facts,relative)
                self.assertLessEqual(time.monotonic(),deadline)
                receipt.update(state='HOST27_NATIVE_AUDIO_SELECTED_C_PASS_NOT_NATIVE',
                    sourceCount=len(observed),sources=observed,allSourceFactsEqual=True,
                    literalProductionFunctions=9,commands=len(children),all27OraclesExact=True)
        except BaseException as error:
            receipt.update(state='HOST27_NATIVE_AUDIO_SELECTED_C_FAILED_NO_REPLAY',
                           error=repr(error),sources=observed)
            raise
        finally:
            receipt['seconds'] = time.monotonic()-started
            if EVIDENCE is not None:
                (EVIDENCE/'result.json').write_text(json.dumps(receipt,indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--evidence',type=Path)
    options,remaining = parser.parse_known_args()
    EVIDENCE = options.evidence
    unittest.main(argv=[sys.argv[0],*remaining])
