# Enhanced editor: owned classic Paula replay

The first Enhanced replay path uses the pinned `PT2.3F_replay_cia.s`. It plays
classic four-channel Paula projects from a separate Chip RAM MOD snapshot.
The selective sample cache omits instrument payloads unused by all stored
patterns; its immutable export and edit-comparison workspaces prefer Fast RAM.
See [sample ownership and memory policy](SAMPLE_MEMORY.md).
Saved mute/solo are applied at the volume output; all other strict classic
representability checks remain in place. It does not discard AmiGUS/MIDI tracks,
downconvert samples or substitute
Paula for other routes. Such projects remain editable and saveable, but this
backend refuses playback. Full mixed 1–16-channel dispatch remains open.

## Controls and display

PLAY / Return / F8 starts at the selected song position. PATTERN / Shift-Return /
F9 loops the selected pattern. STOP / F10 releases sound and timer resources;
Space also stops during playback. EDIT still toggles note entry. SAMPLE auditions
the selected sample at C in the chosen entry octave on a Paula channel, until
Stop. The audition is a temporary single-note project and never edits the song.
Any song edit stops this independent audition. It does not inherit song mute/solo.

The edit cursor stays independent of the playback position. The lower bar shows
the sounding order and row, and the tempo fields report live speed/BPM. The four
wave panes show each active voice's current sample data scaled by its volume.
They are **sample waveform views**, not captured audio or phase-accurate DMA
oscilloscopes. They go flat when stopped or on other channel pages. GUI refresh
uses Intuition tick messages and bounded bitmap regions; replay runs on its CIA
interrupt independently. This does not claim a 50 Hz GUI or physical performance.

Pattern edits, undo and redo publish changed 16-byte MOD rows under short interrupt
exclusion. Playback sees those edits on subsequent reads; already-triggered notes
are not retriggered. An edit requiring an unsupported backend stops playback with
a clear message. Sample data is not republished during pattern edits: effects
such as EFx may modify only the private replay copy. Stop/restart recreates it. Sample imports, edits and sample undo/redo stop the
existing snapshot; restart/audition then constructs a fresh one from current data.

The CHANNEL page applies mute/solo live, including undo and redo. Muted voices
continue their note/effect progression, so unmuting restores current output volume
without retriggering. Multiple solo tracks can sound; mute wins over solo. A
playback-only shallow project copy clears these two flags for snapshot encoding,
then a four-bit output mask applies them. The original project is never changed.
Strict disk MOD export continues to refuse the flags because MOD cannot save them.

## Ownership and adapter changes

`audio.device` allocates all four channels at the lowest precedence so existing
owners are never displaced. After successful allocation the bridge raises
precedence, checks ownership, then submits `ADCMD_LOCK` before touching Paula.
Stop disables replay, removes its CIA vector, stops DMA, restores the LED-filter
state, frees/unlocks the audio allocation, waits for its lock reply, closes the
device and releases memory. Failure unwinds only acquired resources.

This follows the [audio allocation documentation](https://wiki.amigaos.net/wiki/Audio_Device)
and the original [ADCMD_LOCK autodoc](https://d0.se/autodocs/audio.device/ADCMD_LOCK).

The vendor source remains unchanged. `tools/prepare_replay.py` generates a copy
with individually checked anchors and the following limited adaptations:

- Remove the standalone demo and embedded MOD; provide a C ABI and snapshot pointer.
- Refuse CIA setup cleanly when graphics.library cannot be opened, without removing
  a vector that was never acquired.
- Preserve the timer-B vector number on removal; the source unconditionally reset it
  to timer A immediately before `RemICRVector`.
- Initialise the acquired timer to continuous E-clock operation, clearing inherited
  CNT/one-shot modes while retaining unrelated TOD/serial control bits.
- Give empty samples and notes before the first instrument an owned two-byte
  Chip RAM guard, with a one-word DMA length. The original zero length would
  request 65,536 words; no address-zero or past-allocation sample reads are used.
  Empty headers are adjusted only in the private snapshot after sample offsets
  have been resolved.
- Reset persistent voice/effect state on each start, preserve C callee-saved registers,
  and expose tick/current-row data without calling C from the interrupt.
- Replace exactly six final volume-register writes with a register-preserving
  helper. It retains raw effect volume, applies the audible mask, and records the
  effective volume written for the display/tests. Live changes publish the mask
  and current register volumes under short interrupt exclusion. Setup occurs only
  after audio ownership. The helper preserves the original MOVE condition codes,
  including X (address subtraction and ordinary rotation leave X unchanged).

The effect implementation and finetune tables are unchanged, apart from the
assembler's explicit equivalent immediate-opcode spelling and these output hooks.
This is source continuity, not blanket runtime certification of every effect combination.

## Validation

Host tests assert the effect body remains identical after spelling adaptation
and the six narrowly specified output hooks,
reject changed patch anchors, and exercise transport hit regions and playback
rendering under address/undefined-behaviour sanitizers.

`PTPaulaTest` exercises actual native replay periods/volumes, competing audio
allocation refusal, DMA stop, source-project preservation, restart, future-row
edits, sample audition, speed/tempo/volume/cut effects, F00 cleanup,
unsupported-project refusal and complete CIA exhaustion. It also tests muted
starts, live unmute, solo/shared solo, mute precedence, strict export refusal and
unchanged project data. Polling observes the actual volume values supplied by the
assembly output hook, not a second independently masked display calculation.
CIA-A timer-B fallback
and vector release are tested only if that timer is initially free; an existing
OS owner is preserved and the unavailable execution path is recorded as NOT RUN. It holds only timers it can
acquire through the OS and releases them on every tested failure path.

`tools/test_playback_emulator.py` runs that test, then sends actual editor keys to
play a classic MOD, change a note, play the changed pattern, save, reopen and play
the saved change, verify byte-identical resaving, and exit normally. It requires
an explicitly reserved emulator window and guards the exact private profile.
Physical audio quality, complete effects parity, CAMD, AmiGUS and real-device
performance remain separate acceptance gates.

The snapshot is preflighted before hardware allocation, including repeat bounds
for one-word loops. Empty-sample safety tests inspect the native voice pointer and
length to confirm the owned guard is used for both an empty instrument and an
instrument-zero note before any sample has played. The source project is unchanged.

The channel UI integration test (`tools/test_channel_editor_emulator.py`) checks
route changes, the fifth-Paula limit, chronological undo/redo, no-op preservation
of redo, exact saved CHAN bytes/CRC with unrelated data unchanged, byte-identical
reopening, live mute/solo/undo/redo, and saved mute applied after playback restart.
Native register-output observations remain distinct from captured/listened audio
or physical hardware acceptance.

Channel details (dev21): names, groups and dormant MIDI-channel assignments are
removed only from the private classic replay snapshot. They remain in the live
project and saved PTG; strict MOD disk export refuses their loss. Native tests
cover initial and live playback with these properties and preservation of the
original project. Non-default pan remains unsupported by this fixed-placement
Paula backend and is still refused/stopped rather than silently approximated.

Song titles (dev24): the private MOD replay snapshot truncates only its display
name to 20 characters. The project's complete 31-character title remains intact,
and strict disk export continues to refuse titles exceeding MOD's limit. Changing
a title cannot alter audio parameters or interrupt otherwise compatible playback.

Song position edits stop the current immutable replay snapshot and request a
Play restart. This includes order-count and pattern-assignment changes, even
when the sample/pattern byte sizes remain identical. Restart validates the new
song and starts at the editor's selected position. Dynamic order mutation inside
the running assembler engine is not used.
