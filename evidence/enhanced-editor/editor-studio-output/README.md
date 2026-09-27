# Integrated Studio output ownership

Host ASan/UBSan staged editor/wavetable/guard/Studio lifecycle suite passed in
27.584 seconds. The integrated output fixture compares every packed 24-bit output
byte to a direct-mixer oracle; it covers stalls, natural drain, queue allocation
and format refusal, initial reset failure, edit/undo/Stop/disposal, capacity and
partial-write failures, unexpected source-header changes and reset recovery.

Native compilation/main syntax passed; 154 input hashes matched the index.
Candidate PTExecEditorStudioTest: 248896 bytes, SHA256
18ba76692d0510f3688089eb92b48765d1b6af46d0f9a7096ae8554bcbe07144.
Shared030 run render-files-1790546681840279000 passed within the 90-second limit,
RC0, 121 Fast allocations, zero owned bytes, no Chip fallback. All four DMA channels
were off; exact run/launcher cleanup and independent absence verified.

The first attempt refused before staging because the shared emulator was stopped.
No guest artifact/process was created. After confirming no competing instance or
owner, the supported shared lifecycle wrapper started the saved 030 profile;
live CPU/memory/running state and exact Workbench-test.hdf/Dev mounts were checked.
The successful run is a new execution, not a rewrite of the pre-staging refusal.

This is native software ownership and simulated FIFO evidence, not device output,
AmiGUS/MMIO, audible or realtime qualification. Physical Amiga is off and untouched.
