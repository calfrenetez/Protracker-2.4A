# Current recording native qualification

The two current injected recording fixtures passed host sanitizers, cross-build and separate shared030 native runs. Product source is committed `0adacb2`; its zero-overlay export contains 7443 committed inputs. The 64 host dependencies,39/62 native dependencies and nine excluded dirty paths are recorded. Unrelated editor/display work was excluded.

| Fixture | Native run | Result | Owned Fast allocations |
|---|---|---|---:|
| `PTExecCaptureTest` | `1790988616063281000` | RC0; CAPTURE STAGING PASS | 143; zero owned bytes |
| `PTExecEditorCaptureTest` | `1790988631933347000` | RC0; CAPTURE SESSION, AMIGUS CAPTURE and EDITOR CAPTURE PASS | 70; zero owned bytes |

The two ASan/UBSan host groups passed in 9.358 seconds (9.975 seconds including provenance work). The native candidates are 114852 bytes/SHA `ab5ceb7e8f4773b3a41ad741845ca83c4e86bf326432a514b8a0f00c9ecb85f9` and 166164 bytes/SHA `2141a7734809f747047d0af7a06f29df97f69115539b052161e6a2fe5b5b31c7`. Ordered commands, pinned compiler, seven runtime fingerprints, included-fixture dependencies and before/after frozen-input checks are retained.

Each run used the unchanged 90-second fixture bound with a separate 140-second process guard. The root qualification used its 350-second overall guard. Each nested result records exact cleanup; each separate independent cleanup records 11 observations across 10 seconds with both owned paths absent, the original sole PID 19081/profile, and a final running guest with all four Paula DMA channels off. The top `qualification-result.json` is preserved separately from `runs/*/result.json` and independent cleanup records.

Root reported that the shared window was explicitly released after all checks and acknowledged by AmiConnect revision 29 with no hold or reservation. This coordination report is attributed in `root-reported-coordination.json`; the packaging subagent did not perform a live ownership check. Root also confirmed the archived root script has not changed since execution. Its archived bytes are independently verified; no embedded execution-time script hash is claimed.

`frozen-source-manifest.json.gz` retains all original 7443 path/blob/mode/size/SHA records, base commit/tree, zero overlays and nine dirty exclusions. `native/scoped-manifest.json` removes only the duplicated full source hash map, points to that snapshot, and preserves the original manifest fingerprint. Every raw copied file has its origin, byte count and SHA in the unique provenance manifest. Derived records are labelled separately. The scripts are historical evidence; this directory is not an instruction to rerun them.

The 30 September format/transfer prelaunch remains **NOT RUN** and unchanged. `historical-prelaunch-reference.json` records read-only fingerprints of that history. These later current-source runs are separate evidence.

This qualifies synthetic/injected format, staging, transfer, session/editor ownership and Exec Fast allocation cleanup. It does not qualify actual input or card MMIO, physical capture, timing, audible playback or human listening. No product source, tests, builds, targets, locks or staging were changed or rerun during packaging.
