#!/usr/bin/env python3
"""Build native core tests and a real-DOS harness for the tracker guard."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from build_diagnostic import digest, runtime_inputs, compiler_safety_flags, ROOT
from make_mod_corpus import cases
from prepare_replay import prepare_replay
from prepare_flow_trace import prepare_flow_trace
from prepare_pitch_trace import prepare_pitch_trace
from prepare_sample_trace import prepare_sample_trace
from prepare_invert_trace import prepare_invert_trace
from prepare_volume_trace import prepare_volume_trace


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default=os.environ.get('AMIGA_CC', 'm68k-amigaos-gcc'))
    args = p.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        p.error('provide AMIGA_CC')
    out = ROOT / 'build/dev'
    out.mkdir(parents=True, exist_ok=True)
    font = (ROOT / 'vendor/pt23f/raw/ptfont.raw').read_bytes()
    (out / 'pt_font.h').write_text('/* Pinned ProTracker 2.3F bitmap font; see vendor/pt23f license. */\nstatic const unsigned char pt_font[580] = {' + ','.join(str(b) for b in font) + '};\n')
    inputs = {
        'PTInvertSequenceTest': ['tests/invert_sequence_test.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/pcm.c'],
        'PTInvertPCMTest': ['tests/invert_pcm_test.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/pcm.c'],
        'PTInvertTest': ['tests/invert_loop_test.c', 'src/core/invert_loop.c'],
        'PT24GEdit': ['src/native/editor_main.c', 'src/native/file_request.c', 'src/native/paula.c', 'src/editor/editor.c','src/editor/song.c', 'src/editor/view.c', 'src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/platform/file_save.c', 'src/core/safe_save.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTPaulaTest': ['tests/native_paula_test.c', 'src/native/paula.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTViewBench': ['tests/native_view_bench.c', 'src/editor/editor.c','src/editor/song.c', 'src/editor/view.c', 'src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTSlotsTest': ['tests/slots_test.c','src/editor/editor.c','src/editor/song.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTSongTest': ['tests/song_test.c','src/editor/editor.c','src/editor/song.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTSourceTest': ['tests/source_test.c','src/editor/editor.c','src/editor/song.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTNoteSliceTest': ['tests/note_slice_test.c','src/editor/editor.c','src/editor/song.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTSamplerTest': ['tests/sampler_test.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTSampleAttributesTest': ['tests/sample_attributes_test.c','src/editor/sampler.c','src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c','src/core/pattern.c','src/core/document.c', 'src/core/pp20.c','src/core/project.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/channels.c','src/core/pcm.c','src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
        'PTPp20Test': ['tests/pp20_test.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c'],
        'PTRawTest': ['tests/raw_test.c','src/core/raw.c','src/core/pcm.c'],
        'PTSvxTest': ['tests/svx_test.c','src/core/svx.c', 'src/core/raw.c','src/core/pcm.c'],
        'PTFilterTest': ['tests/filter_test.c','src/core/pcm_filtered.c','src/core/pcm.c'],
        'PTMidiTest': ['tests/midi_test.c', 'src/core/midi.c'],
        'PTRecordTest': ['tests/record_test.c', 'src/core/record.c', 'src/core/record_pattern.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTPatternTest': ['tests/pattern_test.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTSlicesTest': ['tests/slices_test.c', 'src/core/slices.c', 'src/core/pcm.c'],
        'PTFileSafetyTest': ['tests/native_file_test.c'],
        'PTProjectTest': ['tests/project_test.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTModProjectTest': ['tests/mod_project_test.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTDocumentTest': ['tests/document_test.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/safe_save.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PT24GConvert': ['tools/pt24g_convert.c', 'src/platform/file_save.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/safe_save.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c'],
        'PTEditorClick': ['tests/native_editor_click.c'],
        'PTInputProbe': ['tests/native_input_probe.c'],
        'PTChannelsTest': ['tests/channels_test.c', 'src/core/channels.c'],
        'PTPcmTest': ['tests/pcm_test.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c'],
    }
    inputs['PTModRound8Test'] = ['tests/mod_round8_test.c', *inputs['PTDocumentTest'][1:]]
    inputs['PTViewProbe'] = ['tests/native_view_probe.c', *inputs['PTViewBench'][1:]]
    inputs['PTPaulaTest'] += ['src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/core/pcm_filtered.c']
    inputs['PT24GEdit'] += ['src/core/sample_cache.c', 'src/core/playback_pcm.c']
    inputs['PTPlaybackPCMTest'] = ['tests/playback_pcm_test.c', 'src/core/playback_pcm.c', 'src/core/sample_cache.c', 'src/core/pcm.c']
    inputs['PTPlaybackUploadTest'] = ['tests/playback_upload_test.c', 'src/core/playback_pcm.c', 'src/core/sample_cache.c', 'src/core/pcm.c']
    inputs['PTPreviewPCMTest'] = ['tests/paula_preview_test.c', 'src/core/pcm_filtered.c', 'src/core/pcm.c']
    inputs['PTSampleCacheTest'] = ['tests/sample_cache_test.c', 'src/core/sample_cache.c']
    inputs['PTFlowTraceTest'] = ['tests/native_flow_trace.c', *inputs['PTPaulaTest'][1:]]
    inputs['PTPitchTraceTest'] = inputs['PTFlowTraceTest']
    inputs['PTSampleTraceTest'] = inputs['PTFlowTraceTest']
    inputs['PTInvertTraceTest'] = inputs['PTFlowTraceTest']
    inputs['PTVolumeTraceTest'] = inputs['PTFlowTraceTest']
    inputs['PTPitchTest'] = ['tests/pitch_test.c','src/core/pitch.c','src/core/flow.c', *inputs['PTPaulaTest'][2:]]
    inputs['PTFlowCoreTest'] = ['tests/flow_test.c','src/core/flow.c', *inputs['PTPaulaTest'][2:]]
    inputs['PTStemsTest'] = ['tests/stems_test.c','src/core/stems.c','src/core/channels.c']
    inputs['PTFrameClockTest'] = ['tests/frame_clock_test.c','src/core/frame_clock.c']
    inputs['PTTimelineTest'] = ['tests/timeline_test.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
    inputs['PTVoiceSegmentTest'] = ['tests/voice_segment_test.c','src/core/voice.c','src/core/pcm.c']
    inputs['PTVoiceTest'] = ['tests/voice_test.c','src/core/voice.c','src/core/pcm.c']
    render_sources = ['src/core/render.c','src/core/pitch.c','src/core/timeline.c','src/core/frame_clock.c','src/core/flow.c','src/core/voice.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
    inputs['PTHandoffTest'] = ['tests/render_handoff_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PT24GEdit'] += [s for s in ['src/platform/render_file.c','src/platform/stem_file.c','src/core/stems.c',*render_sources] if s not in inputs['PT24GEdit']]
    inputs['PT24GEdit'] += ['src/editor/bounce.c','src/core/recent.c','src/platform/recent_file.c']
    inputs['PTRecentTest'] = ['tests/recent_test.c','src/core/recent.c']
    inputs['PTRecentFileTest'] = ['tests/recent_file_test.c','src/core/recent.c','src/platform/recent_file.c']
    inputs['PTBounceTest'] = ['tests/bounce_test.c','src/editor/bounce.c',*dict.fromkeys([*inputs['PTSamplerTest'][1:],*render_sources])]
    inputs['PTTremoloRenderTest'] = ['tests/render_tremolo_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PTOffsetRenderTest'] = ['tests/render_offset_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PTPortaRenderTest'] = ['tests/render_porta_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PTPitchRenderTest'] = ['tests/render_pitch_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PTVolumeRenderTest'] = ['tests/render_volume_test.c',*render_sources,'src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c']
    inputs['PTInstrumentRenderTest'] = ['tests/render_instrument_test.c',*render_sources]
    inputs['PTRangeRenderTest'] = ['tests/render_range_test.c',*render_sources]
    inputs['PTRenderTest'] = ['tests/render_test.c',*render_sources]
    inputs['PTStemFileTest'] = ['tests/stem_file_test.c','src/platform/stem_file.c','src/core/stems.c','src/platform/render_file.c',*render_sources]
    inputs['PTRenderFileTest'] = ['tests/render_file_test.c','src/platform/render_file.c','src/core/wav.c',*render_sources]
    inputs['PT24GRender'] = ['tools/pt24g_render.c','src/platform/stem_file.c','src/core/stems.c','src/platform/render_file.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c',*render_sources]
    inputs['PTRenderMemoryTest'] = ['tests/render_memory_failure_test.c',*inputs['PTStemFileTest'][1:]]
    for name, source in [('PTRenderTest','tests/render_alloc_test.c'),
                         ('PTRenderFileTest','tests/render_file_alloc_test.c'),
                         ('PTStemFileTest','tests/stem_file_alloc_test.c')]:
        inputs[name.replace('Test','AllocTest')] = [source, *inputs[name][1:]]
    for target, base, source in [('PTExecBounceTest','PTBounceTest','tests/native_exec_bounce_test.c'),
                                 ('PTExecStemsTest','PTStemFileTest','tests/native_exec_stems_test.c'),
                                 ('PTExecFailureTest','PTRenderMemoryTest','tests/native_exec_failures_test.c')]:
        inputs[target] = [source, *inputs[base][1:]]
    inputs['PT24GEdit'].append('src/platform/file_load.c')
    inputs['PTSamplerStudioTest'] = ['tests/sampler_studio_test.c','src/editor/sampler_studio.c','src/core/studio_mix.c','src/core/voice.c',*inputs['PTSamplerTest'][1:]]
    inputs['PTVoiceAdvanceTest'] = ['tests/voice_advance_test.c','src/core/voice.c','src/core/pcm.c']
    inputs['PTStudioQueueTest'] = ['tests/studio_queue_test.c','src/core/studio_queue.c','src/core/pcm.c']
    inputs['PTStudioSongTest'] = ['tests/studio_song_test.c','src/core/studio_song.c','src/core/studio_plan.c','src/core/studio_mix.c',*render_sources]
    inputs['PTSamplerSongTest'] = ['tests/sampler_song_test.c','src/editor/sampler_song.c',*dict.fromkeys([*inputs['PTSamplerStudioTest'][1:],*inputs['PTStudioSongTest'][1:]])]
    inputs['PTEditorStudioTest'] = ['tests/editor_studio_test.c','src/editor/editor_studio.c',*dict.fromkeys([*inputs['PTSamplerSongTest'][1:],*inputs['PTSongTest'][1:]])]
    inputs['PTExecEditorStudioTest'] = ['tests/native_exec_editor_studio_test.c',*inputs['PTEditorStudioTest'][1:]]
    inputs['PTExecStudioSongTest'] = ['tests/native_exec_studio_song_test.c',*inputs['PTStudioSongTest'][1:]]
    inputs['PTSequenceTest'] = ['tests/render_sequence_test.c','src/core/studio_plan.c','src/core/studio_mix.c',*render_sources]
    inputs['PTStudioPlanTest'] = ['tests/studio_plan_test.c','src/core/studio_plan.c','src/core/studio_mix.c',*render_sources]
    inputs['PTStudioTickTest'] = ['tests/studio_tick_test.c','src/core/studio_tick.c','src/core/studio_mix.c','src/core/frame_clock.c','src/core/voice.c','src/core/pcm.c']
    inputs['PTStudioMixTest'] = ['tests/studio_mix_test.c','src/core/studio_mix.c','src/core/voice.c','src/core/pcm.c']
    inputs['PTExecStudioTest'] = ['tests/native_exec_studio_test.c',*inputs['PTStudioMixTest'][1:]]
    inputs['PTExecSamplerStudioTest'] = ['tests/native_exec_sampler_studio_test.c',*inputs['PTSamplerStudioTest'][1:]]
    inputs['PTExecImportTest'] = ['tests/native_exec_import_test.c','src/platform/file_load.c']
    inputs['PTExecRecentTest'] = ['tests/native_exec_recent_test.c','src/platform/recent_file.c','src/core/recent.c']
    inputs['PTRecentMemoryTest'] = ['tests/recent_memory_test.c','src/platform/recent_file.c','src/core/recent.c']
    inputs['PTFileLoadTest'] = ['tests/file_load_test.c','src/platform/file_load.c']
    inputs['PTExecSaveTest'] = ['tests/native_exec_save_test.c','src/platform/file_save.c','src/core/safe_save.c']
    flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os',
             '-Wall', '-Wextra', '-Werror', '-Isrc/core', '-Ibuild/dev', *compiler_safety_flags(cc)]
    replay_source=out/'replay.s'
    replay_source.write_bytes(prepare_replay((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay.o'), str(replay_source)], check=True)
    trace_source=out/'replay_trace.s'
    trace_source.write_bytes(prepare_flow_trace((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay_trace.o'), str(trace_source)], check=True)
    pitch_source=out/'replay_pitch.s'
    pitch_source.write_bytes(prepare_pitch_trace((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay_pitch.o'), str(pitch_source)], check=True)
    sample_source=out/'replay_sample.s'
    sample_source.write_bytes(prepare_sample_trace((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay_sample.o'), str(sample_source)], check=True)
    invert_source=out/'replay_invert.s'
    invert_source.write_bytes(prepare_invert_trace((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay_invert.o'), str(invert_source)], check=True)
    volume_source=out/'replay_volume.s'
    volume_source.write_bytes(prepare_volume_trace((ROOT/'vendor/pt23f/replayer/PT2.3F_replay_cia.s').read_bytes(),(ROOT/'src/native/replay_abi.s').read_bytes()))
    subprocess.run([str(ROOT/'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu', '-Fhunk', '-o', str(out/'replay_volume.o'), str(volume_source)], check=True)
    for name, sources in inputs.items():
        subprocess.run([cc, *flags, *(['-DRECORD_BYTES=52U'] if name=='PTPitchTraceTest' else ['-DRECORD_BYTES=76U'] if name=='PTVolumeTraceTest' else ['-DRECORD_BYTES=164U'] if name=='PTInvertTraceTest' else ['-DRECORD_BYTES=140U'] if name=='PTSampleTraceTest' else []), *sources, *([str(out/'replay.o')] if name in ('PT24GEdit','PTPaulaTest') else [str(out/'replay_trace.o')] if name=='PTFlowTraceTest' else [str(out/'replay_pitch.o')] if name=='PTPitchTraceTest' else [str(out/'replay_volume.o')] if name=='PTVolumeTraceTest' else [str(out/'replay_invert.o')] if name=='PTInvertTraceTest' else [str(out/'replay_sample.o')] if name=='PTSampleTraceTest' else []), '-o', str(out / name)], cwd=ROOT, check=True)
    corpus = ROOT / 'local/share/guard'
    corpus.mkdir(parents=True, exist_ok=True)
    rows, manifest = [], []
    for index, (name, data, rc, status) in enumerate(cases()):
        filename = f'f{index:02d}.mod'
        (corpus / filename).write_bytes(data)
        expected = -1 if rc == 20 and status != 'unsupported-format' else 0
        rows += [f"\tdc.b '{filename}',0", f'\tds.b {29-len(filename)}', f'\tdc.w {expected}']
        manifest.append({'index': index, 'case': name, 'expected_guard_result': expected,
                         'scope': 'legacy-dispatch-only' if status == 'unsupported-format' else 'classic-preflight'})
    harness = (ROOT / 'tests/native_guard_harness.s').read_bytes()
    harness += ('\nCaseCount EQU ' + str(len(manifest)) + '\nCases\n' + '\n'.join(rows) + '\n').encode()
    harness += (ROOT / 'src/native/mod_guard.s').read_bytes() + b'\nEND\n'
    generated = out / 'PTGuardTest.s'
    generated.write_bytes(harness)
    subprocess.run([str(ROOT / 'local/vasm/vasmm68k_mot'), '-devpac', '-m68000', '-no-fpu',
                    '-Fhunkexe', '-kick1hunks', '-hunkpad=0', '-nosym', '-o', str(out / 'PTGuardTest'),
                    str(generated)], check=True)
    report = {'compiler': subprocess.check_output([cc, '--version'], text=True).splitlines()[0],
              'compiler_sha256': digest(Path(cc)), 'runtime_inputs': runtime_inputs(cc), 'flags': flags,
              'binaries': {name: {'sha256': digest(out / name), 'bytes': (out / name).stat().st_size}
                           for name in [*inputs, 'PTGuardTest']},
              'sources': {name: digest(ROOT / name) for name in
                          sorted(set(sum(inputs.values(), [])) | {'src/core/recent.h', 'src/platform/recent_file.h', 'src/native/file_request.h', 'src/core/playback.h', 'src/core/scope.h', 'src/core/flow.h', 'src/core/frame_clock.h', 'src/core/timeline.h', 'src/core/voice.h', 'src/core/render.h', 'src/core/render_commands.h', 'src/core/studio_plan.h', 'src/core/studio_song.h', 'src/core/studio_queue.h', 'src/editor/sampler_song.h', 'src/editor/editor_studio.h', 'src/core/stems.h', 'src/platform/stem_file.h', 'src/core/pitch.h', 'src/core/pitch_tables.h', 'tools/generate_pitch_tables.py', 'src/platform/render_file.h', 'tools/build_core_tests.py', 'src/native/paula.h', 'src/native/replay_abi.s', 'tools/prepare_replay.py', 'tools/prepare_flow_trace.py', 'tools/prepare_pitch_trace.py', 'tools/prepare_sample_trace.py', 'tools/prepare_invert_trace.py', 'tools/prepare_volume_trace.py', 'vendor/pt23f/replayer/PT2.3F_replay_cia.s', 'src/core/channels.h', 'src/core/pcm.h',
                          'src/core/sinc_kernel.h', 'tools/generate_sinc_kernel.py', 'src/core/wav.h', 'src/core/svx.h', 'src/core/raw.h', 'src/core/midi.h', 'src/core/record.h', 'src/core/record_pattern.h', 'src/editor/editor.h', 'src/editor/song.h', 'src/editor/sampler.h', 'src/editor/bounce.h', 'src/editor/view.h', 'src/platform/file_save.h', 'src/platform/file_load.h', 'vendor/pt23f/raw/ptfont.raw', 'src/core/project.h', 'src/core/mod_project.h', 'src/core/document.h', 'src/core/pp20.h', 'src/core/safe_save.h', 'src/core/pattern.h', 'src/core/slices.h', 'src/core/mod_inspect.h', 'src/native/present.h', 'src/native/master_memory.h', 'src/core/paula_cache.h', 'src/core/sample_cache.h', 'src/core/playback_pcm.h', 'src/core/paula_preview.h', 'src/native/mod_guard.s', 'tests/native_guard_harness.s'})},
              'guard_cases': manifest}
    (out / 'core-build.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['binaries'], indent=2))


if __name__ == '__main__':
    main()
