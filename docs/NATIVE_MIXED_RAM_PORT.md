# Private typed mixed RAM diagnostic port

`native_mixed_ram_port` supplies a separate ordinary-RAM diagnostic for the copied
[paired activation owner](MIXED_READERS_ACTIVATION.md). It copies Paula geometry
and AmiGUS numeric cache facts into fixed software state: two retained command
records, 32 persistent reader histories, 20 typed slots, 16 actions per packet
and a 64-entry clock trace. Numeric card bounds describe register address width;
they establish neither CPU memory spans nor physical card capacity. The existing
four-Paula port, CIA layout/trampoline, public core/editor APIs and classic layout
remain unchanged. This port is not connected to native PLAY.

## Registration, activation and independent quiet

The private caller initializes fresh, fully zeroed noncopyable storage and binds
the original genuine owner and borrowed queue exactly once, immediately after
successful open/borrow and before validation, factories, enqueue, publication or
source exposure. Empty and cancelled-before-publication lifetimes also bind.
The first packet never supplies registration. Complete port/adapter/clock context
extents remain disjoint and alive through positive source quiet and owner disposal;
the caller excludes entire entries, dispatch and borrowed input edits. A dirty or
initialized port cannot be reset or reused.

Publication copies the complete immutable packet and all 20 expected original
keys. Activation compares every key, validates unique Paula/card slots and all
trigger placements before ordinary-RAM adoption. Manual bounded dispatch uses the
honest injected clock and original window, then calls genuine fire once. Raw
adoption remains recorded if a later diagnostic check fails; lateness cannot undo
references, rebase deadlines or become a clean refusal. New reentry makes claimed
absence uncertain and retains ownership, including raw-zero arm results.

Command quiet removes only the complete packet and callback owner. Reader quiet
requires exact ticket proof for every retained command that names the original
reader in any of its 20 expected keys or 16 action keys. It disables each dependent
whole batch and erases all 16 geometry/card tails only after that batch's positive
proof, then removes only the exact reader/current key. A pending or unknown second
proof retains the reader; successor keys and unrelated commands survive. Genuine
core checks complete original command/reader provenance upstream. Unknown or
malformed replies remain distinct from raw adapter diagnostics and require later
explicit independent proof; there is no automatic retry or forced release.

After actual local queue-pointer consumption and positive C/R quiet, the core
makes at most one source shutdown attempt on the original opaque registration.
Pending/unknown shutdown retains the owner and contexts. Later explicit close
uses a separate read-only source-quiet observation, never a second shutdown.
The consumed queue pointer remains opaque; neither callback dereferences the
owner/queue identity.

A narrow never-bound software-empty cleanup branch establishes universal absence
without learning incoming pointers or calling shutdown, and becomes irreversibly
terminal. It cannot qualify a hardware source; the exceptional retained
failed-constructor recovery path remains independently untested.

Fire updates the activation/RAM ledgers. Queue adoption and original timing become
observed through genuine command or reader receipts. A noncancelling ACTIVE reader
observation returns PENDING and retains all pins/references. The subsequent
original-key query still requires a current eligible reader; observation is not
retirement or a substituted readiness flag.

## Current scope and qualification

Only one ticket may be armed. A finished retained command may coexist with one
pending callback; second armed publication refuses without changing the first
packet or callback. Two dependent retained records are not two simultaneous
future callbacks. The user's **Keep exact scheduling** decision remains in force.
A native next-ticket source adapter, live musical timing and native PLAY integration
remain separate work; this diagnostic introduces no jitter allowance or fallback.

The selected V4 HOST fixture passed all 65 genuine heap cases under strict warnings,
assertions and ASan/UBSan, with the complete oracle, zero runtime stderr and an
accepted independent saved-result audit. It covers mixed 4/12 and 16-card routes,
8/16/24-bit master pins, selective Paula8 and card8/16 caches, replacement pressure,
CONTROL/STOP,
independent C/R orders, expected-only dependencies, raw faults, original windows,
retained late adoption and exact master/project beforeimages. The original V3
strict compile succeeded, then its premature reader-key query assertion failed;
that first failure is preserved. V4 adds genuine noncancelling observations only
before the affected queries and preserves the retirement order and production C/H.
Run the selected portable fixture with `python3 -B tests/test_native_mixed_ram_port.py`.

