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
four channel-page buttons and permanent P/A/M route letters. Clicking a route
letter or Control-R opens CHANNEL settings. Route, mute and solo changes share
chronological undo with note/block edits and are saved in the project.
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
sample slots. The SAMPLER page imports WAV into a selected slot. Cancel/Escape
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
  SAMPLE row do the same. FINETUNE arrows adjust -8–7 and VOLUME arrows adjust
  00–40 hexadecimal (0–64). Alt-up/down changes finetune; Alt-right/left changes
  volume. PATTERN arrows select an existing pattern, and POS arrows select an
  existing order. Other parameter arrows are not wired yet.
- Control-Z / Control-Shift-Z undo/redo one complete event, block, channel or sample change.
- CHANNEL / Control-R: select PAULA, AMIGUS or MIDI; routes are exclusive and a
  fifth Paula assignment is refused. P/A/M select the route while this page is
  open. U toggles MUTE and S toggles SOLO; highlighted buttons show saved state.
  Multiple solos are allowed and mute takes precedence. PREV/NEXT, Tab/Shift-Tab,
  left/right and page buttons select the channel. BACK/Escape closes the page.
  Note-entry keys are isolated from pattern entry while the page is open.
  Header M/S indicators show active flags; the rightmost P/A/M is the route.
  Route changes preserve existing notes, samples, panning and MIDI settings;
  they do not translate pitches or set up a hardware connection. An unsupported
  route change during classic playback stops that backend. Classic four-channel
  Paula playback supports live mute/solo and undo/redo; mixed replay is pending.
  Saved mute/solo are enhanced metadata, so strict MOD export refuses them.
  DETAILS / D opens channel pan, group, MIDI-channel and name controls. P enters
  hexadecimal pan 00–FF (left–right); G enters group 00–0F (00 ungrouped); M enters
  decimal MIDI channel 1–16; N edits the 15-character track name. Click the matching
  button or name row for the same action. Return applies one shared undo command;
  Escape cancels without disturbing redo. A new first character replaces the old
  value; Backspace/Delete clears the initial value or removes the final character.
  Names accept uppercase A–Z, digits, space, hyphen and period using Amiga raw-key
  positions. Input is modal: navigation, transport, clicks and shortcuts cannot
  change the selected channel until Return/Escape. Details BACK/Escape returns to
  the routing page; a second closes it. Names, groups, pan and MIDI assignment
  persist in PTG, with chronological undo/redo. Group is organisational metadata;
  enhanced pan and MIDI assignment are not yet connected to an audio/MIDI backend.
  Classic Paula playback accepts names/groups/dormant MIDI assignment and retains
  its physical stereo placement; changed pan still requires enhanced replay.
  Strict MOD export refuses these
  non-default enhanced values, including MIDI assignment on a Paula-routed track.
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

## Sampler page

SAMPLER / Control-L opens a sample waveform page in the existing classic screen.
The pattern view returns on Escape. +/- and the SAMPLE arrows choose the
sample slot. A mono waveform uses one pane; stereo uses separate left/right panes.
Each horizontal pixel displays its minimum/maximum sample values. This is stored
sample data, not live captured audio. Two clicks select a half-open frame range;
the yellow marker shows a pending first endpoint. Edits are refused until that
range is completed or reset. ALL / A selects the full sample.
Undo/redo of a sample change resets the range to the full current sample.

LOAD SMP / L (also Control-Shift-O while this page is open) uses the native file
requester. It accepts integer PCM WAV, 8/16/24-bit, mono/stereo, up to 192 kHz,
and single-octave mono IFF/8SVX as described below.
Import retains exact decoded precision, rate and channels; no downmix, resampling
or precision conversion occurs. A WAV replaces the selected slot with volume 64,
finetune zero and no loop/slice metadata. Undo restores the complete old sample.
A replacement is refused if any event references its existing slices. WAV loop,
cue and other ancillary metadata are not imported. Recording and adding sample slots beyond the existing document remain open.

