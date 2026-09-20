# IFF/8SVX sample interchange, dev17

LOAD SMP autodetects PCM WAV or bounded IFF 8SVX. Single-octave signed8 mono and
Fibonacci-compressed input retain name, rate, rounded volume, forward loop and
all PCM including trailing samples after the loop. SAVE IFF on LOOPS, W there,
or Shift-W on SAMPLER exports supported data without implicit conversion.

## Evidence

All 26 host groups pass, including sanitizers. Independent hand-authored IFF
fixtures cover endian headers, odd padding, signed limits, loop-tail retention,
Fibonacci signed wrap, mono CHAN, unsupported stereo/octaves/compression,
truncation, malformed lengths, duplicate critical chunks, capacity and aliasing.
A bounded mutation corpus and allocation-failure tests preserve destination,
project and history. Native PTSvxTest and PTSamplerTest pass on the final build.

The actual native requester imports an odd-sized 4097-frame sample with a name,
48/64 volume and forward loop 128–3072. Independent host inspection verifies the
exported FORM chunks and exact PCM, refused pingpong export before a requester,
malformed import preserving redo, exact four-frame Fibonacci decode in WAV,
existing destination preservation, PTG metadata/CRC and byte-identical reopen.
Both editor exits are normal and DMA is off. Screenshots are actual native output.

The first workflow attempt was interrupted through guarded cleanup after a test
expectation incorrectly waited for SAMPLE UPDATED for a loop command; the editor
correctly reported LOOP UPDATED. The corrected retained workflow passes. No
product change was required for this test-script failure.

Amiberry was explicitly reserved from AmiConnect, using the private exact-match
profile. The harness restores launch and quits that profile. Ownership is retained
for successive ProTracker checks, per AmiConnect's explicit handover, until release.

## Limits

IFF export requires mono8 at <=65535 Hz, no finetune/slices, no loop or forward
loop. Name is bounded to 31 bytes. IFF 16.16 volume rounds to 0–64 on import;
pitch/octave hints, channel-side hints, envelopes and annotations are not retained.
Export is uncompressed, and project state is unchanged. This is emulator/software
acceptance, not physical sound quality, enhanced preview or AmiGUS acceptance.
