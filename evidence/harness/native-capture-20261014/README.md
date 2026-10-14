# Native sampler capture qualification

The exact 166,396-byte sampler fixture passed once on the shared 68030 Amiberry
configuration with separate native stdout and Shell controls. The guest verified
the executable CRC and size, observed Stack 262144 in the invoking Shell, returned
RC0 and produced all eight expected native PASS lines. The native capture was
1159 bytes with CRC32 808D90D6, checked before and after bounded readback.

The test covers loop/slice metadata, format conversion, IFF and RAW import/export,
stereo24 gain history, sample budgets, retirement inventory and sampler
save/reopen behavior. It uses clean source 78d6871, Core33545 and eighteen units
with typed replacement and hardware operations disabled. It does not qualify the
newer typed New/Load composition or its frontend integration.

Exact run-owned guest/shared files were removed. The owned emulator stopped,
original launcher files/configuration were independently verified, and ownership
was explicitly released. The owned host service then stopped, followed by a
separate idle check. Original failed-run evidence remains preserved separately.

The capture implementation passed six host test groups; private orchestration
checks covered five mocked guest-phase outcomes, strict metadata handling, eight
control-parser refusals, nine lifetime/restoration cases and deadline expiry.

This is emulator RAM-fixture acceptance. Physical hardware, audio/device stop,
exact scheduling and listening remain separate. See `result.json` for the
publishable candidate and result record.