REVERSE / R, NORMALIZE / N, DC OFFS / D, GAIN X2 / G, GAIN /2 / H, FADE IN / I
and FADE OUT / O act on the selected range. Stereo frames stay paired. Normalize
uses a shared peak, gain saturates at the declared precision, fades reach zero,
and DC removal calculates each channel independently. Existing sample loop/slice
positions are preserved during these length-preserving edits. FORMAT provides
explicit whole-sample rate and precision conversion.

Each change joins the same chronological undo/redo as notes and channel settings.
Staged immutable sample versions are reference-counted; command eviction or redo
truncation cannot release the active sample. Retained/staged sample versions have
a 32 MiB policy budget, additional to loaded document storage and the input file.
Temporary input files are limited to 64 MiB. Allocation/budget/validation failure
preserves the project and journal. The budget is a ceiling, not an assumption
about available RAM. Native allocations may fail below it. Project replacement
and normal exit release all editor-owned versions.

SAVE WAV / W exports the entire selected sample, retaining PCM precision, channels
and rate. It uses verified new-file publication and never replaces an existing
file. WAV export does not mark the project saved. Control-S still saves the full
project, including edited samples, and Control-Z/Control-Shift-Z undo/redo.

AUDITION / Return / F8 uses the existing classic Paula audition backend. It only
accepts compatible classic samples on a Paula route; high-resolution/stereo audition and AmiGUS preview remain pending. Successful sample imports, edits
and sample undo/redo stop existing song/audition playback before another action
can use its old private copy. Failed/cancelled/no-op operations preserve playback.
Pattern edits and channel settings keep their previously documented behaviour.

## Loop and slice tabs

The sampler heading contains SAMPLER, LOOPS, SLICES and RANGE tabs; Tab cycles them.
The same two-click range, A/Control-A for all, +/- sample selection, audition,
Control-S and shared undo remain available. Escape closes all sample pages.
The accepted main-screen grid and bitmap font are unchanged.

LOOPS sets FORWARD / F or PINGPONG / P from the selected half-open frame range.
OFF / O clears loop metadata. USE LOOP / U selects the current loop range.
Yellow brackets show saved loop boundaries. FADE -/+ or [/] halve/double the
crossfade length (1–65536 frames, initially 32). BAKE FADE / B crossfades the
selected tail with its head, then sets a forward loop starting after the consumed
head. It changes PCM and metadata together in one undo command. The fade must
fit within half the selection; it is never silently shortened. Baking does not
also enable runtime crossfade. Pingpong is stored correctly but enhanced pingpong
playback remains unavailable; classic audition refuses it. Forward audition still
requires all existing classic format/alignment constraints.

SLICES edits marker proposals without chopping or changing PCM. ADD START / M
adds the selected range start. DELETE / D removes the last marker at or before
that start; CLEAR / C proposes an empty set. AUTO / T replaces the proposal with
an offline transient analysis. Defaults: threshold 500/1000 of peak, minimum
spacing 50 ms, exponential envelope shift 4 and optional nearest shared-channel
zero crossing within 32 frames. THRESH -/+ or [/] adjust the threshold by 50
(50–1000; lower is more sensitive). GAP -/+ or down/up adjust spacing by 10 ms
(10–1000). Click the middle GAP value or Z to toggle zero-crossing refinement.
Changing options requires AUTO again. There are at most 4096 sorted markers;
nonempty AUTO results begin at frame zero, including silent samples.

Proposed markers appear yellow and saved markers white; the heading explicitly
identifies proposals. Manual edits also work on the AUTO proposal. APPLY / P
publishes all markers as one undo command; CANCEL / X preserves the sample,
dirty state and redo. Proposals are previews, not saved project data. Changing
sample slots, importing/replacing the sample, or undoing a sample change discards
stale proposals. A failed apply retains the proposal for correction. Final loops
and markers survive project save/reopen; WAV export still contains PCM only.

Existing pattern slice references are 1-based ordinals. Applying, undoing or
redoing a marker change must preserve the frame addressed by each referenced
ordinal. A change that would remove or retarget one is refused. Automatic note
remapping, pattern slice assignment controls, slice-trigger playback and MIDI
slice triggering remain pending. The current milestone is editing and persistence,
not a claim that enhanced playback is connected. Metadata-only edits still keep
immutable PCM snapshots within the existing sample-history memory budget.

## Waveform zoom, exact ranges and format conversion

