# Bounded RAW master export

2026-09-24: host sanitized WAV/RAW tests and pinned native build pass. Shared030
run render-files-1790230526554513000 returns0:16 formats match the existing RAW
encoder byte-for-byte (8/16/24 mono/stereo, explicit endian, signed/unsigned8).
Precision/channel/rate mismatches are refused; no conversion. Masters unchanged;
existing destinations preserved; allocation failure leaves no file.

Native export workspace11240 bytes,35 tracked Fast/not-Chip allocations, zero
owned bytes and budget refusal. All output/staging removed by fixture before
harness cleanup. Fresh AmiConnect clearance/shared guards/lock, exact owned
cleanup and explicit release completed. No physical/audio/card operations.
Native editor RAW handler is wired; accepted layout unchanged. This is core
and native file/allocator proof, not visual UI or physical disk qualification.
