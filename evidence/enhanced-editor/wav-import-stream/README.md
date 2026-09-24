# Bounded WAV import and native RAW/WAV qualification

Host ASan/UBSan tests pass: RAW16/WAV6 formats, exact multiblock samples,
three allocation failures, limit/producer/malformed-format refusal, interrupted
and short reads, undo/redo and zero leaks. Core positional WAV reader tests
cover metadata reads<=16 bytes, data-before-fmt, unknown odd chunks, truncation,
duplicate fmt, accepted external tail and failed-read output preservation.
PCM/WAV and existing sampler regressions pass. Full pinned native build passes.

Fresh coordinated shared030 RAW/WAV fixtures pass rc0:160/60 Fast allocations,
zero owned bytes and budget refusal without Chip fallback. Complete test-file
cleanup and explicit release confirmed. No physical/card/audio/lifecycle or
configuration changes. These are file/allocator fixtures, not visual UI or
physical acceptance. IFF and module/PP20 imports still use encoded-file buffers.