RANGE separates the waveform viewport from the edit selection. ZOOM IN / I and
ZOOM OUT / O halve/double the visible span; PAN </> or left/right move half a
viewport and stop at sample boundaries. FIT ALL / F shows the full sample.
ZOOM SEL / V shows exactly the selected range, down to one frame. Selection,
PCM, dirty state and undo history do not change when navigating. Waveform clicks,
loop brackets and saved/proposed slice markers use the same zoomed frame range;
markers outside the viewport are clipped rather than clamped onto a false edge.
Changing sample slots or changing frame count restores the full viewport.

START/END -/+ adjust one frame; [/] adjust start, Shift-[ / Shift-] adjust end.
Click a numeric value or press S/E to enter an exact decimal endpoint. The first
digit replaces the old value, Backspace/Delete removes digits, Return validates,
and Escape cancels just the entry. Invalid, inverted, empty, overflowing and
out-of-sample ranges retain the old selection. While a number is being entered,
other actions cannot edit the project or open a requester. All ranges are half-open.

SAMPLER > FORMAT, or C on SAMPLER/RANGE, opens conversion targets. Choose 8/16/24
bits (keys 1/2/3), a rate preset (8287, 22050, 44100 or 48000 Hz), or click the
rate value / R to enter 1–192000 Hz. FILTERED / LINEAR or F switches quality;
filtered is the default. Tab returns to SAMPLER. These are proposals until APPLY / P; choosing
or cancelling values does not edit the sample. Conversion always covers the
whole selected sample, independent of the waveform selection. Precision reduction
rounds to nearest with ties away from zero and saturates; widening retains exact
values at the new scale. There is no dither in this conversion path.

Filtered rate conversion uses an integer, 16-zero-crossing Blackman-windowed
sinc with antialias filtering. A generated Q24 kernel table and integer arithmetic
avoid any runtime FPU dependency. A bounded 16 KiB stack workspace caches a
phase kernel; repeated phases reuse it. Q14 table interpolation and rational phase
accumulation avoid 64-bit division per tap. The documented 64 KiB Shell stack
remains required. Stereo channels share phase/coefficients;
normalization retains exact DC gain, endpoints extend the nearest source frame,
and output saturates to the declared precision. Filtered reduction is bounded to
128:1 per operation; larger ratios are refused before staging. LINEAR remains an
explicit faster mode without an antialias filter. PCM remains mono/stereo as supplied.
The complete classic-song conversion/bounce engine is still separate work.
The frame count rounds up, loop starts and slice frames round down, and exclusive
loop ends round up. Slice ordinals remain attached to the corresponding scaled
markers. Collapsed markers or invalid crossfade geometry refuse the whole change.
No marker is silently removed and no implicit downmix occurs.

PCM, precision, rate and all scaled metadata publish as one bounded shared undo
command. Undo restores original full-precision PCM and exact old frame positions;
redo restores the converted version. Failures and unchanged targets preserve redo
and current playback. Successful conversion stops the old audio snapshot, clears
stale marker proposals and selects the complete result. Project/WAV saves retain
the converted values; hardware/backend limits on audition still apply.

Native filtered conversion shows percentage progress and accepts Escape to cancel.
The original sample remains live while a private destination is computed. Cancelling
at any progress point releases that destination without changing project data,
dirty state, redo or playback. Other editor inputs are ignored while conversion
is modal; window events are drained/replied and refreshes are handled. The progress
callback is polled in bounded work chunks (at most 32 output frames or roughly
4096 filter taps). Progress redraws occur in 5% steps. This is a software mechanism,
not a physical ACA1234 timing or responsiveness measurement.

## Display and acceptance boundaries

The renderer uses 163,840 bytes for a four-plane software canvas, preferring Fast
RAM, plus 163,840 bytes of Chip RAM for a blitter source. The OS display has its
own bitmap. The current bounded editor/history structure uses about 99 KiB on
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
slice assignment, track names/groups and existing-song resizing,
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

### Button typography and relief, 19 September 2026

