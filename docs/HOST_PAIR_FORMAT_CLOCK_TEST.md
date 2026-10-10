# HOST mixed-pair format/clock fixture

The additive fixture tests mixed Paula/AmiGUS preparation using the HOST resource model and production master, cache and ownership code. Its 18 cases combine 8/16/24-bit masters, card8 BE/card16 BE/card16 LE cache formats, and PAL/NTSC clock models (709379/715909). Paula caches remain 8-bit. The two 24-bit document-backed masters receive nonzero low bits before promotion and pinning.

A single isolated run passed with assertions, AddressSanitizer and UndefinedBehaviorSanitizer enabled. The accepted checks cover exact cache bytes, 108 immutable completion matches, 36 cache-format refusals with unchanged output, and 12 edits that exercise low 24-bit input bits. They also cover full master capacities and exact project/WAV exports, retained reader/successor/source ownership after first-command disposal, and EARLY rearm without rebasing the original 960/1920 schedule. LeakSanitizer was not selected.

The run used committed baseline 9dd85bc62e8f266ee375a4ed63b8f5572ab84578 with the two tested fixture overlays and generated font. Its 81 ordered compiler units passed strict C99 warnings. Compiler and runtime returned RC0; compiler stdout/stderr and runtime stderr were empty. Runtime stdout matched the 365-byte oracle preserved alongside the public result summary.

The new matrix fixture textually includes the existing timer fixture with its entry renamed and uncalled. The timer fixture gains only the optional PT_PRIVATE_PAIR_TIMER_FIXTURE_ENTRY hook, which defaults to main. The complete default function body and output are unchanged. The old timer10, baseline34 and matcher219 suites were not replayed by this run.

Public evidence is limited to result.json and run.stdout in evidence/enhanced-editor/host-format-clock/. The complete receipt, independent saved audit, exact recipe, source manifest and raw streams are retained locally because they contain private paths and environment/process metadata. The public summary does not replace that detailed audit.

The original working checkout was left untouched after its Git inventory timeout. Publication uses the verified remote baseline and a non-force expected-head update; local synchronization remains pending. Current dirty editor/display integration was not tested.

This is HOST model acceptance only. Native ABI, CIA/IRQ delivery, physical DMA/card RAM/upload ordering/voice stop, WCET, exact musical activation, Amiberry, real-A1200 audio and human listening remain unqualified by this fixture. The exact live scheduling requirement remains unchanged.
