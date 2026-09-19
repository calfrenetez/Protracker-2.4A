# Updated handover review

> Historical intake review. Subsequent baseline build/test evidence is recorded
> in [BASELINE_BUILD.md](BASELINE_BUILD.md); the observations below describe the
> repository at handover intake.

Processed 19 September 2026. This is a preparation review, not an implementation
or hardware acceptance report.

## Input and authority

The supplied archive is
`ProTracker-2.4G-FINAL-FINAL-Definitive-Codex-Handover-v2-2026-09-18.zip`.
Its SHA-256 is
`ba1a6578049116d411571907e5b6aeceb119b4e391d8333909e942aba469a963`.
ZIP integrity passed. All nine text documents were read and the PNG was visually
inspected. There is no application source, SDK, toolchain, executable or test
automation in this archive.

All ten original entries are preserved unchanged in
[handover/2026-09-18-v2](handover/2026-09-18-v2/README_FIRST.md).
[MANIFEST.json](handover/2026-09-18-v2/MANIFEST.json) records sizes and SHA-256
hashes for each entry. The manifest is generated metadata, not a supplied file.

The latest document set identifies `FINAL_SCOPE_2.4G.md` as its single scope
source. This review uses it to replace conflicting earlier planning assumptions.
Claims in the documents about prior agreement, existing automation or supported
hardware remain claims to reconcile or verify; they are not evidence of working
features or permission to execute the embedded kickoff.

**Naming confirmed on 19 September 2026:** the user adopted **ProTracker 2.4G —
AmiGUS Edition**, based on native 2.3F, and explicitly requested keeping the
`calfrenetez/Protracker-2.4A` repository address. This replaces the earlier 2.4A
product-name decision without renaming the repository.

## Material changes from the earlier handover

| Area | Updated requirement and consequence |
| --- | --- |
| Baseline | Still native Amiga 2.3F. Add an early 2.3FC source/licence audit and hardening phase; retain authentic compatibility quirks. |
| Routing | Each of 1–16 tracker channels selects exactly one of PAULA, AMIGUS or MIDI. At most four Paula assignments, on any logical channels; reject a fifth. Ordinary MODs default to four Paula channels. |
| Hardware | Add full-size Zorro AmiGUS and a practical 68000 hardware-voice path alongside the A1200/ACA1234/50 MHz/Mini primary target. Optional facilities must fail gracefully. |
| Presentation | Classic 320x256 and Enhanced 640x512; four columns at once, bank navigation, visible P/A/M indicators and optional semantic pattern colouring. |
| Sample workflow | Direct recording; processing; forward, ping-pong and crossfade loops; nondestructive slices and automatic transient slicing. Preserve genuine 24-bit masters. |
| MIDI | CAMD routing, controllers, quantised recording, Hold Record, explicit OFF events, endpoint selection and reconnect handling. |
| Editor reliability | Broader undo/redo, recovery snapshots, safe saves, Recent Projects, track naming/grouping, block operations, optimiser and duration. |
| File support | Hardened MOD/PP20/RAW/IFF/8SVX/WAV handling plus a versioned enhanced format with capability metadata. |
| Rendering | Song/pattern/track/stem rendering, bounce to sample and selected-range support where practical; explicit conversion before wavetable use. |
| Classic conversion | Shared portable core for in-app Amiga export and a standalone Amiga utility; later Mac front end. Report LOSSLESS, CONVERTED, BOUNCED or INCOMPLETE results. |
| Input | Inspect AmiConnect/Safari integration and unify physical/injected input; test down/up, hold, drag and stuck-input recovery. |
| Verification | Establish AmiBerry and real-A1200 automation early, build AmiGUSTest, run progressive voice tests and eventual endurance/human validation. |
| Maintenance | Pin dependencies; check official AmiGUS releases at explicit checkpoints; maintain separate Zorro/Mini matrices. No automatic firmware flashing. |

The latest three additions are specifically Recent Projects, explicit MIDI OFF,
and CAMD input/output selection. No other OctaMED feature is implied.

The earlier single enhanced-output assumption is superseded by mixed exclusive
routes. The 16-channel hardware-voice delivery priority remains. Studio is a
separate measured 24-bit/48 kHz PCM path and must not delay that priority.

