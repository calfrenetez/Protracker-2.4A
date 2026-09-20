# PT24G project format 1.0 — development specification

This format is independent of the executable's version string. The implementation
is shared by the portable project core and native PT24GConvert. It is not yet
wired into the original four-channel assembly editor. All integers are unsigned,
big-endian unless explicitly stated. There are no native pointers, compiler
struct layouts or hardware register values on disk.

## File envelope

The header occupies 32 bytes:

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 8 | `PT24G` followed by CR, LF, byte 26 |
| 8 | 2 | Major version, currently 1 |
| 10 | 2 | Minor version, currently 0 |
| 12 | 4 | Exact total file length; trailing bytes are rejected |
| 16 | 4 | Required capabilities, derived from the actual project |
| 20 | 4 | IEEE CRC-32 of the whole file with these four bytes zeroed |
| 24 | 4 | Chunk count, maximum 4096 |
| 28 | 4 | Reserved, must be zero |

Each chunk has a 12-byte header: four-byte ID, two-byte chunk version, two-byte
flags, four-byte payload length. Flag bit 0 means required; other flag bits are
unsupported. A chunk's payload is followed by zero padding to a four-byte
boundary; the payload length excludes padding.

Readers reject unknown required chunks/capabilities/versions. Unknown optional
chunks are preserved, including their ID, version and payload, when loading and
saving through the project model. Their order relative to known chunks may
change on canonical write. Current readers accept envelope 1.0 only. Future
readers must continue to load 1.0; incompatible field reinterpretation requires
a new explicit version and documented migration.

Exactly one of each known required version-1 chunk must be present. Duplicates,
missing chunks, invalid padding, invalid references and inconsistent capability
bits fail validation before any destination memory is changed.

## Required chunks

| ID | Payload |
| --- | --- |
| HEAD | 44 bytes: title[32], channel count u8, selected channel u8, initial speed u8, mode u8, BPM u16, order count u16, pattern count u16, sample count u16 |
| CHAN | One 22-byte record per active channel: route, pan, mute, solo, group, MIDI channel (one byte each), name[16] |
| ORDR | One u16 pattern index per order |
| PATT | 12-byte events in pattern, row, channel order; always 64 rows per pattern |
| SAMP | Sample records described below, in instrument order |
| MIDI | Input endpoint[64], one output endpoint[64] per active channel, u32 flags (bit 0 send clock, bit 1 receive clock) |

Names/endpoints are fixed byte fields with at least one NUL within the field.
The current model treats them as Amiga byte strings, not UTF-8. Trailing field
bytes are preserved. Routes are exactly 1 Paula, 2 AmiGUS, or 4 MIDI. There are
1–16 channels, at most four Paula routes, 1–256 orders, 1–256 patterns and 0–255
sample slots. Group numbers are 0–15; MIDI channels are 1–16; pan is 0–255.
Speed is 1–31, BPM 32–255, mode 0 wavetable or 1 Studio. Unsupported facilities
can remain unavailable while the project remains editable; routes are not
silently substituted.

An event is: kind u8, instrument u8, pitch u16, classic effect u8, parameter u8,
velocity u8, flags u8, slice u16, reserved-zero u16. Kind is 0 no note, 1 raw
classic period (1–4095), 2 MIDI note (0–127), or 3 explicit OFF. No-note and OFF
have pitch zero. Effect is 0–15; ECx remains an independent classic effect.
Instrument zero means unchanged; other values index sample slots starting at 1.
Flag bit 0 indicates explicit velocity (0–127); absent velocity is stored as zero.
Slice zero means none, otherwise it is a 1-based marker index and the event must
name an explicit valid instrument. This avoids dependence on preceding events
when interpreting the stored slice reference. Future non-MIDI OFF behaviour is
not specified by this format implementation.

## Sample records

Each sample starts with 64 bytes:

