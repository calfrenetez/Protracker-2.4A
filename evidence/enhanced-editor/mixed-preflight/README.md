Mixed Paula/AmiGUS whole-sequence capability gate

Host ASan/UBSan fixture passes for 8/16/24-bit masters and 8/16-bit AmiGUS
formats, separate route masks, late failures on both backends, same rewound
sequence transfer, global tempo/delay/end flow, allocator refusal and foreign
source-pointer refusal. Existing Paula preflight and wavetable dispatch fixtures
also pass. Initial mixed host test used an oversized consume call; corrected to
renderer-required <=256-frame chunks before build/emulator qualification.

Indexed portable/native targets plus guard compile/link. Manifest records 146
sources, zero generated sources and three binaries; all hashes verified against
the exact source tree. The native Exec mixed fixture passes in the shared 030
emulator in a 5.091-second qualification window with independent subsequent
locked running/all-four-DMA-off and exact run/launcher absence checks. AmiConnect
coordination request, TAKE (240s max) and RELEASE recorded in its task. Overall
200s, runner 140s, fixture 90s bounds. No lifecycle changes or physical probes.

Software preflight and native Fast allocation evidence only. No cache allocation,
master pins, device callbacks, mixed playback ownership, output, timing or physical
acceptance. Selected MIDI and row ranges refuse; no independent backend song
engines. The next requirement is a combined master owner and prepared batch with
one scheduler and safe retention until both backend readers stop.
