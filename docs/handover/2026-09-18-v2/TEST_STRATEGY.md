# TEST STRATEGY

Use a two-tier pipeline.

## AmiBerry gate
Fast automated smoke/regression: build, launch, MOD compatibility, pattern/sample editing, 16-channel model/UI, save/load, PP20, RAW/IFF/WAV, undo/recovery, slicing/auto-slicing, rendering/bounce and non-hardware MIDI/input logic.

## Real A1200 gate via AmiConnect
Automatically deploy and test hardware-sensitive builds on the existing A1200/ACA1234/AmiGUS mini setup. Inspect the actual AmiConnect interface before scripting it.

Validate detection/resource lifecycle, interrupts, 1/4/8/16 voices, panning, pitch, loops, interpolation, PCM, Studio 24/48, recording, injected Safari mouse/control, responsiveness and endurance.

Build AmiGUSTest first. Record build ID, target, emulator/hardware, test, duration, PASS/FAIL, counters/logs and relevant output hashes/sizes.

Run progressive Studio voice-count benchmarks and ~60-minute 16-channel endurance tests once stable. Never auto-flash FPGA firmware during routine tests.
