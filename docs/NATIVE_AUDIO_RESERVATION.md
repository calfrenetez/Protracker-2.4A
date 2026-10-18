# Private native audio reservation

The module in `src/native/native_audio_reservation.c` is a separate, one-shot audio.device reservation owner. It is not enrolled in the editor or any default build. Its exact header requires `PT_NATIVE_AUDIO_RESERVATION_PRIVATE_SOURCE=1`; the production translation unit also requires genuine Amiga/m68k compilation and32-bit geometry. The original SOURCE ONLY comments are preserved because runtime qualification is still open.

The API enrolls only resources it creates, asks for all four audio channels at minimum initial precedence, sets its own precedence, and issues one LOCK request. Retirement issues FREE for its exact reservation, consumes a positively completed LOCK reply under the documented terminal checks, closes the owned device and deletes its own requests and port. Any uncertain post-enrollment result enters sticky, nonreturning HELD in the original Process/HUNK/stack. A copied observation is diagnostic data and is never accepted as a caller certificate. There is no reset, retry, adopted populated owner or caller-provided completion flag.

This adds no audio WRITE, Paula register/CIA operation, cache retirement, legacy native stop implementation, document publication authority or frontend/default selection. Shipping native document publication remains STOP_UNQUALIFIED.

## Evidence

[evidence/native-audio-reservation/host27.json](../evidence/native-audio-reservation/host27.json) records the completed private HOST27 qualification of these exact source bytes. It exercises nine actual production functions with explicitly fake Exec/audio layouts, a fake SP accessor and a HOST Wait sentinel. All27 fresh worker cases passed with strict warnings/assertions, ASan and UBSan. LeakSanitizer is unavailable on the selected Darwin route and was disabled; exact allocation/live ownership oracles remain part of every case. Cases25–27 inject high pointer bits into OPEN, FREE and completed LOCK masks and verify that full-width validation retains the model owner before diagnostic truncation.

[evidence/native-audio-reservation/native-objects.json](../evidence/native-audio-reservation/native-objects.json) records ten quiet genuine native stages across the module and a separate private probe. Actual M=I was2 own bodies+30 SDK headers and2+41, within the fixed118-header allowance; genuine32-bit assertions, the explicit private selector and real SP reads passed. The separate private probe is not adopted by this patch. Module/probe object sizes are3512/924 bytes; individual SU maxima36/92 are not aggregate stack/headroom evidence.

The two records above qualify the saved HOST-model and native-object tiers. The separate [static link audit](../evidence/native-audio-reservation/native-static-ABI.json) qualifies the private 15,264-byte probe link, its 18 current imports, 393 bounded relocations, genuine Exec calling conventions and 64 KiB CRT stack request. That probe is a separate component, not an editor build or a measured stack minimum.

One exact-candidate Amiberry attempt subsequently failed without a completed native oracle. The saved polls showed an empty native log and completion file; they did not collect the native log body or establish the first blocking or HELD phase. The failure therefore does not identify an audio.device defect or justify changing the ownership checks. The original failure is preserved, and no unchanged retry or physical promotion occurred. Separately verified run-owned cleanup restored the launcher files, confirmed the emulator/service and staging were absent, and released shared ownership. Guest environment restoration was not confirmed; recovery disposed of the private VM. The [compact failed-attempt record](../evidence/native-audio-reservation/emulator-attempt.json) records these boundaries without private paths or process identities.

Native Process/Task/device/IRQ/retirement acceptance remains unqualified; physical audio and listening remain NOT_RUN. A separate private phase-trace diagnostic has passed 34 strict HOST cases with ASan and UBSan, including the original27 ownership cases and seven new progress/context cases. Its bounded progress collector separately passed17 HOST models. The [diagnostic HOST record](../evidence/native-audio-reservation/host-diagnostic.json) distinguishes those model results from the separate native diagnostic. Partial progress never proves completion or release. The canonical module and production publication boundary are unchanged. Any later component run needs its own admitted, disposable owned VM; this document grants no target authority.

## Focused HOST regression

`tests/test_native_audio_reservation.py` is the new focused repository wrapper. It reads seven named ordinary resident source/data inputs and constructs four compilation inputs in a fresh temporary HOST workspace and uses the existing qualified `/usr/bin/cc` driver entry with the original strict ASan/UBSan flags. The qualified host route is64-bit; other compiler/runtime routes are not implicitly accepted. Normal qualification uses the existing owned HOST supervisor so compiler descendants remain contained; the wrapper adds no process group, shared harness, transport or target controller.

The wrapper reconstructs `selected_native_audio_algorithm.h` directly from the canonical production C. Only the pinned native include/geometry preamble and the pinned native SP accessor are excluded. The explicit HOST declaration replaces that accessor; a full-byte inverse reconstructs the complete production source, and the selected-body hash fixes all nine retained functions. No algorithm is hand-transcribed and no fake `__amigaos__` or `__m68k__` macro is defined. The fake boundary itself refuses native compilation.

The existing C fixture and all27 expected stdout/oracle records are unchanged; their historical HOST24 labels are intentionally retained. The new wrapper passed its own separate Root-operated regression: one unittest, a quiet strict compiler and all 27 exact fresh-worker oracles, with ASan and UBSan. The [saved wrapper qualification](../evidence/native-audio-reservation/host-wrapper.json) records source preservation and positive closure of that operation. It qualifies reproducible HOST packaging; it adds no native runtime or device acceptance.

A later supervised check can select only this file:

```text
python3 tests/test_native_audio_reservation.py --evidence NEW_PRIVATE_DIRECTORY -v
```

The evidence directory must be fresh and private. Raw test logs may contain local paths and belong outside committed evidence. The two compact JSON files above contain no private paths, process identities or filesystem metadata. No existing test/build recipe, native frontend or unrelated source is edited. Ordinary unittest discovery includes the new focused HOST test; native build selection remains unchanged.
