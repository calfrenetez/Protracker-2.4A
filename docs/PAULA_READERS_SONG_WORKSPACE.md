# Caller workspace for Paula whole-song construction

Status: **host PASS and compiler-only portability PASS** for constructor source v4, unchanged public header v3 and workspace fixture v2 in candidate-v3. The workspace and original song host groups each passed once; one separate native compiler invocation produced the retained HUNK test without executing it. Separate compiler stack annotations are **PER_FUNCTION_ONLY**: total stack remains **UNKNOWN**, and a 65536-byte stack is **NOT_CLEARED**. Native, emulator and physical-device execution remain **NOT_RUN**. Earlier producer evidence retains its original scope; the saved results below bind this addition.

The optional `pt_paula_readers_song_begin_in_workspace` entry point constructs the same software schedule producer as `pt_paula_readers_song_begin`. It moves the large construction snapshot into ordinary storage owned by the caller. The existing begin entry point remains available and retains its large local snapshot. Neither entry point activates Paula hardware, connects an editor PLAY action, or supplies a native backend.

## Public entry points

The workspace header adds these declarations:

```c
size_t pt_paula_readers_song_begin_workspace_size(void);
size_t pt_paula_readers_song_begin_workspace_alignment(void);
enum pt_paula_readers_song_result pt_paula_readers_song_begin_in_workspace(
    const struct pt_allocator *, struct pt_sampler *, struct pt_project *,
    const struct pt_paula_readers_song_config *, uint32_t revision,
    void *workspace, size_t workspace_capacity, struct pt_paula_readers_song **);
```

Both queries describe this exact compiled build and ABI. Allocate addressable ordinary storage with the returned alignment and at least the returned size; retain the original allocation address if alignment requires an interior address. Pass the entire capacity being lent to the constructor. Spare bytes belong to the guarded extent. Do not replace the queries with a byte count from a different build.

Set the ordinary output handle to NULL before begin. Every required nonempty master must already have a genuine matching current sampler version. Prepare those versions before any producer or render sequence borrows the project. The producer never promotes a missing master after borrowing starts. Project, sampler, masters and all borrowed tables, markers and extensions remain alive and immutable through producer close, apart from the documented valid selection cursor change.

## Scratch ownership and refusal

The caller owns the scratch independently of the producer allocator. Budget it separately from `config.control_budget`, which covers producer-created ordinary controls, and from `readers.chip_budget`, which covers representations. Combined peak storage includes all three. Ordinary storage does not demonstrate Fast placement, and a representation allocator declaration does not demonstrate Chip placement.

Scratch is exclusively borrowed through the complete begin call and every callback it invokes. Its whole declared capacity must be disjoint from source and control extents, the output slot, the explicit backend context, and opaque callback contexts. All bytes must remain unchanged by the caller and callbacks while the call is active. Calls using the same scratch must be serialized, including a first allocator callback before a public producer owner exists. The API does not accept caller-written readiness fields, copied prepared owners or validation certificates.

For an otherwise valid aligned buffer shorter than the queried size, begin returns `PT_PAULA_READERS_SONG_CAPACITY`. NULL, misaligned, wrapping or recognized overlapping scratch extents return `PT_PAULA_READERS_SONG_INVALID`. Constructor metadata and known alias guards precede scratch writes and allocator/backend callbacks. These initial refusals preserve scratch, source and output. Once the private snapshot has been populated, scratch contents are unspecified on return.

A returned arena recognized as overlapping a guarded extent is not released as fresh ownership. Allocators must return fresh, disjoint storage; unenumerated opaque extents remain caller responsibility. Nested child allocation refusals can surface as `CAPACITY` because the interposed allocator returns NULL. Do not treat a refusal result as evidence that an ambiguous arena acquired ownership. Observable callback mutations also refuse publication; they violate the caller's immutable-input contract rather than provide a supported editing mechanism.

A successful call returns `PENDING` with a genuine owner and its actual opaque preflight setup. That result is separate from semantic validation, Paula compatibility and backend adoption. No pointer into scratch is retained after return. Immediately overwrite, reuse or free it after a successful or refused call; it is not needed for step, publish, service, cancel or close. Keep the copied callbacks' original contexts alive through owner close. The original allocator/config argument objects need live only through begin.

## Continuing the genuine producer

Use the existing public lifecycle after either constructor:

1. Step bounded actual startup, retain the immutable current masters, run the actual compatibility audit and transfer its same sequence, then drive lookahead and boundary preparation.
2. Call `publish_next` explicitly when the original boundary is ready. Successful submission is distinct from actual adoption and activation.
3. Service retained command and reader record indices independently using genuine backend proofs. Command detachment cannot substitute for reader retirement, and old reader retirement cannot retire a replacement.
4. Preserve the original absolute interval and deadline frames through pressure, delayed ACTIVE and explicit acceptance retries. No rebase or catch-up is added.
5. Treat `DONE` as the end of schedule production. Musical STOP is a separate explicit terminal request. Readers remain owned until independent retirement and command-reference release permit closing them.

