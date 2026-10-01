# Native musical-boundary timing decision

The current prepared owner preserves Fast masters, selective pinned Chip copies,
actual clocks and confirmed stop. Cancellation and half-second pre-startup
Wait/service pass in a scoped calling-task priority5 diagnostic. Production
priority remains unchanged. First-start native qualification is FAILED.

In one guarded shared030 run, the first musical deadline was482118739EClock
ticks; ENTRY observed482119000 and CORE observed482119459, about1.015ms late.
At48kHz, one sample-frame interval is20.83microseconds. The existing core requires
the exact scheduled frame and correctly refused before output. The independent
service clock's47996frames is a different epoch and does not clear this failure.
All resource, priority and signal restoration and independent guest cleanup pass.
Exact evidence: `../evidence/enhanced-editor/paula-first-boundary/`.

A production timing choice is needed before changing this contract or selecting
a materially different native output architecture. Timer-only feasibility work
under the user's continuation authority does not require another approval. Raising priority alone has not cleared
the first boundary. Repeating unchanged tests or relaxing checks silently is not
a solution. Neither option below is implemented or qualified.

| Direction | Concrete implementation consequence | Tradeoff and acceptance |
| --- | --- | --- |
| Preserve the exact boundary contract (recommended) | Prepare and pin a bounded queue of future events/cache leases outside deadlines; give a native backend explicit scheduled-output semantics and completion/stop ownership. Investigate an owned timer/interrupt path or backend-supported scheduled activation. Keep ordinary allocation, conversion and editor work outside that path. Refuse unsupported scheduling capability. | Larger backend change. Requires new interrupt/task synchronization and capability tests, guarded emulator runs, and later physical timing/listening validation. No claim that existing audio.device calls or an interrupt automatically provide sample-exact activation. |
| Explicitly allow bounded live-playback jitter | Add a documented native playback tolerance policy, retaining actual-clock measurements and original musical grid. Report lateness, refuse events beyond the chosen limit, retain bounded work and reader ownership. Exact offline render and master precision remain separate. | Smaller scheduler change, but changes live timing behavior. A user-approved maximum lateness is needed; this one1.015ms observation is not a safe universal bound. Tests must cover within/outside-limit, cumulative drift, missed intervals, stop and callbacks. |

The decision concerns enhanced native live playback. It does not authorize
changing the accepted classic display/replayer, touching physical hardware,
installing diagnostic priority5 as frontend policy, or claiming audio acceptance.
Native AmiGUS access, Studio output/frontend, classic segment/repeat support,
recording device integration and listening/endurance/physical gates remain open
as recorded in SAMPLE_MEMORY.md. Software ownership and master/cache/export
components already implemented remain distinct from these integration gates.

Once chosen, write the explicit backend/policy contract and meaningful failure
tests before another candidate run. Every guest window still needs fresh exact
coordination, live guards, bounded lifetime, independent cleanup and release.

## Feasibility assessment completed after the user's continuation

See `NATIVE_SCHEDULING_FEASIBILITY.md` and the host numerical evidence in
`evidence/enhanced-editor/native-timing-feasibility`. The current audio adapter
has no timestamped activation command; a future queue alone cannot fix this.
The strict core gate admits the correct logical frame (14/15 EClock ticks at
48 kHz), rather than requiring one exact counter tick. Passing that gate is
still distinct from physical first-sample timing. The 720-tick late observation
exceeds that entire window.

The next concrete experiment is a separately owned CIA timer-only diagnostic
before designing its playback backend. Keep the existing gate and classic path
unchanged. This experiment is authorized feasibility work, not a decision point.
Neither IRQ feasibility nor sample-exact hardware timing is qualified by the
host probe. No guest run was needed for the preceding host assessment; new
diagnostic runs must have separate exact candidate and cleanup evidence.

## Timer-only diagnostic prepared; emulator prelaunch refused

The diagnostic and host ownership checks are now implemented. Host checks and
pinned cross-build pass; native execution is **NOT RUN** because the coordinated
shared guest reported paused before staging. The unused TAKE was explicitly
released after independent exact-path absence verification. Pause ownership is
unknown; no automatic resume/reset is authorized. No timing tolerance or output
architecture has been changed. See
`../evidence/enhanced-editor/cia-timer-feasibility/README.md` for exact candidate,
checks, refusal and release evidence. A later run needs fresh coordination.
