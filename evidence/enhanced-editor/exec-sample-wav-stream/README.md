# Native streamed master WAV validation

Shared030 run render-files-1790229895561501000: rc0; exact8/16/24 mono/stereo
WAV outputs match the existing encoder, masters unchanged, fixed export
workspace11240 bytes. Eighteen tracked allocations use production Fast RAM
(no Chip), zero owned bytes at end, budget refusal. Generation/verification
failure and mismatch, allocation refusal and destination protection pass.
The fixture removes its output and all staging; host verifies no extra files.

Fresh AmiConnect clearance, shared lock and live target guards preceded the
run; exact owned cleanup and explicit release completed. No audio/card/physical
operations or guest lifecycle changes. Host sanitizer fixture also passes.
This tests the native platform save/allocator path, not visual UI acceptance or
physical disk durability. The native editor is already wired to the same path.
