# Native scheduled playback feasibility

1 October 2026. Host/source assessment against `b43f1e2`; no new guest run,
native timer acquisition, output, frontend or physical qualification. The user
authorized continued feasibility work until a timing-policy decision. A timer-only
CIA diagnostic is within that authority; it does not select a production output
architecture or lateness policy.

## Finding

A prepared future-event queue is necessary for a new scheduler, but does not
make the current task/Wait adapter meet its deadline. There is no absolute
activation timestamp in `pt_paula_voice_api`, `pt_native_paula_output_start`,
or the underlying `IOAudio` request. The source currently issues `CMD_WRITE`
when start is called and verifies observed DMA activation afterwards. This is
ownership/activation evidence, not a scheduled-start guarantee.

The Amiga documentation describes queued writes, a stopped-channel queue,
`CMD_START` synchronization between channels, and changes synchronized to a
waveform cycle. Those are useful mechanisms, but neither the documented request
fields nor those commands specify an absolute EClock activation target. A
waveform-cycle boundary also need not coincide with a song-command boundary.
This conclusion is an interface assessment, not proof that every possible
audio.device implementation has identical timing.
[Audio Device](https://wiki.amigaos.net/wiki/Audio_Device)

The timer documentation explicitly allows notification after the requested
time because of multitasking overhead. An absolute alarm and a higher task
priority therefore do not supply the missing activation guarantee.
[Adding a Time Request](https://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node00C4.html)

## What the exact gate actually checks

`pt_paula_song_clocked_service` converts the actual EClock counter through
`pt_elapsed_clock_advance`, which floors elapsed time to whole logical frames.
`schedule_step` then requires `now == schedule_start`; a later frame refuses
before initial dispatch. The check does **not** require equality with one
specific EClock tick. For epoch E, frequency F, rate R, target frame N, its
counter admission interval is:

```
E + ceil(N * F / R) <= counter < E + ceil((N + 1) * F / R)
```

The host probe in `evidence/enhanced-editor/native-timing-feasibility` checks
184,200 windows in total, covering one second per configuration across the four
PAL/NTSC and 44.1/48 kHz combinations. Each window's beginning, last admitted tick, tick before and
first refused tick agree with an independent integer oracle. Sanitizers pass.

| EClock frequency | Logical rate | Admission width in EClock ticks |
| --- | --- | --- |
| 709379 | 44100 | 16 or 17 |
| 709379 | 48000 | 14 or 15 |
| 715909 | 44100 | 16 or 17 |
| 715909 | 48000 | 14 or 15 |

This is approximately one logical frame (20.83 microseconds at 48 kHz), not a
1 ms tolerance. The existing boundary failure was 720 ticks after the first
reaching counter deadline at CORE, about 1.015 ms at 709379 Hz. It was outside
the entire admission interval. ENTRY/CORE clock observations, command issue,
DMA enable observation and physical first-sample output are different events.
An admitted clock observation alone proves neither command completion within
that frame nor physical sample accuracy. Paula's pitch periods also use its
hardware clock; the logical 48 kHz song timeline is not a claim that each Paula
voice is clocked at 48 kHz.

The probe validates the integer conversion only. The gate behavior above is
source inspection of `src/editor/paula_song.c`, with input hashes recorded in
`frame-windows.json`; this probe neither executes the song nor measures latency.

## Candidate that preserves the current strict refusal

A separately owned CIA timer interrupt can remove the ordinary task wakeup
from the activation path. The documented resource allocates a free timer with
`AddICRVector`; an occupied timer must be left alone. Only the acquired timer
may be programmed, disabled or released. Availability cannot be assumed from
the system configuration, and interrupt code must preserve the documented ABI.
No documentation here establishes a universal interrupt-latency bound.
[CIA Resource](https://wiki.amigaos.net/wiki/CIA_Resource)

The classic bridge already acquires a CIA resource and controls Paula, through
`src/native/replay_abi.s` and the generated replay from `tools/prepare_replay.py`.
This establishes a repository mechanism to inspect; it is not qualification
of an enhanced scheduler. Preserve the classic frontend/replayer. Enhanced and
classic playback cannot independently control the same timer/channels.

The current enhanced core and Studio queue explicitly use a serialized task
owner. Song validation traverses mutable ownership structures and can invoke
failure cleanup; the current device adapter also polls/completes device I/O.
Calling those complete paths from an interrupt is not a safe substitution.
A candidate would need a distinct, bounded publication/activation boundary:

1. The task prepares complete immutable action descriptors and all master/cache
   leases before publishing a fixed-capacity batch. An exhausted queue refuses;
   it never allocates, converts, evicts or overwrites a live descriptor in IRQ.
2. Publication and cancellation use explicit task/interrupt exclusion. An
   interrupt sees either a fully published batch or none, with a generation and
   original absolute timeline. A task signal can request refill; it cannot be
   the final deadline-sensitive activation mechanism.
3. The handler performs only separately qualified bounded clock checks and
   hardware operations. Record actual observation/issue times; preserve the
   strict gate and refuse unsupported timing. Do not pass predicted timestamps
   to existing code as if they were actual observations.
4. Completion/cancellation is acknowledged before leases are released. Stop
   publication, confirm the owned interrupt is inactive/removed and all owned
   DMA readers are stopped, then return channels and storage. Uncertain cleanup
   retains the live owner and storage, without automatic reset, retry or exit.

The first implementation block would be a **timer-only diagnostic**: fixed
descriptors, owned free CIA timer, handler entry timestamps on the immutable
frame grid, independent task termination, no Paula writes, and exact resource
restoration. Native capability remains refused until that and a subsequent
silent activation candidate pass. A successful emulator diagnostic would still
not establish worst-case physical timing or listening acceptance.

## Alternative and decision boundary

An explicitly bounded live-lateness policy can keep more of the existing task
adapter, but changes the product timing contract. It needs a chosen maximum,
measured actual clocks, the original musical grid, missed-event/drift tests and
confirmed stop. One observed 1.015 ms delay cannot select a safe maximum.

Buffered Studio rendering can place events exactly within prepared PCM data,
using the existing master-pinned mixer and bounded queue. It remains a separate
native output integration: it does not implement selective direct Paula voices,
does not establish absolute wall-clock start, and must negotiate real output
format/rate and ownership. Do not silently substitute it for the Paula route.

Recommendation: investigate a **dedicated CIA scheduled backend,
retaining the existing strict frame refusal**, starting with the timer-only
diagnostic above. This is a feasibility experiment, not a promise of universal
sample-exact hardware activation. If it cannot pass, report that result before
changing timing tolerance or substituting a rendered-output route.

The timer-only investigation can proceed under existing authority. It is not a
reason to pause. A user decision is required if the findings demand relaxing the
timing requirement or substituting a materially different playback approach.
The original host assessment installed no runtime change or hardware control;
subsequent diagnostic evidence must be recorded separately.

## Timer-only diagnostic prepared; emulator prelaunch refused

The diagnostic and host ownership checks are now implemented. Host checks and
pinned cross-build pass; native execution is **NOT RUN** because the coordinated
shared guest reported paused before staging. The unused TAKE was explicitly
released after independent exact-path absence verification. Pause ownership is
unknown; no automatic resume/reset is authorized. No timing tolerance or output
architecture has been changed. See
`../evidence/enhanced-editor/cia-timer-feasibility/README.md` for exact candidate,
checks, refusal and release evidence. A later run needs fresh coordination.

## Native timer measurements after explicit resume approval

Two separately coordinated timer-only runs completed and safely closed their
owned resources, but FAILED unchanged strict timing:84..98ticks late, then
47..53ticks after moving setup ahead of the actualclock read. Each collected
16actual IRQ observations, with independent cutoff and2Fastallocationszero.
Pre/post programming spans72..75 then40..43ticks mean these are not pure IRQ
latency bounds. No playback or physical timing qualification follows. Exact
evidence is in `../evidence/enhanced-editor/cia-timer-native/` and
`cia-absolute-arm/`. These supersede the paused guest blocker for those runs.

A distinct native critical-arm diagnostic is built; it checks the same64-bit
count arithmetic on12boundary cases BEFORE timer acquisition, then shortens the
actual ReadEClock/count/START interval. No empirically chosen lead or tolerance.
It remains NOT RUN because the sharedlock was reserved for AmiGUS Safari
installation before Guest construction. Do not override that reservation. See
`../evidence/enhanced-editor/cia-native-arm/`. Fresh coordination/guards are still
required when the window is available; this is ongoing feasibility, not approval
of a new output architecture or an argument for relaxed timing.


## Third native critical-arm measurement and recovered cleanup

1 October 2026. The assembly critical-arm candidate now ran once after fresh
coordination. Twelve actual assembler arithmetic boundary checks PASS; unchanged
strict gate FAILED with16late observations44..47ticks at709379Hz, programming
span36..37ticks. Owned native resources closed, but initial independent filesystem
cleanup FAILED because an empty test directory reappeared. The immediate runner
cleanup claim is preserved and superseded, not used as acceptance. Exact guarded
empty-directory recovery and separate subsequent verification PASS; window
explicitly released after inspecting both records. No emulator lifecycle, test
retry or output/tolerance/priority change. See
`../evidence/enhanced-editor/cia-critical-native/` for separate evidence tiers.

The user now authorizes real-Amiga/AmiGUS testing. Physical timing and positive
AmiGUS capability remain untested; current candidate discovery is being qualified
separately. These observations do not prove universal CIA infeasibility or permit
relaxed timing. No production scheduling policy has changed.
