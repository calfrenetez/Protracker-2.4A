# Device upload cache boundary — 2026-09-23

The production PCM converter/cache interface was tested against opaque simulated
device descriptors with synchronous upload and injected partial-transfer failure.
No AmiGUS register, card memory or hardware operation was performed. The native
editor does not yet dispatch to this interface.

Focused ASan/UBSan tests: `python3 -m unittest discover -s tests -p 'test_playback*.py' -v`
passed both conversion and device upload cases.

Native test cross-build command (pinned compiler):

```
/Users/james1/Documents/Codex/2026-08-20/work-from-the-design-spec-v1/repo/.cache/amiga/bin/m68k-amigaos-gcc -std=c99 -O2 -Wall -Wextra -Werror -m68030 -Isrc/core tests/playback_upload_test.c src/core/playback_pcm.c src/core/sample_cache.c src/core/pcm.c -o build/dev/PTPlaybackUploadTest
```

Cross-build PASS. Binary SHA256: `126c40daf9dfe6a9d56f44f1898b680a9858b82351d8269b64fd087a343ae498`.
This binary has NOT run in Amiberry or on physical hardware. AmiConnect held the
shared emulator; ProTracker made no guest requests or resource claim this turn.
The full editor was not rebuilt or packaged for this change.

Full host suite (`make test`):

```
----------------------------------------------------------------------
Ran 104 tests in 181.199s

OK
ARPEGGIO: nine repeated native traces match host/m68k period words and ramp PCM, including zero-period refusal
FLOW: all 16 native trace fixtures match portable control flow
PITCH: seven repeated native traces match host/m68k state, PCM and zero-period refusal
PORTAMENTO phase oracle rejects forced retrigger on the same-instrument glide
PORTAMENTO: nine repeated native traces match host/m68k pitch, volume and DMA-driven phase; 16-track state and refusal checks pass
VOLUME: seven repeated pinned native traces match every host/m68k rendered PCM frame
OFFSET: eight repeated native traces verify stored ranges, initial trigger ranges, loops, memory, bounds, delayed rows and four independent tracks; shipping 9xx still refused
TRIGGERS: ten repeated native fixtures verify E9x/EDx counter timing, retained notes, pattern delay and offset/loop ranges; renderer enablement remains open
TREMOLO: eight repeated native traces verify output/stored volume, waveforms, nibble memory, reset control, delay and vibrato-phase ramp dependency; renderer support remains open
UNUSED: six repeated native traces match host/m68k stored/output words and PCM for8xx/E8x
VIBRATO: eleven repeated native traces match host/m68k period words and ramp PCM
VOICE traversal PASS: selected ranges, forward/pingpong seams, fractional steps, one-frame loops and large steps
VOICE mix PASS: true24 stereo, single quantization, clipping, 16 voices, mute progression, capacity and alias refusal
VOICE partition PASS hash=990c42f0 clips=40
SEGMENT trajectory 0 hash=2a50eba5
SEGMENT trajectory 1 hash=295ede49
SEGMENT trajectory 2 hash=ac5189f5
SEGMENT trajectory 3 hash=e5165089
SEGMENT trajectory 4 hash=594e0cb9
VOICE segment PASS: independent initial/repeat ranges, interpolation handoff, huge steps, one-frame repeats and invalid-range preservation
```
