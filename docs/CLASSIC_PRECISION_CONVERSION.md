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
voice allocation, slice expansion, bounce-based reduction and native editor
selection of this conversion policy remain subsequent work. The editor's SAVE
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
