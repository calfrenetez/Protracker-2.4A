# MIDI requester parent-window bracket

`src/native/midi_requester_parent.c` is an unselected native source increment for
bracketing the existing saved-settings window. It changes no default build,
frontend action, accepted layout, owner gate, output route or CAMD state.

The caller owns the original ordinary Task, fixed 640x512 borderless/backdrop
SimpleRefresh parent, its separately created exclusive IDCMP port/signal and the
original Window/Screen/RPort. The Task must have normal interrupt/task nesting.
The actual original Intuition library must be V37 or newer before its BOOL
`ModifyIDCMP` result is used. Matching pointers and signal fields are observations;
they do not establish exclusive port ownership or authenticate reused lifetimes.
Complete original progress/model/controller/editor/history/records/allocator and
GUI storage must be live, stationary and disjoint under their caller contracts.
Known minimum span checks are not a transitive allocation census.

Before entry, the caller installs its durable dispatch/modal guard and suspends
prepared display use. The helper captures the exact original parent, port, screen,
RPort, Task, library, signal bit and nonzero input mask. It isolates input using
only `IDCMP_REFRESHWINDOW`, then restores the captured nonzero mask at most once.
It never installs zero, which can destroy/recreate the IDCMP ports; see the
[classic ModifyIDCMP contract](https://d0.se/autodocs/intuition.library/ModifyIDCMP).

Each pre/post drain permits 128 GetMsg attempts. Reaching that bound conservatively
retains HELD; it is neither queue capacity nor a duration guarantee. Only complete
IntuiMessages naming the original parent are replied. A removed foreign, short or
deformed message stays in the original progress object without reply, requeue or
free. The full original Task/domain retains that unfinished obligation.

Refresh handling performs the caller's complete parent repaint inside
BeginRefresh/EndRefresh. The existing child services only its own port, so parent
refresh may queue while it is open. A HELD child skips further drain/repaint;
one still-trustworthy mask restoration may finish, but cannot clear HELD.
Unfinished refresh and restoration uncertainty retain the original resources
without retry or ordinary cleanup. No pixel/Intuition behavior is proved by
source review or the repaint callback's return.

RESTORED concerns the GUI mask/identities only. The caller must still return its
original genuine context/modal ticket exactly once, keep its guard until that
succeeds and `pt_native_midi_requester_parent_restored` confirms the original GUI,
then genuinely redraw/resume. This helper never returns that ticket, stops audio,
closes a controller/editor/workspace, clears the guard or activates a window.
New/Load/exit must not tear down an enrolled HELD domain. No shipping frontend
lifetime or parent-input integration is activated here.

The [independently reviewed standalone object qualification](../evidence/midi-requester-parent/native-object.json)
completed five commands with
RC0 and empty diagnostics, producing a 2,420-byte HUNK_UNIT. Actual dependency
and ordinary-preprocessor path sets agree: 33 project bodies and 49 genuine SDK
headers, within the separate 34-project/72-header source allowance. All 132 saved
input records, 68 protected records and the saved guard match before/after.
These are compiler facts, not GUI acceptance or a whole-Task stack bound.
No link or window execution is supplied.
Native GUI, Amiberry and real parent restoration are **NOT RUN**; physical testing
is **NOT ADMITTED** for this addition.
