# Bounded device upload staging — 2026-09-23

Production `pt_playback_pcm_upload_chunks` tested with an opaque fake device.
The staging size is independent of sample size; all output bytes and their
ordered offsets match whole-sample conversion. Cache publication occurs only
after all writes finish. Injected second-chunk failure releases the partial
resource; retry starts at zero. Source bytes remain unchanged.

The test sweeps source channels, output8/16, byte order, padding and capacities
from one frame through11 bytes, including odd16-bit buffer capacities and a
separate final8-bit padding chunk. Staging canaries remain intact.

Focused ASan/UBSan: `python3 -m unittest discover -s tests -p 'test_playback*.py' -v`
PASS (2 tests).

Pinned68030 cross-build PASS, same command as device-upload-cache/README.md.
PTPlaybackUploadTest SHA256: `afa2ad76af8eb9dec482a8309778f19355ba54ad6ba829cdae215d645b827fd8`.

No emulator or physical run. AmiConnect retained the shared paused guest for
forensics/comparison; ProTracker performed only host operations. No real AmiGUS
allocation/upload protocol or native editor device dispatch is claimed.

Full host suite (`make test`):

```
Ran 104 tests in 170.193s

OK
```
