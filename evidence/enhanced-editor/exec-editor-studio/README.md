# Native editor / Studio ownership lifecycle

Fresh AmiConnect window, shared test.lock and live shared030/bridge identity guards.
PTExecEditorStudioTest passed rc0, editor lifecycle and EXEC MEMORY markers.
21 tracked allocations passed TypeOfMem Fast/not-Chip; zero final owned bytes.
Production allocator budget refusal retained. calloc of the test editor object is
also routed through the tested pool; callback allocations cover project/master/
song ownership. This is not a claim about untracked CRT allocation or stack data.

Real pinned playback stops on note edit, undo and disposal. Navigation preserves
playback; guard ownership and cleanup checks pass. Run
render-files-1790217359166695000 completed, exact run/launcher cleanup confirmed,
lock/window explicitly released. Exact binary SHA256 in result.json.

No UI automation, physical operations, audio device output or AmiGUS proof.
Full native build includes unrelated prepared-display work and is not an editor
release/layout qualification. A fresh full host regression accompanies this run.
