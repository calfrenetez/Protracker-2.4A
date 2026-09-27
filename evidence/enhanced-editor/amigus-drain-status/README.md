# Require confirmed PCM drain status

The session previously treated any positive drain result as completion. It now
requires exactly1;0 stays pending and every other value records a fault and enters
guarded reset. Unexpected status can no longer report successful playback.
The regression first failed with the previous code, then all13 focused AmiGUS
host tests passed. Native fixture covers seven end/reset/failure cases.

Candidate34148bytes SHA256
80195b868172736fa3d98e5d016f6515f77e9219e505af0f7c6890af3b1fada8;
all22 normalized dependency hashes match the index. Shared030 run
render-files-1790551019707595000 passedRC0,7Fast allocations returned to zero,
noChip fallback, all4DMAoff in the independent check. Exact run/launcher absence
confirmed in a separate elevated invocation and window explicitly released.
Injected FIFO only, no real device/IRQ/MMIO/audio or physical acceptance.

Build sources and flags are retained in build.json. Compilation succeeded before
an initial manifest verification rejected a non-normalized dependency path; the
path comparison was corrected and verified without changing the binary.
