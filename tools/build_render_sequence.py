#!/usr/bin/env python3
"""Build bounded rendering, cache-restore or mixed frame-zero fixtures."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
ROOT=Path(__file__).resolve().parents[1]
FIXTURES = {
    'render-sequence': ('test_render_sequence.py', 'PTExecRenderSequenceTest'),
    'amigus-restore': ('test_amigus_restore.py', 'PTAmiGusRestoreTest'),
    'mixed-causal': ('test_mixed_readers_causal.py', 'PTMixedCausalTest'),
    'mixed-frame-zero': ('test_mixed_readers_causal.py', 'PTMixedFrameZero'),
    'routed-combined': ('test_mixed_readers_causal.py', 'PTRoutedCombined'),
    'routed-tracker': ('test_mixed_readers_causal.py', 'PTRoutedTracker'),
    'routed-activation': ('test_mixed_readers_causal.py', 'PTRoutedActivation'),
    'routed-activation-timing': ('test_mixed_readers_causal.py', 'PTRoutedActivationTime'),
    'camd-control': ('test_camd_control.py', 'PTCamdControl'),
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--fixture', choices=FIXTURES, default='render-sequence')
    args = parser.parse_args()
    test, executable = FIXTURES[args.fixture]
    tree=ast.parse((ROOT/'tests'/test).read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    if args.fixture == 'render-sequence':
        sources[0] = 'tests/native_exec_render_sequence_test.c'
    if args.fixture == 'mixed-frame-zero':
        sources[0] = 'tests/native_mixed_frame_zero_test.c'
    routed_sources = {
        'routed-combined': ('routed_midi_combined_test.c', None),
        'routed-tracker': ('routed_tracker_playback_test.c', 'TRACKER_UNITS'),
        'routed-activation': ('routed_activation_test.c', 'ACTIVATION_UNITS'),
        'routed-activation-timing': ('native_routed_activation_timing_test.c', 'ACTIVATION_UNITS'),
    }
    if args.fixture in routed_sources:
        source, extra = routed_sources[args.fixture]
        sources = [f'tests/{source}', *sources[2:], 'src/core/routed_midi.c', 'src/core/midi.c']
        if extra:
            tree = ast.parse((ROOT / 'tests/test_routed_midi_combined.py').read_text())
            sources.extend(next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign)
                                and any(isinstance(target, ast.Name) and target.id == extra for target in node.targets)))
    if args.fixture == 'camd-control':
        sources = ['tests/native_camd_control_test.c', 'src/native/camd_adapter.c',
                   'src/native/camd_vectors.s', 'src/core/elapsed_clock.c', *sources[1:]]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core']
    if args.fixture in ('mixed-causal', 'mixed-frame-zero', 'camd-control') or args.fixture in routed_sources:
        flags.extend(['-I.', '-UNDEBUG'])
    flags.extend(compiler_safety_flags(args.cc))
    out=ROOT/'build/dev';out.mkdir(parents=True,exist_ok=True);binary=out/executable
    subprocess.run([args.cc,*flags,*sources,'-o',str(binary)],cwd=ROOT,check=True)
    syntax=['src/editor/wavetable_dispatch.c'] if args.fixture=='amigus-restore' else []
    for source in syntax:subprocess.run([args.cc,*flags,'-fsyntax-only',source],cwd=ROOT,check=True)
    deps=set()
    for source in sources+syntax:
        if source.endswith('.s'):
            deps.add(source)
            continue
        raw=subprocess.check_output([args.cc,*flags,'-MM',source],cwd=ROOT,text=True).replace('\\\n',' ')
        deps.update(str((ROOT/p).resolve().relative_to(ROOT)) for p in raw.split(':',1)[1].split())
    report=dict(syntax_only=syntax,flags=flags,inputs={p:digest(ROOT/p) for p in sorted(deps)},compiler_sha256=digest(Path(args.cc)),runtime_inputs=runtime_inputs(args.cc),binary_sha256=digest(binary),binary_bytes=binary.stat().st_size)
    (out/(args.fixture+'-build.json')).write_text(json.dumps(report,indent=2)+'\n');print(binary)
if __name__=='__main__':main()
