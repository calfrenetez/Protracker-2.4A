# Native sampler integration, dev13

SAMPLER / Control-L retains the accepted classic controls and replaces the pattern
area with a mono/stereo waveform and two-click frame selection. The native page
connects exact integer PCM WAV import/export, reverse, normalize, half/double gain,
fade in/out and DC-offset removal. Operations use the selected frame range; WAV
export writes the entire selected sample. Existing output files are refused.

Sample versions join the same bounded chronological journal as notes, blocks and
channel settings. Versions are immutable, independently allocated, deep-copy slice
metadata, and use reference counts to keep the active sample alive when commands
are evicted or redo is discarded. The 32 MiB version budget is additional to the
loaded document and temporary input bytes. Staging/allocation failure preserves
project, redo history and current playback. Successful sample changes stop the
old private replay snapshot. Import preserves 8/16/24-bit precision, mono/stereo
and sample rate without automatic conversion. Replacing a sample resets its
loop/slice metadata; active slice references cause refusal. Redo rechecks slice
references so an external event change cannot create an invalid project.

## Evidence

`core-build.json` records final source/toolchain/binary identities. `host-tests.log`
contains 24 passing groups. New sampler tests run with address/undefined-behaviour
sanitizers and cover exact stereo24 import, range boundaries, note/channel/sample
history ordering, clean-state restoration, no-op redo preservation, allocation
failure at every staging step, memory-budget refusal, command eviction and redo
truncation, sample/project round trips, sliced-sample refusal, redo conflicts, and
release after failed/successful document replacement. The native PTSamplerTest
runs the same ownership/journal tests on the Amiga CPU.

`sampler/` retains actual Intuition/ASL workflow evidence: import and reverse an
8-bit mono WAV, audition, stop stale playback on import/edit, fade/undo/redo, invalid
and cancelled imports preserving redo, exact WAV export and existing-file refusal.
It then imports 24-bit stereo, removes DC and undoes it, exports exact PCM, saves
the project, checks signed big-endian sample bytes/CRC independently, reopens and
re-exports/resaves byte-identically. Both editor invocations exit normally.
The native sampler workflow uses keyboard input; new mouse targets and range
selection are covered by the shared controller host test. Full/cached rendering
matches through sampler navigation and selections; the main-screen golden is
unchanged. Screenshots are native output.

The emulator window is coordinated with AmiConnect; the exact private profile,
CPU and memory are retained in each report. Guarded shutdown and fresh
process/private-disk/socket release checks are in emulator-release.txt.

## Remaining scope

This is the first sampler UI integration, not completion of the full handover.
RAW/IFF/MOD sample import, capture, loop/slice tools, waveform zoom, resampling,
explicit precision conversion, additional sample-slot creation and high-resolution
preview remain open. WAV ancillary loop/cue metadata is not imported. The current
Paula audition still requires compatible classic samples; high-resolution data is
editable/saveable without implying playback support. Mixed replay/CAMD and physical
AmiGUS, sound quality and ACA1234 performance qualification remain NOT RUN/open.
