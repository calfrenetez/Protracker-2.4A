# Native MOD export and original 2.3F compatibility

Native MOD export produces the unchanged fixture byte-for-byte and the edited
fixture with exactly the independently expected D-2 period bytes. Cancelling,
existing-file refusal and incompatible enhanced-project refusal preserve data,
project dirty state and undo. Reopening, playing and reexporting remains exact.
The native shared MOD exporter and full owned Paula harness passed on this build.
CIA-A timer-B execution remains NOT RUN because Workbench owns it.

The exported edited MOD was also loaded in the pinned original PT2.3F binary.
Its D-2 was visibly present, all four Paula DMA channels were active during play
and off after Stop, and its native Save Module result was byte-identical to the
Enhanced export. A normal confirmed quit returned to Workbench. Retained original
tracker actions used ordinary mouse/keyboard controls through the guarded IPC.
No guest-memory patching, original-source editing or user modules were involved.
The disposable path alias PTMOD: existed only in this private guest session.

These are synthetic emulator compatibility tests, not a complete effect corpus,
physical sound quality, physical timing or AmiGUS acceptance.
