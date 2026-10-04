# Native workflow controls

These controls extend the existing enhanced editor. The assembler historical
baseline and its display are unchanged. No command here starts enhanced AmiGUS
playback or installs a scheduled audio backend.

| Action | Keyboard | Mouse |
|---|---|---|
| Sample manager | Ctrl-Shift-L | MANAGER in sampler footer |
| Loop / selection toolbox | Ctrl-Shift-K | TOOLS in sampler footer |
| Manager row selection | Up/Down, Left/Right page | Row; always silent |
| Find name | N, then text/Return | FIND footer |
| Eligible-only view / sort ID-name-refs | F / S | ELIGIBLE / SORT |
| Toggle cleanup checkbox | Space | Left checkbox of row |
| Select all eligible / clear selection | A / V | CHECK ALL / CHECK NONE |
| Refresh usage (clears checked selection) | R | REFRESH |
| Apply checked cleanup | D | APPLY |
| Confirm stop before pending cleanup/copy | P | STOP+APPLY |
| Explicit manager audition | F8 | AUDITION |
| Open selected master editor | L / Return | EDITOR |
| Toolbox select complete loop | U | SELECT LOOP |
| Go loop start / exclusive end | S / E | GO START / GO END |
| Copy current frame selection to free slot | C | COPY RANGE |
| Select event resource | Ctrl-Return | NOTE page SELECT RES |
| Open event resource | Ctrl-Shift-Return | NOTE page OPEN RES |
| Return to captured event | Alt-Backspace | EVENT BACK |
| Cancel unpublished workflow | Escape | CANCEL |

Loops page U/USE LOOP is hardened through the same validated frame adapter.
The Loops page also accepts S/E/C for locate/copy. Ctrl-C/V still operate on the
pattern clipboard. Copy Range never uses that clipboard. Toolbox opening
preserves the current sample selection. Ordinary range marking remains two
waveform clicks; each coordinate is a frame boundary, including the exclusive
final boundary. Existing zoom, pan, numeric ranges and format controls remain.

Usage counts include every explicit assignment in every stored pattern and track,
even instrument-only, muted/MIDI tracks and patterns absent from the order list.
They do not count playback occurrences. HOLD means reserved, protected or unknown
persistent ownership; FREE requires unnamed, unconfigured, storage-free and
unreferenced. Names/configuration survive unless that eligible slot is explicitly
checked for cleanup. Checkbox selection starts empty; view filtering/sorting never
renumbers slots or rewrites notes. Refresh or a musical/sample mutation invalidates
an older cleanup preview and clears its checkboxes.

Cleanup/copy requires stopped transport, preview and any attached capture owner.
APPLY/COPY during activity creates a pending request with a visible explanation;
STOP+APPLY is a separate explicit action. A refused release barrier preserves the
project. Active manager audition is refused, rather than replacing a performance.
Preparation yields between chunks and Escape discards it before publication.
Undo storage is preflighted: full journal or memory pressure refuses without
silently purging old history. Cleanup reports freed slots and zero immediate
master reclamation because undo retains the former resources.

Copy retains precision, channels, source rate, name, volume, tuning and
interpolation. It translates a wholly contained valid loop; absent/partly included
loops are disabled. Only markers inside the selected half-open range are retained
and translated. Existing notes, source slots and routes are untouched. A named,
configured, referenced, reserved or unknown slot is never overwritten. The first
proven free slot is used; a new stable slot may append within the 255-slot limit
only when its ownership is also known.

Event navigation labels the event's selected instrument, not the currently
sounding voice. Explicit instrument-only and portamento fields remain direct.
Zero identifiers replay the actual flow/pitch semantics from the known order
context; differing reachable histories are ambiguous, detached contexts are
ambiguous, and exhausted proof budgets are unresolved. Those outcomes preserve
the selected resource. MIDI opens the existing routing page, without treating an
instrument number as a Program Change or sending any message. Empty explicit
slots remain empty targets. Return uses the captured pattern/row/track/order.
The frontend flow mode is explicit: the native supported four-track classic CIA
context uses classic128; enhanced offline context uses extended256.

Changed-version inherited resolver preparation validates the project in bounded,
cancellable steps while stopped and yields before replay. First uncached
preparation remains refused during active playback; same-version cached
resolution advances at most16 ticks per active idle service
(256 while stopped). Waveform summaries read at most4096 PCM values per step and
publish only complete views. Selection/status redraw reuses the completed bins.
A changed viewport displays a preparation placeholder until its own summary is
ready. These are work bounds, not measured 030 wall-clock latency guarantees.
