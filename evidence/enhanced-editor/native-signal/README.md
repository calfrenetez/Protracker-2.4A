# Signal wakeup diagnostic — 27 September 2026

Native fixture change only; production timer/clock/dispatch unchanged.16alternating
2/10ms absolute alarms, Exec Wait on alarm OR already-armed independent2s watchdog,
distinct signal masks, bounded spurious-wake loops and actual counter resampling.
No audio/playback callbacks. Logs deferred until all measured cases complete.

Native build/main syntax PASS;170staged inputs/font/binary hashes verified.
Binary398012bytes SHA256
8b31b3a8d5127277a2d6469134c8f2d6a5c87a9e3092f935584c490c597e7176.
Shared030 render-files-1790505919304448000 PASS within90s RC0. All replies collected,
watchdog cancelled, resources closed.1182Fast allocations/zeroowned/noChipfallback.
All4DMAoff/exactrun and launcher cleanup/independentabsence verified. Explicitly
released shared030/DevBench; no hold or physical/lifecycle/config operations.

At709379Hz observed lateness193..10045ticks (about0.272..14.160ms).15cases were
193..198ticks; initialcase10045. All16exceeded one48kHz sample-frame interval.
These measurements include task switching, poll/WaitIO and clock-read overhead;
they characterize this emulator diagnostic, not intrinsic timer or physical
hardware. They do NOT qualify feeding this task-wakeup path directly to the
strict exact-frame song gate. That gate's no-late-dispatch policy is unchanged.
Next: native real-clock late-wake refusal/lease-cleanup diagnostic with injected
voice callbacks, then separate prepared-dispatch/timing strategy work. No relaxed
lateness tolerance or native/card PLAY enabled by this evidence.
