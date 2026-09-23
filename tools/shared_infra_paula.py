#!/usr/bin/env python3
"""Bounded Paula regression on a separately reserved shared 030 guest.

Reuses the shared Guest ownership/CPU/bridge guards and nonblocking test lock.
Does not boot, stop, reconnect or deploy to physical hardware.
"""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import shutil
import sys
import time

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--infra', type=Path, default=Path('/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra'))
    args = parser.parse_args()
    sys.path.insert(0, str(args.infra / 'scripts'))
    from shared_guest import Guest
    out = ROOT / 'build/dev' / ('paula-cache-' + str(time.time_ns()))
    out.mkdir()
    with (args.infra / 'runtime/test.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        guest = Guest(args.infra, out)
        run = guest.share / out.name
        run.mkdir()
        finished = False
        result = {'passed': False, 'scope': 'shared emulator Paula regression', 'guest_directory': str(run)}
        try:
            for source, name in [(ROOT / 'build/dev/PTPaulaTest', 'PTPaulaTest'),
                                 (ROOT / 'build/dev/PTSampleCacheTest', 'PTSampleCacheTest'),
                                 (ROOT / 'build/dev/PTPlaybackPCMTest', 'PTPlaybackPCMTest'),
                                 (ROOT / 'build/dev/PTPlaybackUploadTest', 'PTPlaybackUploadTest'),
                                 (ROOT / 'build/dev/PTPreviewPCMTest', 'PTPreviewPCMTest'),
                                 (ROOT / 'evidence/baseline/mod.baseline', 'input.mod')]:
                shutil.copyfile(source, run / name)
                result[name + '_sha256'] = hashlib.sha256((run / name).read_bytes()).hexdigest()
            guest.launch.write_text('\n'.join(['FailAt 21', 'Stack 65536', 'CD ' + guest.device + run.name,
                                              'PTSampleCacheTest >cache.log', 'Echo $RC >cache.rc',
                                              'PTPlaybackPCMTest >pcm.log', 'Echo $RC >pcm.rc',
                                              'PTPlaybackUploadTest >upload.log', 'Echo $RC >upload.rc',
                                              'PTPreviewPCMTest >preview.log', 'Echo $RC >preview.rc',
                                              'PTPaulaTest input.mod >paula.log', 'Echo $RC >paula.rc']) + '\n')
            guest.start()
            end = time.monotonic() + 60
            while not (run / 'paula.rc').exists():
                if time.monotonic() > end:
                    raise RuntimeError('Paula test timed out; preserve run files for guarded recovery')
                time.sleep(.2)
            finished = True
            log = (run / 'paula.log').read_text()
            (out / 'native-paula.log').write_text(log)
            cache_log = (run / 'cache.log').read_text()
            (out / 'native-cache.log').write_text(cache_log)
            pcm_log = (run / 'pcm.log').read_text()
            (out / 'native-pcm.log').write_text(pcm_log)
            for stem, marker in [('upload', 'PLAYBACK UPLOAD PASS:'), ('preview', 'PAULA PREVIEW PASS:')]:
                text = (run / (stem + '.log')).read_text()
                (out / ('native-' + stem + '.log')).write_text(text)
                result[stem + '_returncode'] = (run / (stem + '.rc')).read_text().strip()
                assert result[stem + '_returncode'] == '0' and marker in text, text
            result['pcm_returncode'] = (run / 'pcm.rc').read_text().strip()
            result['cache_returncode'] = (run / 'cache.rc').read_text().strip()
            result['returncode'] = (run / 'paula.rc').read_text().strip()
            result['audio_after'] = guest.command('GET_AUDIO_STATE')
            assert result['pcm_returncode'] == '0' and 'PLAYBACK PCM PASS:' in pcm_log, pcm_log
            assert all(marker in log for marker in ('PREVIEW PCM PASS:', 'PREVIEW RATE PASS:', 'PREVIEW LOOP PASS:', 'PREVIEW STEREO PASS:', 'PREVIEW CANCEL PASS:')), log
            assert result['cache_returncode'] == '0' and 'SAMPLE CACHE PASS:' in cache_log, cache_log
            assert 'CACHE REUSE PASS:' in log, log
            assert result['returncode'] == '0' and 'CACHE PASS:' in log and 'PAULA PASS:' in log, log
            assert all('ch%d_dma=0' % i in result['audio_after'].split('\t') for i in range(4)), result['audio_after']
            result['passed'] = True
        finally:
            if finished:
                guest.launch.unlink()
                shutil.rmtree(run)
            result['run_files_cleaned'] = finished
            (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
            print(out)

if __name__ == '__main__':
    main()
