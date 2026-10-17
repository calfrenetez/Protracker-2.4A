# Source and song policy across typed New/Load

The typed document replacement keeps the editor's original sample and song
allocation policies across genuine disposal and fresh initialization. Loading a
sample source temporarily debits its allocated bytes from the sampler budget.
Replacement captures the checked sum of that reduced budget and the source's
actual allocation, so a successfully retired source returns its capacity to the
new editor. The independent song allocator, context and finite budget are also
restored after initialization.

The original allocator contexts must remain alive and disjoint for the whole
operation and any subsequent use of the restored policies. This preserves policy;
it does not transfer an old source, song version, history record or playback
owner to the new editor. Master-preserving Load still uses the genuine document
factory and serialized project data.

Admission refuses an overflowing budget sum before writing the replacement
control or stopping the old owner. Preparation failures and PREPARED cancellation
leave the original source, debit, song/history and document intact. Existing
checked-owner and storage-disjointness requirements remain in force.

Before disposal, including after real stop/voice-close callbacks, replacement
rechecks both policies and the actual source factory state. After disposal and
before old-document release or publication, it requires the source's project and
storage to be empty, no remaining source allocation or loaded/dirty state, and no
factory retirement uncertainty. Song ownership and accounting must also be empty.
The check repeats while deferred sampler retirement is pending and after the
once-only old-document release callback. Uncertainty retains HOLD without a
release retry, forced clear, publication or rollback claim. No retired main
project/PCM traversal is needed for these checks.

Fresh editor initialization restores the sampler policy and initializes a new
song owner with its original policy before optional HOST MIDI-owner rebind. This
does not change the selected exact musical schedule, enable the HOST-only MIDI
composition on native targets, or complete repeating native frontend actions.

Qualification is separate for each layer. The existing genuine MIDI-owner
regression passes with this private policy overlay, including its original eight
focused cases and NULL/default-OFF action cases. The private default-native typed
replacement/action translation units also compile into two Amiga HUNK_UNIT
objects with strict diagnostics. These objects have not been linked or run.

The separate clean-quartet HOST run passed all six new cases and thirteen
unchanged legacy cases. A fresh canonical run with the complete preserved
working editor/view quartet also passed the same cases, plus 26 pure admission
cases. Each run uses 84 translation units and 200 literal inputs under ASan/UBSan,
with five completed contexts and one whole retained uncertain domain. New/Load,
failure/cancellation preservation, real song append/undo/redo with the restored
allocator, admission overflow and source-release reentry are checked.

The canonical MIDI regression separately passed its eight focused cases, original
NULL/default-OFF action cases and 26 pure quartet-admission cases with this policy.
These results preserve the historical private and earlier MIDI results separately.
See the compact receipts in `evidence/document-policy/`. No emulator, physical
allocation/device/voice-stop, output timing or listening acceptance is added.
