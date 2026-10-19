# File-hardening return integration — 19 October 2026

The returned patch adds portable regression tests for file import, project/sample
save and classic MOD export. It demonstrated no new production defect and changes
no production code, public API, file format, export policy, native backend or UI.
Three existing test statements were split across lines for GCC's existing
`-Werror=misleading-indentation`; their non-whitespace tokens are unchanged.

The original ZIP and its Linux evidence were preserved. Its 13-path patch applied
to the exact `da75ddffb98287a19a4df20e57ce2eaed6df9803` handoff subset. On this Mac,
all 16 baseline modules plus the returned focused module passed: 21 positive test
methods. Separate existing RAW-import and sample-dispatch modules passed two
methods against the canonical working source; this is basic HOST adapter coverage.

Review then tightened three new test/document files before integration:

- Allocation-budget refusals compare every old allocation's bytes and live count.
- Every save return checks the complete source document and master/storage bytes.
- PTG and explicit round8/fixed-TPDF MOD exports from 16- and 24-bit masters receive
  the same syscall, allocation, budget, publication and cleanup fault matrix.
- Retained evidence directories must be fresh, and platform fields identify the
  actual host without implying a test result.

The final four-method focused campaign passed ASan/UBSan, assertions and `-Werror`:
148 corpus cases (42 accepted, 106 refused), 812 fault records, 499 syscall triggers,
50 allocation triggers, 16 save paths and 32 expected retained-temporary warnings.
All 116 child commands matched their expected return code. An additional freshness
check refused an existing evidence directory before subprocess creation and left
its sentinel unchanged. The canonical 13 files are byte-identical to that tested
private revision; all 43 production files and all 16 unrelated working paths were
preserved. Independent source and saved-result reviews found no material issue.

The original Linux 568-record campaign and amended Mac 812-record campaign are
separate revisions. The amended revision has not run on Linux. LeakSanitizer was
disabled on this Mac; explicit pointer/live-byte and descriptor assertions remain.
The baseline PP20 ctypes probe is separately unsanitized. These HOST results do
not qualify native ABI/runtime, Amiberry, A1200, AmiGUS, listening, process-crash or
power-loss behavior. No shared target operation was needed for this tests-only
integration. Native application development and hardware qualification continue
under their existing candidate-specific acceptance records.

Run the portable campaign from the repository root:

```sh
python3 -B -m unittest discover -s tests -p test_format_hardening.py -v
```

See [campaign details](README.md) and the [redacted HOST receipt](../test-evidence/file_hardening_host_20261019.json).