| Evidence tier | Current status |
| --- | --- |
| V4 HOST software ownership | PASS, 65 genuine cases |
| Corrected HOST software ownership | PASS, same 65 cases after formatting-only core correction |
| Distinct native Fast/Chip crossbuild | PASS, 28 strict m68000/soft-float units; compiler evidence only |
| First exact Amiberry attempt | FAILED: native 90-second completion deadline; late full log preserved after approved recovery |
| Exact real-A1200 execution | NOT_RUN |
| Live timing, IRQ/WCET/whole stack, device/audio/listening | Not qualified |

The first native build stopped at two inherited `retirement_controls`
misleading-indentation warnings under the unchanged strict GCC flags. No target
ran. A separate whitespace-only core correction preserves the C tokens. The corrected
HOST fixture passed the same 65 cases; its distinct native build passed all 28
units and linked a 237320-byte HUNK with SHA-256
`ec4dbf52210eb0a7ec3ce026400ee885aeb8b0315cbec79328b7184ce5ddc30a`.
All 51 recorded build commands returned zero with empty stderr. The build saved
84 per-unit object/dependency/stack products and 120 canonical active dependencies.
These are compiler results, not runtime or aggregate stack acceptance.
The original partial build failure remains preserved.

The compiled Fast/Chip Exec wrapper V2 retains unchanged wrapper C. Its explicit
entry includes V4 once, preserves assertions and task/stack/priority checks, uses
the bounded existing Fast pool for masters/control/fake card, and intercepts only
the genuine fixture's selective Paula allocation binding for reserve-checked
actual Chip AllocMem/FreeMem. The preselected diagnostic launcher requests 131072
bytes; the unchanged admitted stack span is 65536 after the SDK upper-bound bias.
Source checks and two SP observations establish no total stack or timing result.
Actual Fast/Chip placement/release requires an exact target pass. Card capacity,
transfer ordering/completion, voice stop, DMA, audio and human listening remain
separate acceptance gates. Saved host and compiler evidence is in
[evidence/enhanced-editor/native-mixed-ram-port](../evidence/enhanced-editor/native-mixed-ram-port/README.md).
Update target acceptance only from subsequent actual saved target results.

## First exact emulator attempt: completion deadline failure

On 7 October 2026 the exact HUNK above ran once after a guarded shared 030
startup. It entered the native wrapper and reported stack 131072/guard 131070,
priority 0 and an Exec Fast allocation pool. The 90-second native deadline expired
without a return code, terminal marker or 65-case oracle. The host runner exited
1 after 93.283 seconds; its parent was reaped and its process group was absent.
The cause remains unknown. The saved stdout has no per-case progress and guest
stderr was not independently captured.

The failure was retained with its run files and persistent ownership hold. No
cleanup, retry, disconnect, emulator stop, restoration or release was attempted.
Those obligations require separately approved recovery and actual verification.
Physical execution remains NOT_RUN. Host and crossbuild results above remain
separate from this failed runtime gate. See the saved
[first-attempt evidence](../evidence/enhanced-editor/native-mixed-ram-port/emulator-first-failure/observations.json)
and its available raw output.


## Approved recovery and explicit release

After fresh human approval, the reviewed exact-owner HOST recovery and separate
read-only verification passed once. The original failed run/launcher and all
late artifacts were preserved in private custody; both known startup metadata
files restored exact beforeimages. Independent saved audits passed 30 recovery
and 21 release checks. Normal four-record release completed at EMPTY fence 1144,
with no remaining reservation/HOLD/potential resident; AmiConnect acknowledged
the exact window's reported verified release.

The preserved late log has the complete 65 RAM-only oracles, zero owned
Fast/Chip allocation footers, RC0 and done. The original 90-second admission
remains FAILED. There was no retry, deadline enlargement or physical promotion.
Loaded DevBench connection eligibility remains a separate maintenance gate.
See the [finished recovery and release records](../evidence/enhanced-editor/native-mixed-ram-port/approved-recovery/README.md).


## Project checksum update: HOST qualification

The project checksum now uses a shared immutable64-byte nibble table across its
three private contiguous/stream/positional paths. The format, exact golden bytes,
all validation and master/save assertions are unchanged. Five focused/baseline
HOST sanitizer groups and the unchanged genuine65-case mixed RAM fixture passed.
The original missing-header-path compile failure is preserved; a distinct narrow
command correction completed only the remaining65 group. See the
[complete HOST evidence](../evidence/enhanced-editor/project-crc-nibble-host/README.md).

No updated native crossbuild or exact target execution has occurred. The old
deadline failure staysFAILED; this change establishes no cause or measured native
speedup. Native90/stack131072 and all diagnostic success markers remain required.