Following the owner's request for better fonts and pseudo-3D controls, labels now
use twelve-pixel spacing for the original twelve-pixel bitmap glyphs, with a
bounded eleven/ten-pixel fallback only for narrow controls. This separates adjacent
letters and their shadows. Raised labels have a consistent two-level lower-right
shadow. Buttons and arrow controls gain paired highlight/shadow edges and subtle
face shading. Selected buttons reverse the bevel and inset their lettering by one
pixel. Surrounding information strips keep their flatter treatment, so controls
read as raised surfaces. Layout, hit regions, project data and font source remain
unchanged. Existing full/incremental pixel tests include selected controls.

### Shared border grid, 19 September 2026

The owner's alignment feedback exposed a 19-pixel parameter grid beside an
18-pixel command grid. Both now use the same 19-pixel rows and mouse boundaries.
The scope title, two-row scope area and version strip share those row boundaries;
secondary command panels no longer overlap the scope title. A single pattern
header and uninterrupted rails replace the two-pixel jog at the channel headings.
Rails reach the bottom bar, the sample/status strips have separate edges, and
five-digit parameter values stay inside their numeric-field bevels.

Native playback also used stale presentation rectangles after earlier layout
changes. Cached rendering and native live ticks now use one shared rectangle
list. The BPM clear area includes the last glyph column, preventing remnants as
tempo digits change. Host checks assert actual border pixels and button boundary
behaviour, then compare both cached and tick-only updates with full rendering.
The optional `--playback-check` native editor test verifies a real MOD F96 tempo
change to 150 BPM against independently drawn font pixels without requesting a full redraw.


## IFF/8SVX sample interchange

LOAD SMP / L autodetects PCM WAV or FORM 8SVX from content. IFF accepts a single
mono octave, signed 8-bit PCM or Fibonacci-delta compression. NAME, sample rate,
forward-loop start/end and volume are imported with the PCM in one undo command.
BODY data after the loop is retained, matching the pinned 2.3F writer. Names are
bounded to 31 bytes; 16.16 IFF volume is rounded to the nearest project value 0–64.
Multi-octave files, stereo CHAN=6, other compression and malformed chunk/loop
bounds are refused. Mono CHAN=2/4 is accepted as mono; channel-side hints, octave
pitch hints, envelopes and annotations are not represented in the project.

SAVE IFF on LOOPS / W, or Shift-W on SAMPLER, exports a new uncompressed FORM 8SVX
file with name, rate, volume and forward loop. It uses the existing verified
new-file writer and never marks the richer project saved. Export requires a
nonempty mono 8-bit sample at 1–65535 Hz, no slice markers, no finetune and either
no loop or a forward loop. Other settings refuse before a requester opens; use
explicit FORMAT conversion first where appropriate. WAV export remains W on
SAMPLER and contains PCM only. Neither export removes project metadata.

