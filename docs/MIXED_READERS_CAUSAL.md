# Two queued mixed TRIGGER commands

The opt-in `mixed_readers_causal` owner prepares one mixed Paula/AmiGUS TRIGGER
and one later TRIGGER before the first deadline. The successor depends on the
first command's actual successful commit, adoption, original clock window and
complete 20-slot registry. A prediction alone cannot activate a reader. Failure,
uncertainty, cancellation, late completion or wrong fire order suppresses the
successor and retains resources until independent quiet proofs permit release.

This is a separate owner and port contract. Existing `mixed_readers_activation`
behavior and the one-armed native RAM diagnostic retain their contracts. The
shared queue adds a task-only validator; every existing entry and validator body
is preserved byte for byte. Classic playback and the accepted layout are unchanged.

## Ownership and bounds

The task-side admission seam checks the genuine queue, command, reader and holder
identities under exclusion. The owner and port independently derive the predicted
registry from retained original intent. Fire reads copied immutable metadata and
retained readers; it does not traverse expired command holders or synthesize
ACTIVE/adoption receipts. After actual first completion, a small tombstone retains
identity, keys and completion diagnostics, allowing the first command holder to
dispose before the second fire.

Each lifetime bounds storage to two commands and 32 readers. Both publications
must finish before the first in-window fire. Only one pure TRIGGER successor is
supported; a third admission, rolling refill and scheduled CONTROL/STOP refuse.
Administrative stop preserves queue cancellation. Cancellation includes both
commands and all expected reader keys. Source shutdown is attempted once, with
subsequent close requiring an independent read-only exact-registration quiet proof.

## Validation and remaining work

Three isolated host sanitizer groups pass: 26 genuine two-TRIGGER cases, the
existing genuine mixed queue fixture and the default activation fixture. Twelve
successful cases cover 8/16/24-bit masters, 8/16-bit caches and both command/reader
release orders. Negative cases cover wrong dependencies, capacity, partial or
malformed commit, lateness, reentry, adoption and unknown successor quiet. Master
and save beforeimages survive; the disposed predecessor holder is poisoned.

The first strict compile exposed a const-pointer mismatch in a deliberate negative
reentry test. That failure is preserved. The distinct corrected fixture and exact
production sources pass `-Wall -Wextra -Werror -UNDEBUG` with ASan/UBSan. See
[actual host records](../evidence/enhanced-editor/mixed-readers-causal-host/README.md).

The injected port operates on ordinary host memory. Native CIA/IRQ source ownership,
hardware paired activation, whole-song producer binding, frontend preparation and
editor PLAY integration remain unfinished. Native/physical execution of this owner
is **NOT_RUN**. The separate 56-case HOST publication/cleanup fixture qualifies its stated publication, constructor and consumed-release cases. Retained defensive recovery after unconsumed newborn cleanup, nonempty imported registry and all20 slots simultaneously active remain open; the typed facade has its separately scoped evidence below.
No timing, DMA, card RAM capacity/order/completion, voice-stop, audio or listening
acceptance follows from these tests. The separate one-armed native RAM fixture's deadline failure and failure-time hold evidence remain preserved. Subsequent approved HOST recovery/normal release is recorded [separately](../evidence/enhanced-editor/native-mixed-ram-port/approved-recovery/README.md); the original native90 admission remains FAILED.

The separate [publication and cleanup qualification](MIXED_READERS_CAUSAL_PUBLICATION.md) adds 56 HOST failure/lifetime cases without changing this production core or its original 26-case fixture.

The opt-in [typed task preparation contract](EDITOR_MIXED_CAUSAL_PREPARE.md)
binds the genuine owner and original source before factory/cache construction,
retaining one pure TRIGGER pair with independent C/R/source closure. Its separate
five-group [current-tree HOST evidence](../evidence/enhanced-editor/editor-mixed-causal-preparation-host/README.md)
is distinct from [current CLI HOST content evidence](../evidence/enhanced-editor/editor-mixed-causal-preparation-host/current-cli-host/README.md):
five historical parser records plus two later observations and selected
focused43/default-controller C integration. Original CLI V2 invalidfalse and V3
TMPDIR failures remain FAILED. Independent SAVED content admission evaluates the
complete captured results and exact artifacts without replay or invented runtime
closure snapshots. This facade does not wire live editor PLAY or supply a paired
hardware source.
