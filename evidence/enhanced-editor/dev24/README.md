# Song-title editing, dev24

Click SONGNAME or use Control-Shift-T for a modal 31-character title. Return
applies one shared undo command; Escape cancels without losing redo. Empty titles
are allowed. Title/channel/resource payloads share a command union so adding
text metadata does not allocate PCM or a separate full-size payload per command.

All 31 host groups pass. Title tests cover length bounds, no-op/redo preservation,
external-change conflicts, history eviction, aliasing input from a redo command,
revision exhaustion, corrupt journal refusal, mixed title/channel history,
controller cancellation/undo and full/cached planar rendering. A 31-character
PTG title round-trips exactly and is refused by strict MOD export. The original
main-screen golden remains unchanged. The updated native editor and native tests
cross-build with the established 68000/soft-float toolchain.

Native run titles1789905326148538000 passed in 192.863 seconds. PTPatternTest and
PTPaulaTest passed. Long-title entry preserved active Paula playback and the
complete project title; strict MOD export refused that title. Mixed title/PCM
undo, cancel/redo preservation, title-only exact MOD byte changes, PTG CRC and
byte-identical reopen/resave all passed. Two editor exits were clean, all four
DMA channels stopped, and guarded emulator cleanup released the private profile.
Screenshots were inspected. The private replay snapshot limits its title to
MOD's 20-character field without truncating the live project or disk export.

This is software/emulator evidence, not physical AmiGUS or ACA1234 acceptance.
The complete source/compiler/binary identities are in core-build.json, and
workflow details in native-title.json. No physical Amiga was controlled.