The bounded parser follows the [AmigaOS 8SVX specification](https://wiki.amigaos.net/wiki/8SVX_IFF_8-Bit_Sampled_Voice).
IFF imports do not imply enhanced playback is available. Selected-instrument MOD import is described below.


## Explicit headerless RAW interchange

RAW on SAMPLER, or X, opens an additional page without changing PCM or history.
The visible defaults are signed 8-bit mono, big endian, 8287 Hz. Choose 8/16/24-bit
(1/2/3), MONO / M or STEREO / S, byte order (buttons or E), signed/unsigned8 (U)
and rate (R / numeric value). Unsigned is available only at 8 bits; selecting
16/24 bits sets signed representation. Rate can be entered before loading an
empty slot. Return validates 1–192000 Hz; Escape cancels number entry. Tab/BACK
returns to SAMPLER. A still selects the whole sample on sampler pages.

LOAD RAW / L requires these explicit settings. Ordinary LOAD SMP never guesses
RAW from unknown bytes. File length must be frame-aligned; stereo is interleaved.
Import retains exact signed 8/16/24-bit values, replaces the chosen slot with
volume 64 and no loop/finetune/slices, and shares the atomic sample undo journal.
Existing referenced slices prevent replacement. Invalid input, allocation failure
and cancellation preserve the project and redo.

SAVE RAW / W writes the complete sample as headerless PCM, using the selected
signedness/byte order. Bits, channels and rate must exactly match the current
sample; mismatches refuse before opening a requester. Use FORMAT for explicit
conversion first. RAW has no name/rate/loop/slice metadata; keep the PTG project
and remember the displayed settings for reimport. Export does not mutate or mark
the project saved. The verified new-file writer refuses existing destinations.


## PowerPacker PP20 module loading

LOAD / Control-O and the initial PT24GEdit input detect PP20 by content, independent
of filename. PT24GConvert uses the same loader. The whole compressed bitstream is
structurally checked before decoding: backwards input bounds, efficiency widths,
skipped bits, literal/match lengths and references to already produced bytes.
Decoded output is a separate allocation and passes strict normal-MOD preflight
before it can replace the current project. Invalid input, insufficient peak
memory or allocation failure retains the song and editor history. Temporary
unpacked storage counts against the caller's memory budget and is always released.

A genuine upstream packed MOD is checked against its published decoded checksum,
then round-tripped through both converter and native editor to exact ordinary MOD
bytes. PP20 has no integrity checksum: structural validation cannot detect every
bit change that still describes a valid stream and valid MOD. The maximum decoded
length is the format's 24-bit size field. Encryption (PX20), nested compression,
packed enhanced projects and PP20 saving are unsupported. Save compatible songs
as ordinary MOD, or retain enhanced data in PTG.

The native 2.3F assembler's separate powerpacker.library path is unchanged. The
Enhanced editor uses a bounded adaptation of the explicitly public-domain PP20
algorithm in libxmp, with pinned source and notices in `vendor/pp20-reference/`.
No runtime library installation is required for this path. Musical third-party
regression input remains local and is not redistributed in development packages.


## Import one instrument from another MOD

LOAD SMP / L on SAMPLER now recognises a normal MOD or PP20 MOD and opens a separate
read-only source page. Shift-L requests a MOD source directly. The current song,
selected destination and undo journal remain unchanged while loading/browsing.
The source waveform and name are labelled SOURCE; the main song/parameter strips
still describe the destination project. Source storage is separate from the song.

SOURCE </> / left/right selects instrument 01–1F; DEST </> / -/+ selects the
existing destination slot. IMPORT / I / Return copies PCM, name, rate, volume,
finetune and loop as one shared undo command. Empty source instruments refuse.
The current song's orders, patterns, channels and other sample slots are preserved.
Referenced destination slices cannot be removed or retargeted by an import.

LOAD MOD / L changes source; cancellation or invalid input retains the previous
source and selection. CLOSE / Escape / Tab releases source storage and returns
to SAMPLER. Applied imports remain in the song and can still be undone/redone.
Control-Z / Control-Shift-Z work while the source page is open. Control-S saves
the current song, not the donor. STOP / Space / F10 stops existing playback;
source audition is not connected. Preview clicks cannot edit source PCM.

The loaded donor document reserves space from the sampler's existing 32 MiB policy
budget until closed. Temporary PP20 storage is also budgeted during load; allocation
failure preserves both documents. Imports deep-copy the selected sample, so closing
or replacing the source never invalidates imported data or undo resources. Project
load/creation/exit also releases source storage. Enhanced PTG files are not accepted
as MOD donors. The input buffer and original current-song storage remain additional
allocations, as described under sampler memory limits.

## Assign a slice to a pattern note

EDIT OP. > NOTE or Control-I opens the selected pattern event's slice page while
retaining the four-column pattern grid. The title identifies pattern/row in hex,
channel in decimal and the note's explicit sample instrument in hex. Select a
normal sample note first. The middle control shows the assigned slice and total
marker count in hex: 0000 means the whole sample; 0001 is its first marker.
A slice runs from that marker to the next marker, or the sample end. The displayed
frame range is half-open: its end frame is excluded. PCM is never split or copied
by assigning a slice.

SLICE -/+ or -/+ steps the assignment; click the middle value or press S for exact
hexadecimal entry. Return applies one pattern undo command; Escape cancels and
preserves redo. A nonzero slice requires a normal sample note, explicit sample
instrument and existing marker. Invalid assignments leave the note unchanged.
NO SLICE / C clears only the slice reference. Other note fields, effects, velocity
and PCM remain unchanged. This UI does not assign slices to MIDI notes or OFF.

USE SMP / U explicitly attaches the currently selected sample slot to an existing
sample note, including an instrument-zero note. The target must have markers and
must accommodate any existing slice reference; otherwise the change is refused.
Use [/] or the main SAMPLE arrows to choose the slot. There is no inference from
an earlier note's instrument. SAMPLER / L opens the note's sample for marker work;
Control-I returns to the note page. Up/down selects rows; Tab/Shift-Tab, left/right
and F1–F4 select channels. These controls cannot change the target during numeric
entry. Other note-entry keys are isolated while this settings page is open.

UNDO/REDO and Control-Z/Control-Shift-Z follow the same chronological journal as
marker edits. Referenced marker removal or start retargeting is refused until the
note references are cleared. PTG stores these assignments exactly. BACK/Escape or
Control-I returns to EDIT OP. Slice playback through AmiGUS/enhanced preview is
still pending; classic Paula playback and direct MOD export refuse sliced data.


## Sample names, volume and finetune

Click the sample-name strip or press Control-Shift-N to edit up to 31 characters.
Return applies and Escape cancels without losing redo. Names use the same raw-key
uppercase letters, digits, spaces, hyphen and period as track names. Entry is
modal: the selected sample cannot change until it is completed or cancelled.
FINETUNE/VOLUME arrows and the Alt-arrow shortcuts edit bounded metadata, with
no wraparound. These edits share chronological undo with PCM, loops, slices,
notes and channel settings. Project saves preserve all 31 name characters;
strict MOD export refuses names longer than its 22-character sample-name field.
Compatible volume/finetune/name changes export without changing PCM or patterns.

Metadata versions share one immutable owned PCM/marker allocation. For a sample
still backed by the initially loaded document, the first metadata edit captures
one owned baseline; subsequent name/volume/finetune versions need only a small
header. Existing owned sampler versions need no initial PCM copy. History uses
flat backing references, so repeated metadata edits do not build recursive
ownership chains or duplicate a large sample. PCM edits still create independent
versions. Allocation/budget failure leaves the sample and redo intact. The
32 MiB sampler policy ceiling remains in force; small journal resources and the
original document remain additional memory as previously documented. Validation
still examines PCM; these tests do not establish large-sample 68030 UI latency.

A sample metadata change stops an active old audio snapshot, as other sample
edits do. Replay/audition can then start with the updated attributes. Native
validation verifies the attributes and exported files; physical pitch, audio
quality and enhanced playback remain separate acceptance gates.


## Song-title editing

Click the song-name strip or press Control-Shift-T to edit a title of up to 31
characters. Return applies one shared undo command; Escape cancels without losing
redo. The text input and modal target rules match sample-name editing. Empty titles
are allowed. Titles join note, channel and sample changes in chronological undo;
they do not allocate or copy PCM. The fixed-capacity command payload uses a union
for channel, title and external-resource commands.

PTG retains all 31 characters. Strict MOD export refuses titles longer than its
20-character field; it never silently truncates the saved song. The private Paula
replay snapshot limits its unused display title to 20 characters so a long project
title cannot interrupt compatible audio. The live project title remains intact.

## Song positions and new patterns

POS ED. or Control-P opens the position editor. Up/down selects a position;
left/right changes its assigned pattern. ADD POS / A appends a position using
the currently displayed pattern. NEW PAT / N appends one empty 64-row pattern
and a position referencing it, then selects that position. Return, Escape or
BACK returns to the main screen. The main PATTERN arrows still browse patterns
without changing the order list. Pattern indices and position indices are zero
based; the page header shows the final valid index for each.

These operations share Control-Z / Control-Shift-Z history with notes, sample
edits and metadata. Undo refuses unexpected changes to the appended pattern or
surviving references rather than deleting them. The project format permits up
to 256 patterns and positions; strict MOD export retains its separate classic
limits. Growth uses reference-counted arrays with geometric capacity, subject
to an 8 MiB editor song-storage budget plus original document storage. History
eviction releases unused versions. Allocation failures preserve project/history.

Position-list changes stop the running Paula snapshot; press Play to restart
with the new song order. Removing/reordering positions, deleting patterns and
changing an existing song's channel/sample-slot count are still separate work.
