# Editor MIDI recording ordinary-Task RAM test

This separate experimental entry tests the genuine guarded recording owner in
ordinary Amiga Task context. It is excluded from the default editor and build.
No input source, CAMD, GUI, clock, output or audio backend is activated.

The [relative recipe](../tools/probe_recipes/editor_midi_recording_task.json)
lists 35 explicit units and the qualification-only selector. It describes the
actual strict 68000, soft-float, nix20, C99, Os profile; it is not a builder,
SDK/toolchain admission or target launcher. The earlier generic-O1 source-only
recipe remains preserved and was never executed. The actual private build uses
the complete saved clean editor pair, preserving the unrelated working pair.

The test owns a real document/editor and two separate recording owners, with
three encoded snapshots. It retains an original pinned 24-bit master and checks
its complete descriptor, pointer, four values, version and accounting. The first
take tests armed and stopped-but-attached edit/disposal vetoes, channel filtering,
Note On/zero-velocity Note Off, normal Stop, matched detach, preserved instrument
and effect fields, and exact save/undo/redo bytes. The second tests capacity failure
at the row 63 boundary: Stop retains armed obligations; explicit local cancel keeps
the accepted note; detach remains separate. These supplied Q16 positions are
synthetic data, not a live sequencer or timestamp-conversion measurement.

Checked allocation has an explicit 1 MiB sampler allowance and derived twelve-block
peak, within the unchanged 32-slot registry. Success requires real master unpin,
positive editor storage retirement, checked document release, exact detached-owner
and snapshot release, balanced allocation counts, zero live/used ledgers and
memory-control completion. Detached queries after ordinary native frees do not
establish protected-page or former allocator-context lifetime acceptance.

Every unexpected observation retains the original whole Task, stack, HUNK,
allocator and surviving resources without unwind, forced free, retry or Break.
That path is **not cancellable**. Only a separately guarded, disposable owned-VM
experiment is being prepared; no physical execution is admitted by this source.
Original Task/Exec identity and nesting are checked. A real SP/Task-bound check
requires at least 1 MiB remaining before heavy work; the planned total Shell stack
is 2 MiB. This is not a whole-call-chain, CRT, OS, IRQ, WCET or measured stack proof.
TypeOfMem endpoint checks do not certify every byte's placement.

## Observed software qualification

The [native object result](../evidence/editor-midi-recording/native-task-objects.json)
qualifies 35 genuine objects and 175 successful compiler stages. All actual
compiler sources, SDK inputs, tools and private controls stayed unchanged.
The outer result remains **FAILED**: two public evidence files changed only their
change-time fields; all 221 input bodies and 219 full records matched. The cause
is unknown. This failed boundary is preserved; the build was not repeated or
relabelled. The separate [linked-artifact result](../evidence/editor-midi-recording/native-task-link.json)
passes five commands, all 26 current runtime providers, 42 ordered link inputs,
complete HUNK structure and the emitted startup/SP/lifecycle checks. All 75 link
input full records, protected files, guard, admission and final products match.
It produces a 228,840-byte executable, SHA-256
`070fd6b991482a3521ce7abb7117a4f46430766e2fb56eec64ab00263397f574`,
CRC32 `02279F4C`. This distinct HOST link does not erase the object-boundary limit.
Default HUNK memory attributes are unchanged; actual Fast placement is unobserved.

Native execution, actual Fast placement, observed Task lifecycle, interactive
input, playback recording, live timing, CAMD, device output, physical testing and
listening acceptance remain **NOT RUN** for this exact test.
