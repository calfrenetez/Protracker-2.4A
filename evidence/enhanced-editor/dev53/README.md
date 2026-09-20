# Dev53 Recent Projects validation

Native UI workflow PASS; physical acceptance remains open.

66 host tests passed in117.519s. An earlier full run caught a disk-panel Back
hit-target regression; it was fixed preserving the existing target and the
failed development log is retained separately. The accepted main-screen golden
remains unchanged. Final short status messages passed the targeted editor/golden check in6.339s.

Native attempt recentui1789943125303677000 passed core and alternating preference
recovery but failed the startup isolation assertion: getenv did not read the
AmigaDOS global test variable. Native code now uses GetVar/GVF_GLOBAL_ONLY.
Native attempt recentui1789943318824247000 passed preference recovery, damaged/
missing opens preserving dirty state, successful save/reopen and failed-save
exclusion. It needed an injected note because the original test note matched the
fixture, then hit a harness high-resolution mouse-coordinate error. This is
partial diagnostic evidence only, not a final unattended workflow pass. The
runner now uses a different note and correct native click coordinates.
Both attempts were explicitly released after guarded QUIT and fresh checks.

No physical A1200 or AmiGUS tests are claimed.

Final unattended UI capture `recentui1789943869571091000` PASS in 72.385s.
Tests used isolated RAM preferences via DOS GetVar, actual document open/save,
four native input.device mouse deliveries, and three editor launches. Missing
and damaged files preserve dirty edits/list; failed save is excluded; saved
project is byte-identical to baseline; Remove/Clear are persisted and an empty
list survives relaunch. Final screenshot reviewed with no new status wrapping.
Core/file stress tests were not repeated in this UI-only run: preceding native
captures include their passing results. The immediately preceding capture
recentui1789943613807449000 passed those and keyboard/storage, but IPC mouse
scaling failed to target controls; it is retained as partial diagnostic evidence.
The final harness uses the established PTEditorClick helper instead.

Final guarded QUIT and fresh no-process/private-HDF-holder/socket/launcher checks
passed at22:39:27UTC; AmiConnect explicitly notified of release.
