# Physical project master-save and Fast RAM qualification

The exact `PTExecProjectStreamTest` passed once on the real A1200 on 3 October
2026, using the coordinated shared harness. Physical run 1791039556965048000
returned RC 0 in 92.3766 seconds within its310-second external bound. The native
90-second deadline was unchanged. Candidate 65,656 bytes has SHA256
`d4a3da30620fef5e644ca8dae5a8a48d3483cc1522a50acbbd7b57835c380451`;
all21 compiled inputs matched committed and working source before execution.

The fixture preserved exact project layout/CRC, mixed 8/16/24-bit masters, loops,
slices and extensions, and passed sink/file/budget/replacement refusal checks.
Workspace was 7,164 bytes. All six owned allocations were verified Fast memory,
ended at zero owned bytes, and the pool refused a zero budget without Chip fallback.
The physical pool reported 128,218,328 bytes, reserve 262,144 and MEMF_FAST flags 4.
This is software-memory/file evidence; it does not measure installed hardware
sample-RAM capacity or audio timing.

The installed bridge identity and actual 68030/Plipbox identity passed before a
fresh private RAM directory was created. Uploaded candidate CRC/size and pulled
SHA matched. All 26 transport calls were acknowledged. Typed file/directory
inventories accepted only the two owned files, no remaining project or temporary
children. Exactly those two files and the empty directory were deleted
nonrecursively. Eleven separate physical absence calls spanned 16.1443 seconds.

DevBench then returned to the same original PID 19081/full 030 AGA command/profile,
connected localhost bridge, running 68030 and all four **emulated** DMA channels off.
A separate locked return checker completed RC 0 in 14.0199 seconds: eleven idle
observations over 13.2091 seconds. Safari was positively observed authenticated
Connected View Only, control and remote input released; this scope acquired no
browser control. AmiConnect TAKE 100 and explicit RELEASE 101 are recorded.
No reservation, outstanding operation or recovery hold remains.

Physical result SHA256:
`7d6c91ab22f35d8508e527791ff7ce1542ecccd8dda7dfcaa0eeaf8a0926c4f3`.
Independent return result SHA256:
`cd7b62c714da5719a6e262f07b044603cdb9fe6a859a4099d8d28b4bfb2aa5bb`.

The original FIFO 4096 and WAV qualification failures remain FAILED. This run did
not use AmiGUS registers/sampleRAM, drivers, IRQs, physical Paula register reads,
audio, input, installed settings or lifecycle operations. It does not qualify
frontend PLAY, music timing, hardware output or listening. Host/mock preparation,
exact emulator prerequisite, physical fixture and independent return evidence
retain their separate scopes. Initial local availability-check errors are
preserved as host errors; neither touched a target or created a recovery hold.

`manifest.json` records 42 original copies and their hashes. The packet includes
reviewed preparation/helpers, host pins/mocks, exact staged binary/readback,
physical result/native log, separate return receipt, coordination gate/release
and Safari observation. External SDK/compiler files are pinned by hash and are
not duplicated here. Historical source and exact emulator prerequisite remain
in `../project-output-safety/`; no product fixture was rerun for this archive.
