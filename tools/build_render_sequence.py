#!/usr/bin/env python3
"""Build bounded rendering, cache-restore or mixed frame-zero fixtures."""
import argparse,ast,json,subprocess
from pathlib import Path
from build_diagnostic import digest,runtime_inputs,compiler_safety_flags
ROOT=Path(__file__).resolve().parents[1]
FIXTURES = {
    'render-sequence': ('test_render_sequence.py', 'PTExecRenderSequenceTest'),
    'amigus-restore': ('test_amigus_restore.py', 'PTAmiGusRestoreTest'),
    'amigus-voice-plan': ('test_amigus_voice_plan.py', 'PTAmiGusVoicePlan'),
    'mixed-causal': ('test_mixed_readers_causal.py', 'PTMixedCausalTest'),
    'mixed-frame-zero': ('test_mixed_readers_causal.py', 'PTMixedFrameZero'),
    'routed-combined': ('test_mixed_readers_causal.py', 'PTRoutedCombined'),
    'routed-tracker': ('test_mixed_readers_causal.py', 'PTRoutedTracker'),
    'routed-activation': ('test_mixed_readers_causal.py', 'PTRoutedActivation'),
    'routed-activation-timing': ('test_mixed_readers_causal.py', 'PTRoutedActivationTime'),
    'routed-paula-trace': ('test_mixed_readers_causal.py', 'PTRoutedPaulaTrace'),
    'routed-paula-window': ('test_mixed_readers_causal.py', 'PTRoutedPaulaWindow'),
    'routed-paula-alarm': ('test_mixed_readers_causal.py', 'PTRoutedPaulaAlarm'),
    'routed-paula-tracker': ('test_mixed_readers_causal.py', 'PTRoutedPaulaTracker'),
    'routed-mixed-alarm': ('test_mixed_readers_causal.py', 'PTRoutedMixedAlarm'),
    'routed-mixed-wait': ('test_mixed_readers_causal.py', 'PTRoutedMixedWait'),
    'camd-control': ('test_camd_control.py', 'PTCamdControl'),
    'cia-direct-clock': (None, 'PTCIADirectClock'),
    'cia-reload': (None, 'PTCIAReload'),
    'cia-local-activation': (None, 'PTCIALocalActivate'),
    'countdown-window': ('test_countdown_window.py', 'PTCountdownWindow'),
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--fixture', choices=FIXTURES, default='render-sequence')
    args = parser.parse_args()
    test, executable = FIXTURES[args.fixture]
    mixed_alarm = args.fixture in ('routed-mixed-alarm', 'routed-mixed-wait')
    tracker_alarm = mixed_alarm or args.fixture == 'routed-paula-tracker'
    paula_alarm = tracker_alarm or args.fixture == 'routed-paula-alarm'
    if args.fixture == 'cia-direct-clock':
        sources = ['tests/native_cia_direct_clock_test.c', 'tests/native_cia_direct_clock.s']
    elif args.fixture == 'cia-reload':
        sources = ['tests/native_cia_reload_test.c', 'tests/native_cia_reload_irq.s',
                   'tests/native_cia_direct_clock.s', 'src/core/countdown_window.c']
    elif args.fixture == 'cia-local-activation':
        sources = ['tests/native_cia_local_activation_test.c', 'tests/native_cia_local_activation.s',
                   'tests/native_cia_direct_clock.s', 'src/core/countdown_window.c', 'src/core/elapsed_clock.c']
    else:
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
        'routed-paula-trace': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
        'routed-paula-window': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
        'routed-paula-alarm': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
        'routed-paula-tracker': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
        'routed-mixed-alarm': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
        'routed-mixed-wait': ('native_routed_paula_trace_test.c', 'ACTIVATION_UNITS'),
    }
    if args.fixture in routed_sources:
        source, extra = routed_sources[args.fixture]
        sources = [f'tests/{source}', *sources[2:], 'src/core/routed_midi.c', 'src/core/midi.c']
        if extra:
            tree = ast.parse((ROOT / 'tests/test_routed_midi_combined.py').read_text())
            units = [extra]
            if args.fixture == 'routed-activation' or mixed_alarm:
                units.append('CAMD_UNITS')
            for unit in units:
                sources.extend(next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign)
                                    and any(isinstance(target, ast.Name) and target.id == unit for target in node.targets)))
    if args.fixture == 'camd-control':
        sources = ['tests/native_camd_control_test.c', 'src/native/camd_adapter.c',
                   'src/native/camd_vectors.s', 'src/core/elapsed_clock.c', *sources[1:]]
    flags=['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core']
    if args.fixture in ('amigus-voice-plan', 'mixed-causal', 'mixed-frame-zero', 'camd-control', 'countdown-window') or args.fixture in routed_sources:
        flags.extend(['-I.', '-UNDEBUG'])
    if paula_alarm or args.fixture == 'routed-paula-window':
        flags.append('-DPT_PAULA_TIMED_TRACE')
        sources.extend(['tests/native_cia_direct_clock.s', 'tests/native_cia_local_activation.s',
                        'src/core/countdown_window.c'])
    if paula_alarm:
        flags.append('-DPT_PAULA_TIMED_IRQ')
    if tracker_alarm:
        flags.append('-DPT_PAULA_TRACKER_IRQ')
        sources.extend(['src/core/render.c', 'src/core/pitch.c', 'src/core/timeline.c', 'src/core/frame_clock.c',
                        'src/core/flow.c', 'src/core/voice.c', 'src/core/paula_render_voice.c',
                        'src/core/amigus_render_voice.c', 'src/core/amigus_trigger_levels.c',
                        'src/editor/mixed_readers_plan_normalize.c'])
    if mixed_alarm:
        flags.append('-DPT_PAULA_MIXED_IRQ')
    if args.fixture == 'routed-mixed-wait':
        flags.append('-DPT_PAULA_WAIT_IRQ')
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
