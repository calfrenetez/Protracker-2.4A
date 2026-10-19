# Portable format hardening

Run only the explicit entry point from code/:

    python3 -B -m unittest discover -s tests -p test_format_hardening.py -v

Requires Python 3 and literal `cc` with C99/ASan/UBSan. No network, packages,
GNU link wrapping, LD_PRELOAD, editor, native SDK, emulator or hardware is used.

Optional environment settings:

- `FH_EVIDENCE_DIR`: persist bounded raw logs, exact command receipts, host facts,
  corpus manifest and CLI arity cases outside the source tree. The directory must
  not exist: creation is exclusive, so old command receipts cannot be mixed in.
- `FH_DETECT_LEAKS=1`: enable LeakSanitizer on hosts that support it. Default 0;
  pointer/live-byte allocation ledgers are always asserted independently.
- `FH_MAX_CASES`: generation cap, 1..512 (default 512). Insufficient caps fail.
- `FH_MAX_ALLOCATIONS`: checked per-path allocation bound, 1..64 (default 64).
  Insufficient caps fail instead of reporting skipped indices as exercised.

The I/O redirect header is force-included ONLY while compiling the five selected
portable adapter objects. All wrappers call actual POSIX operations and preserve
descriptor ownership. Injected close failure closes the real descriptor first,
modeling a consumed/ambiguous descriptor; tests assert the adapter does not retry
it. Core code, syscall shim, CLI, and test drivers are separate translation units;
the CLI is linked against unwrapped production adapters.

The original eight-bit save campaign covers allocated, generated, streamed, PTG,
direct MOD, round8 MOD, fixed-TPDF MOD, WAV, RAW and 8SVX publication. Two further
mono master cohorts, loaded from ptg-16.ptg and ptg-24.ptg, run the entire same
syscall/allocation/budget/commit-point matrix for PTG plus explicit round8 and
fixed-TPDF MOD only. High-precision direct MOD success is never assumed. Distinct
labels identify all sixteen save paths; final evidence row counts come from the
actual run, not an earlier campaign's totals.

Every successful syscall index has a fresh failure case and N+1 sentinel, plus
partial progress, EOF, EINTR, ENOSPC, fsync, close, verification corruption,
truncation/extra bytes, destination races, temp conflicts and pre/postcommit
cleanup denial. Original generated/streamed producer-interruption cases remain.
The full source document, live-byte count, all tracked master/storage bytes and
original PCM masters are checked before/after every save return and every report.
Snapshots are dropped only after checking them and immediately before legitimate
source replacement/release. No deletion of committed output is permitted when
temp cleanup fails; exact owned residue is inspected and cleaned after assertions.
Each of sixteen paths must emit its two expected retained-temp diagnostics.

MOD/PTG/PP20 file imports determine actual allocation count N, then fail 1..N
and exercise N+1. The previous document is a populated 24-bit stereo project with
slices and loop metadata; each ordinary failure compares the full document and
all live allocation bytes. Below-required budget refusals also compare the full
old document, every tracked old extent and its live bytes; snapshots are dropped
before legitimate successful replacement. Budgets below/at/above exact requirements
are checked.
A separate whole-file loader campaign preserves output pointer/size on refusal.

Sample corpus tests call real WAV/8SVX memory and reader APIs and RAW's explicit
format API, using independent signed PCM values. Reader callbacks can fail every
successful call; 8SVX reader decode may alter unpublished staging, as documented.
8SVX includes the existing Fibonacci-delta decoder; its encoder remains mono8,
uncompressed. Unknown optional chunks need not survive sample re-encoding.

Sample file import into the editor sampler is not linked: this handoff omits
src/editor/sampler.h and editor.h, required by raw_import/sample_import. No fake
sampler stubs replace it. Core sample decoding and real sample-file saves are
exercised. HOST_ENVIRONMENT.json records the actual current host system; macOS or
Linux is marked CURRENT_HOST only when it is that system, with the other NOT_RUN.
Other host systems retain their real platform name without claiming either OS.
This host identity is not a test PASS: actual command receipts supply outcomes.
Native/editor integration, listening, process-crash and power-loss acceptance
remain NOT_RUN. These amendments change tests only; results must be collected
from the amended source. Three existing test lines in the returned patch were
split solely to satisfy GCC's unchanged `-Werror` flags.
