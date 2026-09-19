# Enhanced native editor integration prototype

`PT24GEdit` is a native Amiga 640x512, four-bitplane editor using the shared
extended-project and pattern-undo cores. It is a separate development executable
while the original assembler tracker/replayer is being integrated. It is not the
finished 16-channel tracker and does not replace the known-good four-channel
`PT2.4G` executable. The classic control grid is retained. A first owned Paula/CIA
path connects classic four-channel song/pattern playback and sample audition;
see [playback scope and validation](PAULA_REPLAY.md). Unfinished actions report
that they are not connected.

The interface follows the supplied four-column reference: the original pinned
2.3F bitmap font, grey raised controls, a black pattern area, twenty visible rows,
four channel-page buttons and permanent P/A/M route letters. Routing is displayed;
route changes, mute/solo and channel-count changes are not yet wired to this UI.
Four quadrascope panes occupy the reference position. During Paula playback they
show current voice sample waveforms scaled by volume; stopped traces are flat.
They are not captured audio or phase-accurate DMA scopes. MIDI labels indicate project routes;
CAMD transmission is not yet connected.

## Start and controls

Build with `AMIGA_CC=/path/to/m68k-amigaos-gcc make core-tests`, then on Amiga:

```
Stack 65536
PT24GEdit
; or preselect input/output:
PT24GEdit song.ptg edited-new.ptg
```

The input may be a supported classic MOD or version 1 project. The optional output
path must be new. The editor and converter share the same staging/readback/new-file
publication adapter. Existing files are refused and current edits remain dirty.
Starting from the Shell with no arguments creates a blank four-channel song.
CLEAR / Control-N opens NEW SONG. Choose 1–16 channels with +/- and select CREATE
or press Return. The first four channels default to Paula and additional channels
to AmiGUS. New songs contain one empty pattern, one order and 31 empty classic
sample slots; sample loading/editing still needs integration. Cancel/Escape
preserves the current song. Dirty replacement needs CREATE/Return twice; changing
the count or any other input cancels the discard confirmation. Allocation failure
preserves the old song, history, clipboard and playback. Successful creation stops
old playback and resets the editor/history/clipboard after the staged song commits.
This creates a new song; resizing an existing song remains separate work.
LOAD / Control-O opens a native Amiga ASL requester for a MOD or project.
Control-S opens Save New when no command-line output was supplied. DISK OP. >
SAVE NEW or Control-Shift-S always lets you choose another new filename. A supplied
command-line output remains the Control-S destination; existing files are refused.
DISK OP. > SAVE MOD / Control-Shift-M opens a separate native MOD export dialog.
The shared converter accepts only lossless classic projects. Extra channels,
MIDI, enhanced samples/notes/loops/panning or metadata are refused before a file
requester is opened. No track is dropped and no sample is downconverted. Export
uses the same verified new-file publication, preserves undo history, and does
not mark the richer project saved. Existing MOD files are refused.
Ordinary replacement saves and recovery remain to be integrated.

