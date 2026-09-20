# Explicit RAW sample interchange, dev18

RAW on SAMPLER or X opens settings for 8/16/24-bit, mono/stereo, signed/unsigned8,
byte order and rate. LOAD RAW / L applies those explicit settings. SAVE RAW / W
requires matching sample depth/channels/rate, exports PCM only and shares the
verified new-file writer. Settings changes do not edit sample data or history.

All 27 host groups pass. ASan/UBSan checks cover full signed24 precision, both
endian orders, signed/unsigned8, frame alignment, capacity/alias refusal, no
implicit conversion/autodetection, allocation failures, shared history, rejected
replacement with slice references, redo and serialization. UI tests include mouse
and key settings, number-entry validation/cancellation with empty sample slots,
modal isolation and full/cached pixel identity. The accepted main golden remains.

Native PTRawTest and PTSamplerTest pass. Native requesters reject RAW through the
ordinary autodetect loader, then import 1024 stereo24 frames at explicit 48 kHz
and export both byte orders exactly. The workflow checks mismatched export refusal
before a requester, malformed input preserving redo, requester cancellation,
unsigned8/signed8 exact exports, undo back to the original stereo24 sample and
refusal to overwrite an existing destination. PTG metadata, exact PCM and CRC
validate independently; reopen/resave is byte-identical. Both exits are normal
and DMA is off. Screenshots are actual native output.

The initial cross-build found an incorrect playback-stop helper in the new native
adapter. It was replaced with the existing Paula stop/poll and range-reset path;
the retained final native build and workflow pass. This is software/emulator proof,
not hardware performance, subjective audio quality or AmiGUS acceptance.

Amiberry ownership remains with this ProTracker task across successive validation
runs by explicit AmiConnect handover. Each harness restores launch and quits only
the exact private profile. Final release is coordinated separately.
