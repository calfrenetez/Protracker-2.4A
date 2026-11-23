# Mixed Paula, AmiGUS and MIDI sequence

A Wavetable project can now produce Paula, AmiGUS and MIDI actions from one
software timeline. Each track keeps its assigned route and the existing Paula
map limits playback to four hardware voices. The routed preflight checks the
audio actions against the existing Paula and AmiGUS backend rules. It returns
the same rewound sequence for playback preparation. The ordinary audio render
and Paula editor playback entry points retain their existing behavior.

MIDI actions carry the project track, note and velocity. The sequence shares
position, break, loop, delay and tempo changes across all routes. Lookahead
prepares a common batch before its exact frame. The consumer must commit at that
frame and release all owned voices and notes on stop. Endpoint resolution and
delivery belong to the consumer. Unsupported MIDI effects and MIDI clock flags
fail preflight. This work uses the 16-bit render options and leaves the existing
sample masters intact.

The MIDI preparation code converts routed notes to complete messages ahead of
their frame. It binds endpoint names and MIDI channels from the project and
uses the existing note owner to count shared notes. Only a confirmed batch for
the original frame and preparation serial advances the accepted note state.
Cancelling a refused batch leaves accepted notes intact, so Stop can release
exactly the notes that were accepted. The existing CAMD interface still needs
a delivery backend. CAMD's documented timestamps apply to received messages
and do not schedule outgoing messages.
[CAMD documentation](https://wiki.amigaos.net/wiki/Camd_Library#MIDI_Timestamps)

The timed MIDI owner derives the same half-open counter window as the audio
queue from the shared epoch, rate and counter frequency. It submits the whole
prepared batch before the window and waits for the provider's exact session,
generation, frame and serial receipt. An uncertain or incomplete receipt
retains the batch. A proven early or late issue retains the actual notes for
Stop and reports a timing failure. The software fixture uses a provider model.

The combined provider accepts one audio queue event and its MIDI batch in one
publication. It checks the original session, generation, frame and counter
window, binds the audio queue for the session, and requires advancing audio
tickets. Audio and MIDI receipts must account for every issued action and the
whole activation interval inside the shared logical frame. A partial result
blocks further note submissions and preserves the actual active audio keys
and MIDI notes. A terminal Stop must name all remaining audio keys. If only
the MIDI leg issues, the confirmed MIDI note-offs stay recorded and the audio
leg remains outstanding. An independently detached, unissued audio Stop can
leave those exact readers available for a later frame. The audio queue owns
reader retirement.
The combined fixture uses the genuine audio queue and a modeled provider.
Binding that provider to Paula, AmiGUS and native CAMD delivery remains open.

A tracker frame with audio commands and zero MIDI messages can use the same
combined queue. The prepared boundary requires its audio event. The audio
receipt settles that boundary without reporting fictitious MIDI delivery.
The note ledger carries its accepted voices forward until later note events
or the owned Stop release them.

To stop with a queued MIDI batch, the task can ask the provider to cancel its
exact key once. The schedule blocks further note submissions and waits for the
same terminal delivery receipt whether the cancellation was accepted, refused
or uncertain. If activation won the race, the receipt records the issued notes
before Stop prepares their releases. Providers without a cancellation request
still settle the existing batch before Stop.

The AmiGUS Mini RAM adapter follows the per-longword pacing read used by
[ProTracker 4.1.2.1](https://github.com/akustikrausch/protracker4/blob/v4.1.2.1/src/sys/amigus41.asm#L478-L505).
The existing 2.4G C adapter reads FIFO status after each RAM data write and
latches failure if the FIFO is full. The transfer owner retains responsibility
for cleanup. Credit to the ProTracker developers Lars Hamre, CryptoBurners,
Tom Beyer and Oliver Ickler, and to Andreas Wendorf for the 4.1 additions.
The integration uses the register-access technique in 2.4G's C code. The
[PT4 licence](https://github.com/akustikrausch/protracker4/blob/v4.1.2.1/LICENSE)
describes conditions for direct source reuse.

The 48 kHz host fixture produces both audio attacks and the first MIDI attack at
frame 0. It checks a delayed MIDI attack at frame 6080 after a tempo change,
note release at frame 10080, MIDI note zero at frame 12480 and final release at
the shared F00 boundary at frame 14880. A fake MIDI sink checks exact bytes.
The preflight fixture checks Paula and AmiGUS source masks, missing MIDI route
configuration and a late AmiGUS sample refusal. The synthetic register test
checks the FIFO fault and upload refusal.

The Amiga cross-build covers the routed fixture and the changed native RAM
adapter. The editor's mixed Play path, native scheduled output, CAMD delivery
and physical A1200 timing and listening checks remain open. The
[exact native scheduling decision](NATIVE_TIMING_DECISION.md) governs that work.

The routed MIDI fixture has a logs-only Amiberry runner in
`tools/test_routed_midi_emulator.py`. Use `--fixture wire` for the render
fixture and `--fixture schedule` for the timed provider model. It takes the
exact cross-built executable and SHA-256, the current profile SHA-256, the
emulator and DevBench PIDs and a new evidence directory. Run it using the
shared harness Python, its scripts on `PYTHONPATH` and the lease for the
currently admitted 030 guest. The runner
checks the guest's CPU and the staged bytes before execution. It preserves the
output and verifies the guest exit code before removing its staged files.
Starting, stopping, restoring and releasing the guest remain the session
owner's responsibility. A pass qualifies the core assertions in Amiberry.

Both MIDI fixtures also completed in the shared Amiberry 68030 profile using
a private DOS-only disk with the guest bridge disabled. The guest copied back
the exact executables, returned zero, and produced both complete assertion
logs. The owned guest stopped normally. The shared files were restored and the
reservation was released.
[Offline 030 test receipt](test-evidence/routed_midi_offline_amiberry_20261122.json)

Build the full causal queue fixture with `tools/build_render_sequence.py
--cc <amiga-compiler> --fixture mixed-causal`. Use `--fixture mixed-frame-zero`
for the four first-frame assertions with a flushed progress marker per case.
Both use the project's standard native compiler flags. The original frame-zero
candidate used the compiler's extra optimizer. Its Paula route-check loop
stepped 23 bytes through 22-byte channel records, falsely invalidating the
newly acquired sample cache entry. The standard compiler mitigation preserves
the 22-byte stride.

The full rebuilt fixture reached the 68030 guest and matched its staged bytes.
Its observation window ended without a completion marker. The separate
four-case fixture returned zero in Amiberry and matched the host sanitizer
log. It covered exact frame-zero activation and refusal of submission at the
deadline, activation at the first excluded tick, and clock regression.
[Frame-zero 030 test receipt](test-evidence/mixed_frame_zero_amiberry_20261122.json)

Build the combined fixture with `tools/build_render_sequence.py
--cc <amiga-compiler> --fixture routed-combined`. It drives both audio routes
and a MIDI note through the same frame-zero publication. It checks both partial
outcomes, confirmed cancellation, stale and incomplete audio proofs, timing,
and exact Stop after an audio-only stop result.
The exact native fixture returned zero and completed all seven cases on the
Amiberry 68030. The expanded nine-case build also completed there, adding
audio-only publication and cancellation. [Combined 030 test receipt](test-evidence/routed_combined_amiberry_20261123.json)

The routed tracker fixture uses a complete pattern as its input. Its real
lookahead produces the attacks at frame 0, a volume-slide control at frame 960
while the MIDI note is held, and F00 at frame 1920. The existing normalizer
derives the audio commands, and the sampler factory binds them to held masters
and adopted readers. The modeled joint provider settles them through the
genuine audio queue before the fixture commits the corresponding render interval.
The audio readers and MIDI note are released through their existing ownership
paths. Build it with `tools/build_render_sequence.py
--cc <amiga-compiler> --fixture routed-tracker`.
The Amiberry 68030 run completed at the same three frame boundaries.
[Routed tracker 030 test receipt](test-evidence/routed_tracker_amiberry_20261123.json)

`pt_routed_preflight` audits every reached audio boundary with the same
quantized normalizer used for playback. Only a complete pass can transfer
the original renderer, rewound once for playback. The fixture verifies that
a later looped Paula trigger refuses routed playback before any output
submission, even after earlier mixed boundaries pass the audit.

`pt_routed_playback_prepare` binds the ready forecast to its prepared batch.
`pt_routed_playback_commit` advances that original interval only after live
consumption and the exact complete output receipt. Idle forecasts require no
output receipt. For F00, the schedule snapshots its exact outstanding reader
keys. The sampler factory builds the audio Stop from its original track and
sample registrations. A one-shot that ends early leaves this list only when
the audio owner supplies a complete retirement proof for its original key.
The fixture exercises that path with a short AmiGUS sample and holds the MIDI
note through the next audio-only boundary. Any held outputs retain their
own cancellation and retirement obligations if the render binding is abandoned.

The Amiberry retrigger follow-up adds a real `E91` command at frame 960.
The old and replacement readers share one AmiGUS slot and retain independent
reader keys. The audio owner retires the old key from its original
registration, and the factory rejects a Stop for that retired key before
allocating. F00 stops the replacement at frame 1920 while the held MIDI note
also reaches its scheduled release. [Routed retrigger receipt](test-evidence/routed_tracker_amiberry_20261123.json)

`pt_routed_sampler_boundary` binds one ready lookahead to the original
project and sample grid. It advances the quantizer and factory in bounded
steps, resolves controls against adopted reader keys, and submits the
resulting command through the combined provider once. F00 uses the
schedule's complete set of owned keys. A stale preparation can be abandoned
before submission. After submission, the caller services the original
ticket and retains reader handles through their independent retirement. The
coordinator advances the render interval and its sample origins only after
source consumption and exact combined settlement.

Use `pt_routed_midi_schedule_audio_receipt` on the typed audio owner's
positive detach result before closing the command handle. The schedule checks
the original command and every action key, then derives the earliest and
latest issue tick from the actions. A waiting command stays owned even if its
individual actions have already issued.

The audio activation owner exposes its original event only during the
task-side callback for the exact prepared packet. That callback can use
`pt_routed_activation_publish` to hand off the copied audio and MIDI
commands together. The audio owner now accepts counter reads before the
grid epoch, allowing the frame-zero command to queue before its unchanged
activation window. In the routed activation fixture, one software port
queues four Paula readers, two AmiGUS readers and notes for two distinct
MIDI destinations, then applies their exact Stop from the held keys and
notes. The device output and activation counter are modeled in this fixture.

Explicit transport Stop uses the same scheduled path and owned reader keys
as F00. It submits for the requested absolute frame after the preceding
command has settled or been abandoned with proof. A pending command keeps
its original ownership, and Stop leaves the renderer's next interval in
place. The fixture stops at frame 960 before its original F00 at frame 1920
and checks the complete Note Off and reader releases.

During the original publication callback, the provider can copy the entire
prepared MIDI event with `pt_routed_midi_schedule_copy_packet`. The copy
includes the exact frame, window, generation, serial, audio ticket and all
message bytes. Its numeric endpoint indexes retain the original destination
mapping. A scheduled consumer can use this copy without reading the renderer,
live MIDI planner or project strings.

`pt_camd_routes_resolve` maps those project-order indexes to CAMD sender slots
before playback. Every name must match an owned sender and a receiver in the
latest CAMD scan. A missing receiver blocks the whole map even if its sender
is still open. Resolve again after a scan or link change. The native fixture
checks reversed destination order and distinct Note On and Note Off delivery
to two temporary local receivers. Scheduled CAMD delivery has a separate
timing requirement.

`pt_routed_activation_prepare` copies the Paula DMA geometry and builds the
ordered AmiGUS register programs alongside the MIDI packet during the original
publication. The provider gets one complete prepared object after validation.
The audio owner retains the original reader keys and sample memory. The
activation fixture applies a rate and level change at frame 960 while both
MIDI notes and the original audio readers stay active, then stops them at
frame 1920.

The Paula DMA program separates register preparation for inactive owned
channels from the writes due in the selected frame. Up to four starts share
one DMACON set word and up to four stops share one clear word. Control changes
only period and volume on the original active channel. The master DMA bit and
unrelated channel bits stay untouched. An active-slot retrigger cannot use
this preload path. The encoder checks word alignment and the complete sample
extent within the 2 MiB Chip address range. Before issuing any writes, a
native adapter still needs genuine Chip memory and channel ownership, DMA
readiness, safe register staging and cancellation, and measured timing.
[Commodore Amiga Hardware Reference Manual, audio and DMA control](https://www.ikod.se/wp-content/uploads/2020/08/Amiga_Hardware_Reference_Manual_3rd_Edition.pdf)

For an inactive owned AmiGUS voice, the prepared trigger similarly puts
geometry, rate and level writes in the preload. The frame then needs two
writes per started voice, one to reselect its bank and one to enable it.
Reselection lets other voices be staged in between. An existing active
reader keeps the full trigger path and is ineligible for this preload.
The native adapter must establish real voice quiet and account for staged
register writes on cancellation before releasing sample leases.

The ordinary-RAM activation diagnostic also measured 24 runs on the native
709,379 Hz E-clock in Amiberry. At 48 kHz, one sample frame spans 14 or 15
ticks. Every measured bracket crossed its start frame, including the
standalone adjacent reads used as a comparison.

| Bracket | Observed E-clock ticks |
| --- | ---: |
| Adjacent task reads | 44–1,009 |
| Publish start | 51,850–110,346 |
| Activate start | 5,225–28,430 |
| Publish Stop | 51,704–88,319 |
| Activate Stop | 5,411–18,617 |

The brackets include clock-reading overhead, owner checks and fixture work.
The controlled counter still supplies the fixture's correctness checks. The
measured path needs a prepared, shorter live dispatch before it can use the
original sample window. [Native timing receipt](test-evidence/routed_activation_timing_amiberry_20261123.json)

The private `routed-paula-trace` fixture exercises the prepared Paula words
on the Amiberry custom-chip model. It locks all four channels through
audio.device and uses zeroed Chip buffers and zero volume. It checks the same
INTREQR sample that it records, so request clearing and status readback use
their respective registers. The corrected exact run observed DMA and fresh
request masks of zero at preload, all four at Start and Control, only the
two remaining channels after selective Stop, and zero at final Stop.
The device reset, original owned interrupt state and Chip cleanup completed.
The earlier diagnostics and their mistaken write-register check are retained.
Live musical-frame dispatch and listening have separate acceptance checks.
[AmigaOS audio.device](https://wiki.amigaos.net/wiki/Audio_Device)
