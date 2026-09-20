# Recent Projects

DISK OP → RECENT (or Control-Shift-R) opens a ten-entry list without changing
the main tracker layout. Select a row with the mouse or Up/Down; choose OPEN
or press Return. Unsaved edits require a second OPEN/Return. Selecting a
different row or another action cancels that pending discard confirmation.
REMOVE removes the selected entry; CLEAR LIST empties the list. Neither deletes
project files. BACK/Escape returns to disk operations.

Only successfully loaded MOD, PP20 or PTG projects and successfully verified
PTG/MOD saves enter the list. A missing, damaged or unreadable project leaves
the current project, edits and list intact. A failed or refused save is excluded.
The newest successful path goes first; ASCII case-insensitive duplicate paths
are promoted. Native DOS resolves successful paths before remembering them,
so a relative filename remains useful after changing directory. Long paths are
shown by their trailing portion; the complete bounded path is retained.

Preferences use two alternating `ENVARC:ProTracker2.4G/recent.0` / `recent.1`
files. Each contains a generation and its complement followed by a versioned,
length-checked, CRC32-protected list. Startup reads these two preferences files
only, never all listed project paths. An incomplete or damaged newest slot
falls back to the previous valid generation. A save verifies the written list
by rereading it; no write touches the currently newest valid copy. Clearing is
a new empty generation, so it survives a subsequent launch. If neither slot is
valid, startup has an empty list. Preference errors do not roll back a successful
project operation; the status reports that the recent list was not saved.
This is a single-writer preference store, not a concurrent application database.

Tests can override the application-owned prefix with `PT24G_RECENT_PREFIX` to
keep their preferences in an isolated RAM directory. The caller must supply
an existing parent directory. The normal application creates its own ENVARC
directory. Preferences never overwrite song/project destinations.

The portable core validates all paths and list structure before mutation,
stages decoding before replacing state, and has no filesystem calls. The
platform layer owns the bounded preferences I/O. The native application calls
it only after successful document operations and owns the list separately
from the replaceable editor/project state.

Validation is recorded per milestone; host checks and emulator checks do not
constitute physical A1200 or AmiGUS acceptance.
