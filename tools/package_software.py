#!/usr/bin/env python3
"""Package tested native software and tracked source; never include local media."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile
from build_diagnostic import ROOT


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    if subprocess.check_output(['git','status','--porcelain'],cwd=ROOT):
        parser.error('commit the reviewed source/evidence before packaging')
    if args.output.exists():parser.error('output already exists; choose a new package name')
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    files={}
    for path in subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).split(b'\0'):
        if path:
            name=path.decode();files['source/'+name]=(ROOT/name).read_bytes()
    core=json.loads((ROOT/'build/dev/core-build.json').read_text())
    for name,expected in core['sources'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:
            parser.error('source changed after the native build: '+name)
    for name in ['PT24GEdit','PT24GConvert','PT24GRender']:
        data=(ROOT/'build/dev'/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=core['binaries'][name]['sha256']:
            parser.error('native binary does not match manifest: '+name)
        files['Amiga/'+name]=data
    for name in ['PT2.4G','PT.HELP','LICENSE-2.3F.txt']:
        files['Amiga/'+name]=(ROOT/'build/dev'/name).read_bytes()
    classic=json.loads((ROOT/'build/dev/build.json').read_text())
    if hashlib.sha256(files['Amiga/PT2.4G']).hexdigest()!=classic['binary_sha256']:
        parser.error('classic tracker does not match its build manifest')
    files['Amiga/examples/mixed.ptg']=(ROOT/'tests/fixtures/project-v1/mixed.ptg').read_bytes()
    files['Amiga/examples/classic.mod']=(ROOT/'evidence/baseline/mod.baseline').read_bytes()
    files['Amiga/core-build.json']=(ROOT/'build/dev/core-build.json').read_bytes()
    files['Amiga/classic-build.json']=(ROOT/'build/dev/build.json').read_bytes()
    files['README.txt']=('''ProTracker 2.4G - hardware-independent development package

PT2.4G is the four-channel assembler derivative (dev2), with loader/input guards.
PT24GEdit is a separate native Enhanced editor prototype with project editing,
paging, undo, verified new-file saves and classic four-channel Paula playback.
F8 plays, F9 loops the pattern and F10 stops. SAMPLE auditions a classic sample.
LOAD/Control-O opens a MOD/project, including structurally validated PP20 MODs. Control-Shift-S selects a new save path.
Sampler +SMP or Control-Shift-Plus adds a sample slot, up to 255; shares undo.
Projects with more than 31 slots require the enhanced project format.
Starting PT24GEdit without arguments creates a blank song.
POS ED./Control-P edits positions; A appends a position, N adds a blank pattern.
Song edits share undo; changed positions stop Paula until Play is restarted.
POS ED. MORE: I inserts, D removes a position, Shift-Up/Down moves it.
Removing a position retains all patterns; the last position cannot be removed.
CLEAR/Control-N creates a new song with 1-16 channels; dirty edits need confirmation.
EDIT OP./Control-E provides mark/copy/paste/clear/transpose and whole-block undo.
EDIT OP. > NOTE / Control-I assigns existing sample slices to normal pattern notes:
S enters a hex ordinal, +/- steps, C clears, U attaches selected sample, [/] selects
that sample slot, L opens its sampler. Slice0000 is whole; 0001 is first marker.
Slice references share note/sample undo and PTG saves; slice playback is pending.
DISK OP. > SAVE MOD / Control-Shift-M exports strictly lossless classic MODs.
DISK OP. > CONVERT MOD or Control-Alt-M opens an explicit 8-bit conversion panel.
It states rounding/no dither/source preservation; EXPORT opens a new MOD path.
Standalone PT24GConvert mod8 INPUT NEW_OUTPUT explicitly rounds otherwise-classic
16/24-bit mono samples to 8-bit (no dither); reports CONVERTED and preserves input.
Standalone mod8tpdf selects deterministic triangular dither for this reduction.
It adds low-level noise; the conversion panel offers the same choice with D
or a click on its DITHER row. Each newly opened panel defaults to no dither.
DISK OP. > RECENT / Control-Shift-R opens the ten most recent successful projects.
Select a row, then OPEN/Return; dirty edits need a second confirmation. REMOVE
and CLEAR LIST change only the list. Preferences persist in ENVARC:ProTracker2.4G.
Control-R or a P/A/M header letter opens CHANNEL: routes, mute and solo share undo
and project saves. P/A/M chooses route, U mute, S solo, Escape closes the page.
DETAILS/D opens P pan (hex00-FF), G group (hex00-0F), M MIDI channel (decimal1-16),
N track name. Return applies, Escape cancels; these settings share undo and PTG saves.
Sample FINETUNE/VOLUME arrows and Alt-up/down or Alt-right/left edit metadata.
Click the sample name or Control-Shift-N to rename; Return applies, Escape cancels.
Click the song title or Control-Shift-T to edit its 31-character PTG title with undo.
Strict MOD export refuses titles longer than 20 characters; Paula replay retains
normal audio while the full title remains in the project.
Metadata shares immutable PCM storage, bounded history and project/MOD saves.
Paula stays fixed-pan; enhanced live pan/MIDI playback remains pending.
Offline track/group stems are available from the render panel.
Classic Paula playback applies mute/solo live; MOD export refuses saved flags and
non-default names/groups/pan/MIDI-channel metadata.
SAMPLER/Control-L opens WAV/IFF/explicit RAW import/export, waveform selection and undoable edits.
In SAMPLER: L load WAV/IFF or browse instruments in a MOD/PP20 source, W export WAV, Shift-W export IFF, R reverse, N normalize, D DC removal, G/H gain
x2 or /2, I/O fades, A all. Two clicks select a frame range. Control-S saves the
project. High-resolution samples retain their precision; Paula audition requires
compatible classic samples. Sample changes stop the old playback snapshot.
Tab cycles SAMPLER/LOOPS/SLICES/RANGE. LOOPS: F forward, P pingpong, O off, B bake
crossfade, U select loop, [/] fade length. SLICES: M add range start, D delete,
C clear, T auto proposal, P apply, X cancel; [/] threshold, up/down gap, Z zero
crossing. Proposals do not change the project until applied. Loop/slice edits
share undo and persist in PTG. LOOPS SAVE IFF / W exports compatible mono8 samples. Pingpong and slice-trigger playback remain pending.
RANGE: I/O zoom, left/right pan, F fit, V zoom selection, S/E exact frame entry.
FORMAT (C from SAMPLER/RANGE): 1/2/3 selects 8/16/24 bits, R enters rate, P applies.
Conversion covers the whole sample, scales loops/markers, and shares undo.
Filtered conversion is default; F toggles linear mode. Filter progress accepts Escape
to cancel without losing the source or redo. Bit reduction currently has no dither.
RAW / X from SAMPLER opens explicit headerless import/export settings: 1/2/3 bits,
M/S mono/stereo, U unsigned8, E byte order, R rate, L load and W save RAW.
RAW export contains PCM only and requires matching sample bits/channels/rate.
Tab returns to SAMPLER; A selects all. Unknown files are never auto-loaded as RAW.
MOD source page: left/right choose source, -/+ choose destination, I/Return imports.
Escape/Tab closes source; imported PCM/name/finetune/volume/loop share undo.
Browsing a MOD never replaces the current song. Shift-L selects a MOD source directly.
Mixed AmiGUS/MIDI replay and other unfinished controls remain unavailable.
PP20 loads through the shared bounded decoder; PX20 and PP20 saving are unsupported.
PT24GConvert inspects projects, performs strict lossless MOD/project conversion,
and offers explicit precision-only mod8 conversion as described above.
PT24GRender INPUT NEW.wav exports a bounded reference song, with optional --pattern N,
--rate 44100|48000, --bits 16|24, --tracks HEX, --gain 0..65536 and --lead-in.
Default: 48kHz/stereo24, all tracks, gain32768, trimmed startup, 30-minute limit.
This uses ideal BPM timing and sample-rate-at-period-428 scaling, not captured hardware.
Supported effects: 0xy..Dxx, E1x..EEx and Fxx. The pinned2.3F unused8xx restores
stored pitch (not panning); E8x is a no-op (no Karplus-Strong processing).
All16 finetunes, E5 overrides and native note quantization are supported. 3xx/5xx
retain sample phase and glide-speed memory; E3 controls glissando. Arpeggio follows
native nibble phases and adjacent-table values without restarting PCM. Vibrato
4xy/6xy and tremolo7xy retain their native memory and waveform controls.
Instrument-only rows can preload silently or reload the same whole sample without
restarting phase. MIDI audio/pitches, active cross-sample/slice handoffs, zero
playback periods, filter/funk and other unsupported effects are refused before output.
9xx tracks require mono8 samples of even length 2..131070, even forward loops or no
loop, and no slice notes. 900 remembers offsets; native double-application and saved
range behavior are retained. Nonloops stop; source PCM is never rewritten.
Staged WAV bytes are verified by a second render; existing paths are never replaced.
Editor: DISK OP. > RENDER WAV or Control-Shift-W. P scope, M track mask, A all, T one,
R rate, B bits, G gain, L lead-in, W/Return WAV file, U new sample, S stems, O track/group mode, E marked rows. Escape cancels rendering.
WAV export preserves project undo/dirty state. NEW SAMPLE / U creates one undoable stereo16/24
sample at the chosen rate, within the sampler memory budget. Cancellation preserves redo.
Selected-row WAV/bounce and track/group stems are implemented; full effect compatibility
and mixed-backend live playback remain unfinished.
This is not the finished sixteen-channel tracker or a hardware-qualified release.

Amiga Shell, from the Amiga directory:
  Stack 65536
  PT24GEdit examples/classic.mod my-new-project.ptg
  PT24GConvert inspect my-new-project.ptg

Song/audio output paths must be new. Existing song/audio files are never replaced. Read
source/docs/ENHANCED_EDITOR.md and source/docs/HARDWARE_INDEPENDENT.md for commands,
current limitations and separate emulator/hardware acceptance boundaries.
The synthetic mixed fixture exercises routes and precision; it is not a demo song.

The source tree and retained test evidence are included. ROMs, AmigaOS media,
private disk images, local emulator configuration and build toolchains are excluded.
Original ProTracker notices are retained in Amiga/LICENSE-2.3F.txt and vendor source.
Git commit: '''+commit+'\n').encode()
    manifest={'git_commit':commit,'physical_amigus_tests':'NOT RUN','files':{
        name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        for name,data in sorted(files.items())}}
    files['MANIFEST.json']=(json.dumps(manifest,indent=2)+'\n').encode()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,'x',zipfile.ZIP_DEFLATED) as archive:
        for name,data in sorted(files.items()):archive.writestr(name,data)
    with zipfile.ZipFile(args.output) as archive:
        assert archive.testzip() is None
        for name,entry in manifest['files'].items():
            assert hashlib.sha256(archive.read(name)).hexdigest()==entry['sha256']
    print(json.dumps({'file':str(args.output),'bytes':args.output.stat().st_size,
        'sha256':hashlib.sha256(args.output.read_bytes()).hexdigest(),'git_commit':commit}))


if __name__=='__main__':main()