Command capacity remains two and reader capacity remains eight. The copied configuration is immutable. Service proven domains or make allocator space available within the existing budgets to relieve pressure. An insufficient configured budget requires cancel, independent drain and close, followed by a separate begin with an adequate configuration; it does not authorize changing the live owner or rebasing its deadlines.

Cancellation stops production without implicit musical STOP, receipt polling or forced release. Supply a non-NULL close handle only when its value is a live owner genuinely returned by begin. The handle slot must itself be disjoint from the owner's guarded storage. A failed close does not prove retirement. Explicitly drain command and reader domains before completing close; callbacks remain valid through their final releases.

## Written acceptance and limits

The separate workspace fixture retains every current v5 genuine case and its oracle/backend helpers, with inherited constructors routed through live aligned heap scratch. The wrapper starts with dirty scratch and overwrites/frees it before semantic steps. It then exercises actual preflight, same-sequence transfer, lookahead, original boundaries, mono 8/16/24 masters, four triggers, more than twenty continuing controls, capacity two/eight, pressure, delayed ACTIVE, replacement, independent retirement, explicit terminal STOP and zero final ownership. The workspace group and the unchanged original song group passed once under the recorded C99 strict-warning, ASan and UBSan recipe. The workspace runtime emitted all three exact markers; the original group emitted its two existing markers. This qualifies the exercised host software behavior described here, without extending it to untested geometries, all alias/callback combinations, native stack use or a device backend.

Focused new cases cover short/NULL/misaligned/wrapping capacity, full spare-capacity overlap with allocator/config/output/backend controls, genuine owned PCM unused capacity, a live source output slot, returned allocator aliases into used/spare scratch, callback fixed-header/config mutation, genuine allocation failures, exact-size reuse and a separate compatibility-begin smoke. Fixture v1 had one incorrect nested stale-header expected result found by source review before execution. Fixture v2 changes that expectation and comment only; v1 is retained as unexecuted source, not classified as an observed runtime failure.

The tests do not exhaust every table, marker, extension, version and opaque-context alias, every callback mutation, stereo or rate 44100. Close through arbitrary unused PCM padding remains uncovered; the inherited close alias uses a valid typed actual parent handle slot. The recorded host sanitizer run exercised immediate scratch free before the genuine lifecycle. This can detect exercised escaped dereferences, but cannot prove absence of unused numeric references on all paths.

The workspace entry point removes the large local ledger/sample snapshot from its common constructor. The separate saved compiler annotations report local frames of 16 bytes (`static`) for the public workspace entry point and 124 bytes (`dynamic,bounded`) for its private common constructor. The compatibility begin reports its own 70,352-byte frame (`dynamic,bounded`), exceeding 65536 bytes before callees or its caller are considered. The annotated fixture main reports 844 bytes, and other inherited fixture functions also have large frames. These are individual compiler-reported frames; they are not summed into an asserted call-chain bound. No cumulative callback-stack or runtime high-water measurement is supplied. The full workspace fixture deliberately includes compatibility smoke and retains the large-stack execution gate. Total stack is **UNKNOWN** and a 65536-byte launch stack is **NOT_CLEARED**. Native runtime, timing, IRQ, DMA, audio and physical-device acceptance require separate evidence and authorization.

## Saved host result binding

The root saved-byte audit `root-host-audit-v3.json` is PASS, with SHA256 `4b6ceb2488a9421e0cd79f3e14bd8dbe78571b18c1b125e45b759588e01653f5`. It binds `host-final-v3/host-checks.json` (SHA256 `dcc7ba6014779c1a75d2f76f54c01bc3a2e8c8c5e19bf4419f2c496f1e2ecceb`) and `frozen-source-host-v3.json` (SHA256 `6f23403814b3835d2d9fd5838acac8c8f06bcf52d8f8d5756599bd18121ac27d`). The two groups recorded four zero-return compiler/runtime commands, 58 full dependency queries, 80 canonical and 105 system inputs, two retained products and no generated inputs. The documentation author read saved data only and performed no rerun.

The tested constructor C has SHA256 `576f0ea6461d31d55a32606b1535a7c503b8c343a317c79accaaca1e94bb597d`; its header has SHA256 `5bc7540e93e52177332344d292172bdaa931d10c7f436767c8f8439a6019aaf7`. Workspace fixture v2 has SHA256 `f4b0f5fbadc33c8c142efdae31164a55d46805337290f028d1e39e784acaca28` and its recipe has SHA256 `ec1ad2c5936fb9b2a6602595dbdf6003403777c3214dee42841fc672c0b78095`. This result belongs to that closure, not a later source revision.

