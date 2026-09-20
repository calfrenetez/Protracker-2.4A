# Song construction, dev25

POS ED. / Control-P adds positions, assigns existing patterns and appends empty
patterns. Operations share chronological note/sample/metadata undo. Geometrically
grown, reference-counted event arrays avoid whole-song copies per small edit;
original document storage remains separately owned. The owner has an 8 MiB policy
budget. Allocation failure, invalid references, unexpected nonempty appended
patterns and revision exhaustion preserve project and history. Disposal is safe
after successful document replacement, when original arrays have already gone.

All 32 host groups pass (35.147 seconds). Address/undefined sanitizers cover
allocation failure at every initial/growth allocation, shared capacity, mixed
note/song undo, no-op redo, conflicts, redo truncation, saved-state restoration,
128-command eviction and growth to 256 patterns/positions at 16 channels. Live
song allocations stay below 6 MiB in that maximum growth test; all are released.
Exact PTG and MOD round trips and controller selection/clamping also pass. Full
and cached planar rendering agree through new position controls. The unchanged
main-screen golden is 87ebf44ea470be133f7d47931f04210e5b883b90e3b882a256f1f641bd7450eb.

The native 68000/soft-float build passes. Amiberry run song1789906341304100000
passed in 100.611 seconds. PTSongTest runs the same ownership/undo cases, with
16 patterns for its bounded native growth loop (the full 256 case is host-only).
PTPaulaTest verifies changed positions stop stale replay and a restart uses the
new order list. The actual editor creates a pattern, adds/reassigns positions,
undos/redoes the structure, adds a note, mixes note/order undo, saves and exports.
The MOD byte comparison permits only the exact expected order-count/list,
appended 1024-byte pattern and C-2/sample-1 note changes. Reopened PTG bytes match
exactly, including CRC. Native playback starts at order 1/pattern 1 with period
428. Both editor exits are clean and every DMA channel is off. Screenshots were
inspected. The private profile was released after process/HDF/socket checks.

No real AmiGUS, A1200 or ACA1234 testing was performed. Position deletion/reorder,
pattern deletion and existing-song channel/sample-slot resizing remain open.