A dirty project requires LOAD / Control-O twice before the requester opens. Any
other editor action cancels that confirmation. Cancelling a requester or selecting
an invalid file preserves the project and its undo history. Input files are fully
read and closed successfully before the transactional project loader commits.
Successful loading stops the private replay snapshot and resets editor/history
state to the newly loaded document. ASL requires asl.library V39 or later; failure
to allocate/open a requester leaves the document intact. The dialogs follow the
[native ASL file-requester interface](https://wiki.amigaos.net/wiki/ASL_File_Requester).

- EDIT toggles note entry. Space toggles entry when stopped and stops playback when playing.
- F8 / Return / PLAY starts the song; F9 / Shift-Return / PATTERN loops the selected
  pattern. F10 / STOP releases audio. SAMPLE auditions the selected classic sample.
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
- Control-Z / Control-Shift-Z undo/redo one complete event or block command.
- EDIT OP. / Control-E opens the block-editing panel; BACK / Control-E returns.
  MARK / Control-B starts a rectangle at the cursor. Move with arrows, Tab and
  page buttons, then COPY / Control-C freezes and copies the highlighted block.
  MARK again or UNMARK clears the selection. Copy does not change the song.
- PASTE / Control-V replaces events starting at the cursor. It refuses to cross
  row 63 or the last channel; it never clips or spills into another pattern.
- CLEAR / Control-Delete clears the marked block. SEMI - / SEMI + or Control-minus
  / Control-equal transpose it one semitone. Transpose preserves instrument,
  effect, velocity and slice fields; an unsupported raw period or out-of-range
  note refuses the whole operation. Copy, clear and transpose freeze the range.
- ALL / Control-A selects the whole current pattern. To clone it, copy, select
  an existing destination pattern with its arrows, move to row 00/channel 1,
  and paste. One Undo restores the entire previous destination. Pattern/order
  changes clear selection but retain the clipboard. Successful document loads
  reset both; cancelled/invalid loads preserve both.
- Control-S saves to the command-line output or opens Save New. Control-Shift-S
  and DISK OP. > SAVE NEW always open the requester. DISK OP. also contains QUIT and BACK.
- Escape / QUIT exits. Dirty edits require a second Escape or QUIT; another
  key/click cancels that discard confirmation.

Raw period values outside the normal three-octave note table display `???` and
remain preserved until explicitly edited. MIDI notes use C-0 for note zero;
notes 120–127 display their numeric MIDI value. This is a display convention,
not an assertion about an external instrument's octave naming.

## Display and acceptance boundaries

The renderer uses 163,840 bytes for a four-plane software canvas, preferring Fast
RAM, plus 163,840 bytes of Chip RAM for a blitter source. The OS display has its
own bitmap. The current bounded editor/history structure uses about 75 KiB on
68k. Loaded project/sample storage is additional. No fixed available-RAM amount
is assumed; allocation failure unwinds all owned resources.

Cursor moves and event edits now redraw and copy only affected rows and status
regions. Page/scroll/panel changes and returning from file dialogs redraw the
whole scene. Incremental output and its reported dirty rectangles match complete
rendering byte-for-byte through navigation, editing, undo and playback changes.
Six cursor redraws took 14 PAL ticks against 311 for full drawing in the same
private 68030 emulator, with identical pixel hashes: about 95.5% less drawing time.
Three full frames took 155 ticks, compared with 224 before font optimization.
Full-frame drawing remains costly; these are emulator measurements, not ACA1234
performance or end-to-end input-latency results.
The screen is PAL high-resolution interlaced. A suitable display/flicker handling
and physical ACA1234/Chip-RAM performance still need testing. No 50 Hz redraw or
physical responsiveness result is implied by a successful emulator run.

The UI test uses actual Amiberry keyboard events delivered through Intuition,
compares saved bytes to an independently specified edited project with a fresh
CRC, checks refusal to replace an existing file, reloads/resaves byte-identically,
and exits normally twice. Screenshots require completed-frame acknowledgements;
an early capture before the first draw is not accepted as UI evidence. Host
sanitizer tests cover controller navigation, editing/history/discard handling,
and every page of the same planar renderer. Native input.device mouse tests also open Disk Op, return to the main screen,
start/stop song playback and audition/stop a sample while the physical CIA button
bit remains released. The full browser/AmiConnect transport is a separate check.

Still open: full mixed-channel backend dispatch, comprehensive effect parity, pattern
route/metadata editing and its history, sample editing UI,
replacement saves/recovery, capture, CAMD, renderer/export strategies and hardware acceptance.


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

### Second reference refinement, 19 September 2026

The owner described the restored layout as nearly there and supplied the same
reference again. This pass adjusts the actual renderer: shorter main command
rows make room for deeper scopes; the pattern begins four pixels higher and the
bottom controls are taller. Shared layout constants keep the relocated pattern,
channel headers and bottom mouse targets aligned. The original bitmap glyphs
remain, with separate spacing for command labels, numeric fields and pattern
notes/digits. Grey panels are darker, blue notes brighter, and raised edges and
vertical channel dividers are more pronounced. Song/sample labels and the status,
tempo and tune fields follow the reference spacing. The ready status displays
`ALL RIGHT`; detailed action/error messages still use the available two lines.

This is a visual refinement, not a claim of identical photo pixels or owner
acceptance. The 2.4G title, route letters, actual project contents and actual tempo
remain. Scope traces continue to reflect playback and stay flat when stopped.
The test fixture is unchanged musical data, not a recreation of the pictured
song. Hardware acceptance remains separate and has not been run.
