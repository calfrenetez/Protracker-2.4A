# Enhanced native editor integration prototype

`PT24GEdit` is a native Amiga 640x512, four-bitplane editor using the shared
extended-project and pattern-undo cores. It is a separate development executable
while the original assembler tracker/replayer is being integrated. It is not the
finished 16-channel tracker and does not replace the known-good four-channel
`PT2.4G` executable. PLAY and STOP are visibly disabled; no audio backend is called.

The interface follows the supplied four-column reference: the original pinned
2.3F bitmap font, grey raised controls, a black pattern area, twenty visible rows,
four channel-page buttons and permanent P/A/M route letters. Routing is displayed;
route changes, mute/solo and channel-count changes are not yet wired to this UI.
The sample waveform is a static preview of the selected sample's first channel,
not a running scope or an audio-output claim. MIDI labels indicate project routes;
CAMD transmission is not yet connected.

## Start and controls

Build with `AMIGA_CC=/path/to/m68k-amigaos-gcc make core-tests`, then on Amiga:

```
Stack 65536
PT24GEdit song.ptg edited-new.ptg
```

The input may be a supported classic MOD or version 1 project. The optional output
path must be new. The editor and converter share the same staging/readback/new-file
publication adapter. Existing files are refused and current edits remain dirty.
A file requester, ordinary replacement saves and recovery remain to be integrated.
This first editor can make one successful save to the command-line destination;
launch it with a different new path for another saved version.

- Space or EDIT toggles note entry. The initial state is edit off.
- Z–M and Q–U enter tracker notes; the Amiga key positions follow the classic
  chromatic arrangement. F5/F6/F7 select the three classic entry octaves.
- Left/right select note, instrument and the three effect digits; hexadecimal
  keys edit instrument/effect digits. Invalid sample references are refused.
- Up/down move through the 64 rows, with a scrolling twenty-row window.
- Tab/Shift-Tab select the next/previous channel and wrap over the project count.
- F1–F4 or the page buttons select channels 1–4, 5–8, 9–12 or 13–16.
- Backspace in the note field inserts explicit OFF. Delete clears the event.
- Minus/equal decrease/increase the selected sample; the left/right halves of
  the sample value do the same. Pattern buttons select an existing pattern.
- Control-Z / Control-Shift-Z or UNDO/REDO use the bounded pattern journal.
- Control-S / SAVE NEW writes and verifies a project at the new output path.
- Escape / QUIT exits. Dirty edits require a second Escape or QUIT; another
  key/click cancels that discard confirmation.

Raw period values outside the normal three-octave note table display `???` and
remain preserved until explicitly edited. MIDI notes use C-0 for note zero;
notes 120–127 display their numeric MIDI value. This is a display convention,
not an assertion about an external instrument's octave naming.

## Display and acceptance boundaries

The renderer uses 163,840 bytes for a four-plane software canvas, preferring Fast
RAM, plus 163,840 bytes of Chip RAM for a blitter source. The OS display has its
own bitmap. The current bounded editor/history structure uses about 51 KiB on
68k. Loaded project/sample storage is additional. No fixed available-RAM amount
is assumed; allocation failure unwinds all owned resources.

Whole-window updates occur on UI input; font stamps operate on bitmap bytes.
The screen is PAL high-resolution interlaced. A suitable display/flicker handling
and physical ACA1234/Chip-RAM performance still need testing. No 50 Hz redraw or
physical responsiveness result is implied by a successful emulator run.

The UI test uses actual Amiberry keyboard events delivered through Intuition,
compares saved bytes to an independently specified edited project with a fresh
CRC, checks refusal to replace an existing file, reloads/resaves byte-identically,
and exits normally twice. Screenshots require completed-frame acknowledgements;
an early capture before the first draw is not accepted as UI evidence. Host
sanitizer tests cover controller navigation, editing/history/discard handling,
and every page of the same planar renderer. Native mouse interaction and the
full browser/AmiConnect transport remain separate UI checks.

Still open: classic and enhanced frontends sharing the replay engine, pattern
block-operation UI, route/metadata editing and its history, sample editing UI,
file requesters, capture, CAMD, renderer/export strategies and hardware acceptance.
