# Activation packet scratch: tested source and native object evidence

The activation now stores the full packet before-image in the existing owner
staging packet while serialized entry is busy. A mutated packet is fully
restored before further walking; failure remains latched. Only the transient
packet is cleared. The owner layout, allocation budget, full expected-slot
checks and independent command/reader proofs are unchanged. The controller
warning fix adds braces and a newline without changing sticky-error behavior.

The tested source is based on `d4966fa`. The three source/test SHA256 values in
[observations.json](observations.json) match the actual private HOST inputs and
native compiler input map. Private paths are provenance references to the
workspace records, not additional repository dependencies.

## HOST checks

The whole-song aggregate and separate activation fixture passed once with
assertions, strict warnings, ASan and UBSan. The activation fixture executes
192 combinations of callback phase, eight mutation fields, raw outcome,
reentry and independent proof order, plus consecutive scratch reuse. It checks
full restoration, transient cleanup, retained uncertain owners, both proof
orders and unchanged sample masters. The two full stdout oracles and all four
compiler/runtime records are retained in the workspace HOST receipt. The
independent saved HOST audit and root acceptance bind those actual records.

## Native object checks

Three strict 68000/software-float object compiles passed with warnings treated
as errors. Saved object disassembly and dependency admission were completed
without recompiling. The original validation runner remains failed: its
controller dependency checker rejected 91 emitted names which resolve to
exactly 80 expected providers. All occurrences were verified. A separate
bounded objdump inspected the already saved controller object. The independent
combined audit verifies six recorded drivers, twelve streams and all three
object/dependency/stack groups.

All 83 target layout words match the earlier layout. Pointer and size_t are
4 bytes; the packet is 3,456 bytes, actual-slot snapshot 1,128 bytes and owner
72,252 bytes. The owner allocation size is unchanged.

| Function | Earlier local stack annotation | Current local stack annotation |
| --- | ---: | ---: |
| Activation fire | 4,692 bytes | 1,236 bytes |
| Packet publication | 3,704 bytes | 252 bytes |
| Command service | 1,504 bytes | 1,504 bytes |

These compiler annotations exclude callees, library helpers, indirect callbacks
and the interrupt trampoline. Controller frames still reach 2,812 bytes.
Neither the reduction nor the object layout qualifies total task/interrupt
stack use, residency, WCET, a launch candidate or exact live timing.

## Still to test

No linked native executable, Amiberry execution or real A1200 execution was
performed for this increment. CIA/source integration, full stack and deadline
checks, device RAM capacity, upload ordering/completion, hardware voice stop,
audio output and human listening remain separate requirements. Musical
scheduling has not been rebased or relaxed.
