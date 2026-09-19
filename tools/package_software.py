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
    for name in ['PT24GEdit','PT24GConvert']:
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
LOAD/Control-O opens a MOD/project. Control-Shift-S selects a new save path.
Starting PT24GEdit without arguments creates a blank song.
CLEAR/Control-N creates a new song with 1-16 channels; dirty edits need confirmation.
EDIT OP./Control-E provides mark/copy/paste/clear/transpose and whole-block undo.
DISK OP. > SAVE MOD / Control-Shift-M exports strictly lossless classic MODs.
Mixed AmiGUS/MIDI replay and other unfinished controls remain unavailable.
PT24GConvert inspects projects and performs strict lossless MOD/project conversion.
This is not the finished sixteen-channel tracker or a hardware-qualified release.

Amiga Shell, from the Amiga directory:
  Stack 65536
  PT24GEdit examples/classic.mod my-new-project.ptg
  PT24GConvert inspect my-new-project.ptg

Output paths must be new. Existing files are never replaced. Read
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
