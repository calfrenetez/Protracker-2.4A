# Native AmiGUS call ABI without hardware

The production native adapter's Find/Reserve/Free callbacks execute against a
private Fast-RAM jump-vector table inside the diagnostic process. Independently
transcribed published SFD offsets -30/-36/-42 and assembly stubs capture a0,d0,d1,
a6 and verify full32-bit result transport, PCM/wavetable flags, two distinct library
bases and exact owner pointers. Core reservation tests exercise successful access
lease/close and busy unwind through these native callbacks. Fake open/close callbacks
supply lifecycle without ever opening/closing or registering an Amiga library.

Pinned SDK lock and native compile pass;8input/header hashes match staged sources.
Disassembly confirms expected JSR offsets and captured registers. Candidate20356bytes,
SHAb489462f01c390696018a2f3b70829cb5807866b10a6f18d2b11565b474e7d2b.
Shared030 run render-files-1790547329898643000 passed90sec bound/RC0, one Fast
allocation/zero owned bytes/noChipfallback, all4DMAoff and exact cleanup; independent
run/launcher absence confirmed. No interrupts, MMIO, actual device/library access,
physical operations or audible/realtime acceptance. Real library discovery, hardware
capacity/bus transactions and native output remain separate gates.

Build with tools/build_amigus_reservation.py --abi and the pinned AMIGA_CC.
Run only in a coordinated shared emulator window using --studio-memory native-abi.