| Offset | Field |
| --- | --- |
| 0 | Name[32] |
| 32 | Sample rate u32, 1–192000 |
| 36 | Frame count u32 |
| 40 | Precision u8: 8, 16 or 24 |
| 41 | Channels u8: 1 or 2 |
| 42 | Default volume u8: 0–64 |
| 43 | Signed finetune byte: -8–7 |
| 44 | Loop kind: 0 none, 1 forward, 2 ping-pong, 3 crossfade |
| 45 | Interpolation preference: 0 or 1 |
| 46 | Slice-marker count u16, maximum 4096 |
| 48 | Loop start frame u32 |
| 52 | Exclusive loop end frame u32 |
| 56 | Crossfade frame count u32 |
| 60 | Reserved u32, zero |

The header is followed by u32 slice start frames, then interleaved signed PCM
samples packed to their declared bit width, then zero padding to a four-byte
record boundary. Eight-bit PCM is signed here, unlike unsigned eight-bit WAV.
Twenty-four-bit values retain all low bits. Decoding expands values to signed
32-bit integers without changing their precision or sample rate.

Slice markers increase strictly and are less than the frame count. A slice ends
at the next marker or the sample end. They share the original sample data.
No-loop records have zero loop endpoints/crossfade. Other loops require
0 <= start < end <= frames. Crossfade length is positive and at most half the
loop length; non-crossfade loops have crossfade zero. These are stored semantics;
playback/rendering implementations must separately implement and test them.

## Required capability bits

Bits 0–12 respectively describe: more than four channels, AmiGUS route, MIDI
route or clock, 16-bit samples, 24-bit samples, stereo samples, slices, ping-pong
loops, crossfade loops, OFF events, Studio mode, explicit velocity, MIDI note
values. The writer computes these; the reader requires an exact match to the
project content. Endpoint preferences alone do not require an active MIDI backend.

## Classic MOD preservation and export

Strict MOD import retains the original 1084-byte header in the optional `CMOD`
version-1 chunk. This preserves inactive orders, restart byte, exact names and
legacy non-loop header values while editable fields remain in the shared model.
The nominal sample-rate metadata for PAL period 428 is 8287 Hz; raw event periods
and finetune remain separate. Sample bytes are not resampled on import.

PT24GConvert currently provides direct export only when the analysis finds no
loss. MIDI audio, OFF, extended precision/rates/stereo, slices/loop types,
nonclassic panning, metadata and classic limits are reported. CONVERTED, BOUNCED
and INCOMPLETE in analysis are the **required strategies**, not claims that those
transformations have already been performed. A nonzero issue mask prevents the
direct writer from modifying its output. Transforming/bouncing strategies and
in-editor export remain subsequent work.

## Loading and save transactions

Probe validates the complete file and computes storage requirements without
allocation. Decode requires separate, correctly sized staging buffers and checks
for overlap before writing any of them. Unknown optional bytes are copied into
owned staging storage, so the input buffer can be released after success.

The document wrapper budgets the candidate, handles each allocation failure,
and swaps it in only after successful decoding. The previous project and dirty
state survive any failure. Peak memory includes the previous project, candidate
and input bytes; the caller supplies policy and available-memory information.

The save state machine creates staging, handles partial writes, finishes/closes,
reads back the expected content, then publishes. Failed phases abort only owned
staging. PT24GConvert always creates a new destination and refuses existing
paths. On POSIX it uses an exclusive staging file and no-replace hard-link
publication. On Amiga it reserves a unique directory with DOS CreateDir, writes
inside that directory, then uses DOS Rename, which the native test must prove
refuses an existing destination. It does not rely on the local C library's
O_EXCL/errno behaviour. Editor overwrite/recovery adapters remain to be built;
there is no power-loss durability claim from these ordinary failure tests.

## Format recognition and MOD titles

Document loading and MOD sample-source import compare all eight project-magic
bytes, including CR, LF and byte 26. The five printable characters `PT24G` alone
are a legitimate start to a MOD song title and do not identify an enhanced
project. Truncated or corrupt input still passes through the selected format's
full validation before the current document or sample source is replaced.
