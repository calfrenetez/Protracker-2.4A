# Bounded MOD donor source loading

Host ASan/UBSan source tests pass a streamed real-file donor load, all six
staging allocation failures, budget rejection and failure after a completed
staged load. Old donor, destination song, sampler budget and history survive.
Exact imported PCM, volume, finetune and loop metadata, independent source/master
storage, undo/redo and source release checks pass. Editor regression also passes.

No new emulator or physical run is claimed for this donor integration. The
underlying bounded MOD document reader has separate shared030 qualification in
../mod-import-stream. PP20 input remains on the previous path. Native UI and
physical acceptance remain separate; unrelated display work is preserved.

Full pinned native cross-build passes.
