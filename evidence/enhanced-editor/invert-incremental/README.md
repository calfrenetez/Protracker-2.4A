# Incremental classic EFx preparation — host and cross-build evidence

This package records nine successful host ASan/UBSan groups, three pinned native cross-builds and three exact shared030 native software qualifications. All three native fixtures passed with RC0. Native evidence is injected software/ownership qualification; physical-device, realtime timing and human listening acceptance remain separate.

The tested candidate is a clean commit `2470f68` source/tree export plus exactly 21 EFx core, sampler/editor, test and scoped-document overlays, now present in product commit `0adacb2`. `source-snapshot.json` is explicitly derived metadata: it retains base commit/tree, the 21 overlay hashes, two new inputs and nine excluded dirty-file checks. The original 2,304,789-byte source manifest and its SHA256 remain referenced; that full 7443-input manifest is deliberately not duplicated here. Dirty display/editor/test-helper/tool work was excluded from the source snapshot.

## Successful host evidence

`host/core-host.log` records six passing groups: private PCM, private bank, synchronous offline EFx render, incremental preparation, queued EFx session and renderer sequence. The final immutable-export run finished in 14.201 s. Existing PCM oracles, classic first-word behavior, repeat/retrigger/delay, synchronous compatibility, budgets, all initial allocation failures, alias refusal, phase cancellation and complete master comparisons remain covered.

`host/host-checks-v2.json` and its log record three further passing groups: sampler EFx, editor EFx and ordinary queued editor Studio. Their unittest runtime was 23.324 s; the provenance harness elapsed field also includes logging work. That evidence retains exact compile/runtime commands, 105 canonical dependencies, host compiler/sanitizer/system-input hashes and before/after frozen-input verification. The ordinary Studio group retains direct24 byte parity and reservation/queue lifetime checks. Its FIFO transport is a fake callback boundary, not a physical-device qualification.

## Native cross-build evidence

`cross-build/manifest.json` records pinned m68000/soft-float/nix20 compiler and runtime hashes, ordered translation units, canonical transitive dependencies and binary bytes/SHA256 for PTExecInvertSessionTest, PTExecSamplerInvertSongTest and PTExecEditorInvertStudioTest. `compile-commands.json`, three build logs and the original byte-copied helper preserve the commands. Included fixture C files are dependencies of their native wrappers and are never compiled twice. The session wrapper includes the bounded preparation fixture and preserves its existing safe RC20 assertion handling.

Their compiler logs are empty successful outputs; zero bytes are intentional. Root subsequently ran all three exact candidates through the shared030 harness. `native/qualification-1790987821163521000/qualification-result.json` preserves the top-level qualification record; each separate session/sampler/editor directory retains its own `result.json`, native/launch logs, guest identity and independent cleanup record. Top-level and nested results have distinct destinations.

The session, sampler and editor fixtures returned RC0 and recorded 169, 454 and 278 Fast allocations respectively, with zero final owned bytes. Every case completed exact-run cleanup and a separate 11-observation, 10-second absence window. Original emulator PID 19081 remained running and all four Paula DMA channels were off. Existing 120-second fixture deadlines remained unchanged. Root sent RELEASE and reports AmiConnect acknowledgement revision 26 with no reservation or recovery hold. The acknowledged coordination report is recorded in `native/summary.json`.

The byte-copied `historical-qualify_incremental_invert.py` is the actual executed orchestration source, retained as historical evidence rather than a portable command. Packaging itself executes no native fixture. These checks establish software PCM/ownership/preparation behavior in the shared030 guest; they do not exercise AmiGUS sample RAM/device output, physical hardware, realtime audio timing or human listening.

## Implemented software bounds and remaining acceptance

EFx still supports the explicit classic mono8 one-shot/forward-loop subset. Authoritative masters remain unchanged; the private output mixer produces 48 kHz stereo 24-bit PCM without enabling 16/24-bit EFx samples. Initial project/PCM validation, static selection and allocations remain synchronous. Each subsequent prepare performs one copy setup, a copy of at most 4096 PCM bytes, complete-bank publication/private first-word initialization, or measurement of at most 256 timeline ticks. The final first-word clearing touches at most 2040 private bytes. Fully copied and measured readiness precedes playable pull. Cancellation frees private sample/sequencer state at every phase, while the retained controller still requires close.

Pending pull emits no PCM. Readiness/handle outputs overlapping immutable inputs or owner/private state refuse before publication; generation/shared-header guards remain in the sampler/editor owner wrappers. The nine host groups and native software qualifications do not resolve the pending strict scheduled-output versus permitted-lateness decision, or physical AmiGUS RAM capacity, upload completion/ordering, voice-stop, sound or realtime timing acceptance.

## Preserved logging failures

`failures/superseded-gvbbj8_f` preserves the first dependency-continuation parsing failure. `failures/final-parser-v1-u7z20nyz` preserves the second logger failure on external macOS SDK metadata. Each pass compiled all three groups with return code 0, then failed while collecting dependency provenance before running any product fixture. Neither contains a product assertion failure. Their JSON/log/helper/raw dependency outputs remain exact byte copies. `failures/summary.json` records this distinction; successful v2 host evidence supersedes both logging passes without rewriting them.

`copied-files.json` records the original source, destination, SHA256, byte count and byte-equality verification for every copied file, and labels generated summaries separately. Packaging performs no source edits, staging, commit, push, target access or qualification-script changes; native records are byte copies of the completed root qualification.