Independent saved host proof review is PASS for host software evidence only: `independent-saved-host-proof-review-v3.json`, SHA256 `0e68f46b590222b2d8881033ca78d2025cc5800619e94eabb26e45efb94fa668`. The review used saved bytes and did not rerun the compiler, fixture or product.

## Saved native compiler and annotation bindings

The separate native compiler helper ran once and recorded 40 zero-return commands, 29 full dependency queries, 79 canonical and 32 SDK inputs, and the exact seven pinned runtimes. The retained HUNK `PTPaulaReadersSongWorkspaceTest` is 273684 bytes, SHA256 `337a1bdf2e479da75443d06ac83156b995db7acd5b62989849cf520125b344e4`. It was **never executed**. Its compiler-only manifest is `native-final-v3/manifest.json`, SHA256 `faee9a20a25cd419d4d4a35e5efbe45049a280fe2ad07945a2881dc3fa72500f`, bound by `root-native-audit-v3.json`, SHA256 `a55e3eb6c9fa981736e9a1640534732048c3db988a192e0e3f1dd0806b7f4457`. Independent saved native proof review is PASS for compiler-only portability evidence: `independent-saved-native-proof-review-v3.json`, SHA256 `26a4a8fc1a7def5d34b9f4420635fbbaff2975df26550f0cdb1e351cae0f51b3`.

An initial pure native-proof inspection compared raw GCC pathname spelling against normalized saved SDK paths. Its assertion was corrected to compare resolved paths; actual SDK files and pinned hashes were exact. `independent-native-proof-inspection-refusal-v1.json` preserves that review history. No compiler failure, product failure or product retry occurred because of that inspection correction.

The separate first/once annotation lane recorded 131 zero-return commands across 29 translation units and 627 compiler function rows. It performed no link or product execution. `stack-final-v3/manifest.json` has SHA256 `f3314dde1c39002516c885c866ed21ba148ce850eda61d2e46a331b73ba61032`. No dynamic unbounded rows were reported, which does not establish bounded total stack: indirect callback/callee edges, cumulative frames and runtime reserves remain separate unknowns. The frame observations above are **PER_FUNCTION_ONLY**. Annotation saved-byte review is **CLEAR SAVED-BYTE INTEGRITY ONLY**: `independent-stack-annotation-review-author-v1.json`, SHA256 `98f8a6363dfded0571967c489e964a571706033b07013c4e76178b0bc88707a4`. It reconstructed the 131 zero-return commands, 29 translation units, 627 rows, 79 canonical/32 SDK inputs, exact seven runtimes, all 320 recorded artifacts and 321 durable mappings against the 8122-file source snapshot. It leaves total stack **UNKNOWN**, 65536 bytes **NOT_CLEARED**, and native execution **NOT_RUN**.


Additional saved local frame observations are:

| Emitted function | Reported own frame, bytes | Compiler qualifier |
| --- | ---: | --- |
| `pt_paula_readers_song_begin_in_workspace` | 16 | `static` |
| `song_begin_in_workspace` | 124 | `dynamic,bounded` |
| `song_source_spans` | 128 | `dynamic,bounded` |
| `pt_project_validation_begin` | 2192 | `dynamic,bounded` |
| `pt_flow_begin` | 2332 | `dynamic,bounded` |
| `pt_render_sequence_setup_begin` | 2876 | `dynamic,bounded` |
| `pt_render_sequence_setup_take` | 2900 | `dynamic,bounded` |
| `pt_paula_readers_song_begin` | 70352 | `dynamic,bounded` |
| Fixture `main` | 844 | `dynamic,bounded` |

Each row describes that emitted function, without its callees. No aggregate is asserted. The new API continues real validation and preflight work, while the full fixture still calls the large compatibility constructor. Neither small entry-point rows nor zero dynamic-unbounded rows clear a complete call chain, callback depth, IRQ reserve or launch stack.


Current results: `HOST_WORKSPACE_RESULT=PASS`; `HOST_LEGACY_RESULT=PASS`; `NATIVE_COMPILE_RESULT=PASS_COMPILER_ONLY`; `STACK_ANNOTATION_RESULT=PER_FUNCTION_ONLY`; `TOTAL_STACK=UNKNOWN`; `STACK65536=NOT_CLEARED`; `NATIVE_RUNTIME_RESULT=NOT_RUN`; `EMULATOR_RESULT=NOT_RUN`; `PHYSICAL_DEVICE_RESULT=NOT_RUN`. Compilation and annotations cannot fill any runtime, timing or complete-stack result.
