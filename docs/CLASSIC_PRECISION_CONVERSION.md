# Explicit precision-only classic MOD conversion

`PT24GConvert mod8 INPUT.ptg NEW_OUTPUT.mod` explicitly converts otherwise
classic-compatible mono 16/24-bit samples to signed 8-bit PCM. The input project
and its samples remain unchanged. Output is published as a verified new file;
existing destinations are refused. `mod` remains strictly lossless and continues
to refuse higher-precision samples.

This first transformation uses nearest rounding, ties away from zero, followed
by saturation to -128..127. There is no dither, normalization or resampling.
For example, 16-bit +128 becomes +1, -128 becomes -1, and +32767 saturates to +127.
24-bit conversion uses all source bits before the one final quantization.

The shared portable core exposes `pt_mod_export_analyse_round8` and
`pt_mod_export_round8`. It performs complete project validation and classic
compatibility checks before writing. Output must not alias the source. Invalid,
unsupported, undersized or aliased output leaves the destination and byte count
unchanged. The source PCM is borrowed and immutable throughout conversion; no
extra sample buffer or allocation is needed by the core.

Policy analysis retains the precision issue bit and supplies an output size only
when precision is the sole obstacle. The utility prints the chosen rounding
policy, `dither=none`, remaining unsupported issues, and `CONVERTED` when any
sample precision changes. Already-compatible 8-bit input remains `LOSSLESS` and
byte-identical. Sample lengths, rates, loops, finetune, notes and effects are
preserved; their existing classic constraints still apply.

Stereo, changed sample rates, extra tracks, slices, enhanced loops, MIDI,
panning, metadata and other classic-format conflicts remain explicit refusals.
This is not a complete enhanced-project down-converter. Dithering, resampling,
voice allocation, slice expansion and bounce-based reduction remain subsequent
work. Native editor selection of the precision policy is described below. The editor's SAVE
MOD command is still lossless; it never implicitly enables this policy.

The C boundary suite checks 16/24-bit ties and saturation against fixed expected
samples, strict/capacity/rate/alias refusals, source preservation, strict MOD
reopening and unchanged 8-bit re-export. Host tests also verify explicit CLI
selection, source-file identity, new-file publication and existing-file refusal.
The coordinated emulator runner repeats the C and native CLI workflows and runs
the resulting MOD through the pinned 2.3F replay trace diagnostic. That is replay
code evidence; it does not substitute for a complete original-tracker GUI
load/edit/save test or physical A1200/AmiGUS acceptance.

Dev59 validation: 72 host tests passed in 129.943 seconds. A subsequently
strengthened 24-bit low-byte/boundary case passed in 3.002 seconds without product
code changes. Native run `round81789948765056579000` passed six programs in
30.252 seconds, with exact host/native MOD and source-PTG identity and stopped
Paula DMA after the pinned replay diagnostic. An initial runner used Unix `./`
in its fixture-output path; its failed run and guarded release are retained,
and the successful runner uses an explicit Amiga DOS `PTDEV:` path.


## Explicit editor conversion (dev60)

DISK OP -> CONVERT MOD, or Control-Alt-M, opens a separate confirmation panel.
It states `8 BIT ROUND / NO DITHER`, `SOURCE PROJECT KEPT`, and that other classic
limits still apply. EXPORT/Return opens the new-MOD filename requester; BACK or
Escape returns to disk operations without conversion. Ordinary SAVE MOD and
Control-Shift-M retain the strict lossless policy.

The conversion panel does not edit the song or consume undo history. Successful
precision conversion reports `MOD CONVERTED`, retains dirty status, and leaves
original high-resolution samples available for normal PTG saving. Cancellation,
existing-file refusal and unsupported content retain the project and history.
MIDI and other unsupported transformations are refused before a filename request.
No preference silently enables conversion for future ordinary SAVE MOD actions.
The accepted main screen is unchanged; the disk-operation page gains the entry.

Dev60 validation: 72 host tests passed in 129.898 seconds; after shortening new
status text, the shared editor/controller and unchanged main-screen golden test
passed again in 6.094 seconds. Final native run `mod8ui1789949777315862000` passed
in 63.737 seconds: explicit panel/requester cancellation, exact converted MOD
including an unsaved finetune edit, existing-destination refusal, preserved dirty
state and undo, exact original PTG after undo/save, and refusal of external MIDI
and remaining enhanced metadata before a requester. The final panel screenshot
was visually checked. Evidence is under `evidence/enhanced-editor/dev60`.