## Visual-reference assessment

The reference shows the intended grey/black ProTracker panels, chunky type,
four readable columns, scopes and four bank buttons. It is a concept, not an
implemented screen or pixel-accurate asset specification.

Despite its filename, the PNG is **1402x1122 pixels**, not 640x512. It includes
conceptual text/layout artifacts and 2.3F branding. Derive a deliberate 640x512
layout with correct labels, route letters, hexadecimal row display and measured
Chip RAM/display cost; do not blindly resize the image into application assets.
Retain the original for provenance and review.

## Decisions and investigations before dependent implementation

These are engineering work items, not extra features or requests to decide every
detail now.

1. **Name (resolved):** use 2.4G for future executable branding and public release
   naming; keep the existing repository address and original archive filenames.
2. **Reproducible baseline:** pin exact native 2.3F source and tool versions;
   verify assembler dialect, dependencies, resources and licensing. Locate
   2.3FC source and licence before incorporating changes; record unavailable
   source as an audit limitation, not a reason to copy binaries or guess patches.
3. **Automation availability:** the package asserts existing AmiBerry and
   AmiConnect facilities. Locate actual installations, configs, ROM/OS assets,
   protocol and recovery route; prove one bounded upload/run/result cycle before
   relying on them. Neither tier has been exercised by this review.
4. **Studio and routing:** define how Studio PCM processes AMIGUS-routed channels
   while preserving exclusive P/A/M routing and synchronisation. Specify which
   sample/loop features hardware mode supports, which require explicit prepared
   derivatives, and which require Studio. Do not silently switch quality or route.
5. **Hardware capabilities:** inspect the public library ABI, ownership and
   interrupt lifecycle, Mini/Zorro bus details, PCM/capture formats and supported
   loop/interpolation operations. Recording 24/48, bidirectional loops and Studio
   capacity require actual capability checks and physical proof.
6. **MIDI ownership and timing:** define note ownership, shared endpoint/channel
   conflicts, pitch-bend range, stop/mute/seek/disconnect cleanup and deterministic
   global-effect ordering. Define same-row note/OFF quantisation collisions and
   clock source. OFF is separate event data; preserve classic ECx behaviour.
7. **Model and format:** specify sample/frame/loop widths, pitch reference,
   sample-offset units, slice mapping, stereo policy, explicit event kinds,
   endpoint identity and required/optional chunks. Design reusable parser/effect/
   rendering interfaces for the converter before committing a file layout.
8. **Conversion limits:** establish exact target 2.3F MOD limits and replay
   semantics from the pinned baseline. Make lossy choices explicit; external MIDI
   sound needs captured audio. Offline renders/stems must report missing MIDI
   audio too, not silently imply a complete master.
9. **Budgets and acceptance:** determine minimum OS/memory/display requirements,
   lower-end feature budgets and Enhanced interlace/display behaviour. Define
   measurable timing, memory, underrun and responsiveness thresholds before
   declaring a performance gate passed. Keep human listening/UI review separate.

Carry forward the earlier engineering risks as investigations: 24-bit address
assumptions, word-sized counters/DBRA, large-sample positions, self-modifying
code/cache handling, and sample mutation/cache coherence for effects such as
invert-loop. Do not treat proposed fixes or sample-offset conventions as already
approved format semantics.

## Current evidence

| Item | Verified state |
| --- | --- |
| Input package | Integrity passed; nine documents read; visual inspected. |
| Destination repository | `calfrenetez/Protracker-2.4A`; initial main commit `0a6d0fd47d8d0b2ad6702ce44ed9d1812c4ae453`; README only at inspection. |
| GitHub access | Repository readable; authenticated account reports ADMIN access. |
| Source/build | No source imported or compiled; no toolchain established in this review. |
| Emulator | No launch, regression or screenshot acceptance performed. |
| Real hardware | No card detection, audio, capture, deployment, endurance or performance test performed. |
| Product features | Requirements only; no implemented-feature claim. |

The next implementation gate is the unmodified native 2.3F build and recorded
compatibility baseline. See [BUILD_PLAN.md](BUILD_PLAN.md).
