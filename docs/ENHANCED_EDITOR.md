# Enhanced native editor integration prototype

`PT24GEdit` is a native Amiga 640x512, four-bitplane editor using the shared
extended-project and pattern-undo cores. It is a separate development executable
while the original assembler tracker/replayer is being integrated. It is not the
finished 16-channel tracker and does not replace the known-good four-channel
`PT2.4G` executable. The classic control grid is retained; playback and other unfinished actions
report that they are not connected. No audio backend is called.

The interface follows the supplied four-column reference: the original pinned
2.3F bitmap font, grey raised controls, a black pattern area, twenty visible rows,
four channel-page buttons and permanent P/A/M route letters. Routing is displayed;
route changes, mute/solo and channel-count changes are not yet wired to this UI.
Four quadrascope panes occupy the reference position. Their stopped traces stay
flat until an actual playback scope feed is connected; no waveform activity is
fabricated. MIDI labels indicate project routes;
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
- Minus/equal decrease/increase the selected sample; the paired arrows on the
  SAMPLE row do the same. PATTERN arrows select an existing pattern, and POS
  arrows select an existing order. Other parameter arrows are not wired yet.
- Control-Z / Control-Shift-Z use the bounded pattern journal. EDIT OP. opens
  the UNDO/REDO secondary panel; BACK returns to the reference main screen.
- Control-S or DISK OP. > SAVE NEW writes and verifies the project at the new
  output path. DISK OP. also contains QUIT and BACK.
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


## Reference correction, 19 September 2026

The owner rejected the earlier simplified layout and supplied the full ProTracker
reference again. That rejection supersedes the dev1 visual direction. The main
screen now restores the nine-row parameter table, POS I/D cells, paired arrows,
three-column command grid and numbered buttons, quadrascope/title band, distinct
song/sample-name strips, tempo/status/tune area, twenty-row pattern view and lower
bank/transport/pattern bar. Route letters remain small identifiers in channel
headers. Undo/save move behind EDIT OP. and DISK OP.; they do not replace the
classic main controls. The original font is rendered with wider body spacing.

This is implemented in the actual shared/native renderer, not a pasted reference
image. The layout fixture contains synthetic, editable project events solely to
make spacing and typography easy to review. Its scope traces are stopped. Use
`tools/editor_preview.c` with a fixture, the pinned font, an output PTG and a PPM
path to regenerate it. `tools/test_editor_emulator.py --layout-fixture PATH`
also opens that project in the native app and captures the actual display.

The inherited control labels do not imply completed rendering, sampler, live
recording or replay features. Unconnected actions report their status. Native
keyboard editing, transactional new-file saves and history retain their working
paths. Mouse hit regions now match the revised layout and secondary panels.
