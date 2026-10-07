"""Current-source HOST regressions in a contained temporary source tree.

No archive, Git, repository write, native compiler or target operation. The exact
literal font header is generated only in an owned TemporaryDirectory. Full command
streams/receipts remain in a separate owned attempt directory. No driver imports tests.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
CFLAGS = ["-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
          "-fsanitize=address,undefined", "-Isrc/core", "-I."]
SANITIZER_ENV = {"ASAN_OPTIONS": "halt_on_error=1:abort_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
FACTORY_SOURCES = ['tests/sampler_mixed_readers_test.c', 'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c']
CONTROLLER_SOURCES = ['tests/editor_mixed_readers_prepare_test.c', 'src/editor/editor_mixed_readers_prepare.c', 'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c', 'src/core/mixed_readers_activation.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/view.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c', 'src/core/amigus_reservation.c', 'src/editor/sampler_invert_song.c', 'src/core/render_invert.c', 'src/core/invert_bank.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/amigus_session.c', 'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/studio_consumer.c', 'src/core/studio_pump.c', 'src/core/studio_queue.c', 'src/editor/editor_studio.c', 'src/editor/sampler_song.c', 'src/editor/sampler_studio.c', 'src/core/studio_song.c', 'src/core/studio_plan.c', 'src/core/render.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/studio_mix.c', 'src/core/voice.c', 'src/editor/wavetable_song.c', 'src/editor/wavetable_voices.c', 'src/editor/wavetable_dispatch.c', 'src/core/amigus_render_voice.c', 'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/editor_mixed.c', 'src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/mixed_preflight.c', 'src/editor/paula_voices.c', 'src/editor/paula_dispatch.c', 'src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c', 'src/platform/mod_import.c', 'src/platform/pp20_import.c']
FOCUS_SOURCES = ['tests/editor_mixed_causal_prepare_test.c', 'src/editor/editor_mixed_causal_prepare.c', 'src/editor/sampler_mixed_readers.c', 'src/core/mixed_readers_causal.c', 'src/core/amigus_trigger_levels.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/view.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c', 'src/core/amigus_reservation.c', 'src/editor/sampler_invert_song.c', 'src/core/render_invert.c', 'src/core/invert_bank.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/amigus_session.c', 'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/studio_consumer.c', 'src/core/studio_pump.c', 'src/core/studio_queue.c', 'src/editor/editor_studio.c', 'src/editor/sampler_song.c', 'src/editor/sampler_studio.c', 'src/core/studio_song.c', 'src/core/studio_plan.c', 'src/core/render.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/studio_mix.c', 'src/core/voice.c', 'src/editor/wavetable_song.c', 'src/editor/wavetable_voices.c', 'src/editor/wavetable_dispatch.c', 'src/core/amigus_render_voice.c', 'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/editor_mixed.c', 'src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/mixed_preflight.c', 'src/editor/paula_voices.c', 'src/editor/paula_dispatch.c', 'src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c', 'src/platform/mod_import.c', 'src/platform/pp20_import.c']
FACTORY_ORACLE = 'SAMPLER MIXED FACTORY PASS:24 genuine16-reader paired8/16/24 master lifetimes; real private validation and persistent/TEMP pins; selective signed8 Chip and8/16 card literal endian/channel/padding oracles; exact saves/grid/independent proof orders; constructor/admission/all-phase cancel/source expiry/alias/reentry/fragmented cache-slot2 arena-block0; factory queued effects/unadopted pins/CONTROL+STOP/output guards/budget;6 released caller/current persistent-pin lifetimes;32-reader replacement/pressure and malformed exact-domain drains; SOFTWARE_ONLY\nMIXED READERS PASS:12 genuine16-action paired lifetimes;20 constructor/180 typed admissions;78 callback/effect/proof cases;24 explicit clock/refusal/pending/cancellation groups;6 malformed-reader envelopes;18 endian/loop/padding groups;6 full alias,32-reader capacity/replacement and control/STOP groups; expired controls/consumed close0; exact common grid/master saves; SOFTWARE_ONLY\namigus reservation lifecycle: PASS (fake library, no hardware)\nWAVETABLE UPLOAD JOB PASS: bounded steps, cancellation, unpublished leases, partial words, live ownership loss and hit transfer; injected only\nAMIGUS WAVETABLE OWNER PASS: explicit resource, pinned cache lifetime, failure cleanup, lost ownership refusal; fake library/bus only\n'
CONTROLLER_ORACLE = 'SAMPLER MIXED FACTORY PASS:24 genuine16-reader paired8/16/24 master lifetimes; real private validation and persistent/TEMP pins; selective signed8 Chip and8/16 card literal endian/channel/padding oracles; exact saves/grid/independent proof orders; constructor/admission/all-phase cancel/source expiry/alias/reentry/fragmented cache-slot2 arena-block0; factory queued effects/unadopted pins/CONTROL+STOP/output guards/budget;6 released caller/current persistent-pin lifetimes;32-reader replacement/pressure and malformed exact-domain drains; SOFTWARE_ONLY\nSAMPLER MIXED ACTIVATION PASS:12 genuine16-reader8/16/24 master/Chip8/card8+16 paired lifetimes; copied whole-packet exact-window activation, CONTROL and STOP; caller-pin expiry, literal precision/endian/channel/padding and exact saves; independent quiet/persistent retention, partial effects and one shutdown with later independent source proof;32-reader replacement pressure preserves new keys, fragmented slot2/block0 and six unpublished full-span output guards; SOFTWARE_ONLY\nEDITOR MIXED READERS PREPARE PASS:12 genuine paired editor 8/16/24 master/Chip8/card8+16 precision/channel/endian lifetimes; original-window CONTROL/STOP only from positive ACTIVE keys; actual edit/undo/route/dispose veto until independent C/R and source quiet; construction/action cancellation, revision-only stale and poisoned former tables, complete unpublished/spare output/input guards, authoritative enqueue transfer, callback faults/partial effects, slot serial reuse and exact master saves; SOFTWARE_ONLY\nMIXED READERS PASS:12 genuine16-action paired lifetimes;20 constructor/180 typed admissions;78 callback/effect/proof cases;24 explicit clock/refusal/pending/cancellation groups;6 malformed-reader envelopes;18 endian/loop/padding groups;6 full alias,32-reader capacity/replacement and control/STOP groups; expired controls/consumed close0; exact common grid/master saves; SOFTWARE_ONLY\namigus reservation lifecycle: PASS (fake library, no hardware)\nWAVETABLE UPLOAD JOB PASS: bounded steps, cancellation, unpublished leases, partial words, live ownership loss and hit transfer; injected only\nAMIGUS WAVETABLE OWNER PASS: explicit resource, pinned cache lifetime, failure cleanup, lost ownership refusal; fake library/bus only\n'
FOCUS_ORACLE = 'EDITOR MIXED CAUSAL PREPARE PASS:43 genuine typed pure-TRIGGER pair cases; original bind before factory, raw/refusal/reentry retention, literal windows, 4Paula+12card/16card and 8/16/24 masters,32 pinned readers and independent C/R/source quiet, complete guards/actual transfer/sticky pair scope/exact saves/SOURCE borrow; SOFTWARE_ONLY\n'

CAUSAL_SOURCES = ['tests/mixed_readers_causal_test.c', 'src/core/mixed_readers_causal.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c']
CAUSAL_ORACLE = 'MIXED CAUSAL TWO TRIGGER PASS:26 genuine heap cases;12 two16 mixed4/12 8/16/24-master cache8/16 lifetimes;internally derived all20 prediction;early successor before/after completion;actual original clocks/adoption/tombstone;poisoned disposed predecessor C;32-reader and third admission bounds;CONTROL/STOP/wrong predecessor refusal;raw first partial/zero/malformed/late/reentry and wrong fire order suppression;expected-only R and unknown second proof;once shutdown then read-only quiet;master beforeimages;SOFTWARE_ONLY\n'
PUBLICATION_SOURCES = ['tests/mixed_readers_causal_publication_test.c', 'src/core/mixed_readers_causal.c', 'src/core/mixed_scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_voice_plan.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c']
PUBLICATION_ORACLE = 'MIXED CAUSAL PUBLICATION PASS:56 genuine heap cases;48 first/successor raw0/-1/malformed identity/full-copy/reentry/fault/late publication paths;independent C/R proof orders;faulting raw0 retains pins and unchanged outputs until exact later proofs;6 real allocation/reentry/fail-closed constructors;2 actual consumed-close0 release lifetimes;bound original registration;master beforeimages;SOFTWARE_ONLY\n'
PROTECTED = ['AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h', 'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c', 'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c', 'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py', 'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py']
FONT_RAW_SHA256 = '56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091'
FONT_HEADER_SHA256 = '0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0'
ALL_GROUPS = ('focused-causal43', 'default-factory', 'default-controller',
              'causal26', 'publication56')

GROUPS = {"default-factory": (FACTORY_SOURCES, FACTORY_ORACLE, 180),
          "default-controller": (CONTROLLER_SOURCES, CONTROLLER_ORACLE, 180),
          "focused-causal43": (FOCUS_SOURCES, FOCUS_ORACLE, 30),
          "causal26": (CAUSAL_SOURCES, CAUSAL_ORACLE, 30),
          "publication56": (PUBLICATION_SOURCES, PUBLICATION_ORACLE, 30)}


def pin(path):
    assert path.is_file() and not path.is_symlink()
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'mode': path.stat().st_mode}


def protected_snapshot(root):
    # Known task-local watch paths are optional in a clean checkout. lstat,
    # rather than exists(), refuses even a broken final symlink. Only genuine
    # FileNotFoundError records ABSENT; other errors or nonregular inputs refuse.
    observed = {}
    for relative in PROTECTED:
        path = root / relative
        try:
            mode = path.lstat().st_mode
        except FileNotFoundError:
            observed[relative] = {'state': 'ABSENT'}
        else:
            assert stat.S_ISREG(mode), 'protected watch path must be a regular file'
            observed[relative] = {'state': 'PRESENT', 'pin': pin(path)}
    return observed


def inventory(root, with_font=False):
    entries = [p for directory in ('src', 'tests')
               for p in (root / directory).rglob('*')]
    assert all(not p.is_symlink() for p in entries), 'source symlink refused'
    paths = [p for p in entries if p.is_file()]
    if with_font:
        paths.append(root / 'pt_font.h')
    assert len(paths) <= 2048
    assert all(p.stat().st_size <= 16777216 for p in paths)
    return {str(p.relative_to(root)): pin(p) for p in sorted(paths)}


def run_current(selected, attempt, tree):
    # Full streams/receipt/executables survive temporary source cleanup.
    result = {'scope': 'HOST_ONLY_CURRENT_SOURCE_CONTAINED_REGRESSIONS',
        'status': 'NOT_COMPLETE', 'selected_groups': selected, 'commands': [],
        'groups': [], 'automatic_retry': False, 'native_compiler_invoked': False,
        'target_operation': False, 'source_scope': 'CURRENT_CHECKOUT_BYTES_NOT_AN_ARCHIVE',
        'temporary_source_tree': str(tree), 'temporary_source_cleanup': 'NOT_COMPLETE'}
    # Register all known inputs before any initial read can fail. Unknown initial
    # observations cannot qualify; final independent readback is still attempted.
    required = {
        'repository_after': (lambda: inventory(ROOT), None),
        'protected_after': (lambda: protected_snapshot(ROOT), None),
        'raw_font_after': (lambda: pin(ROOT / 'vendor/pt23f/raw/ptfont.raw'), None),
        'compiler_after': (lambda: pin(Path('/usr/bin/cc')), None)}

    def save():
        (attempt / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')

    def observe(name, callback):
        required[name + '_after'] = (callback, None)
        actual = callback()
        result[name + '_before'] = actual
        required[name + '_after'] = (callback, actual)
        save()
        return actual

    def call(label, argv, seconds, environment):
        entry = {'label': label, 'argv': argv, 'cwd': str(tree),
                 'limit_seconds': seconds, 'passed': False, 'errors': []}
        result['commands'].append(entry)
        save()
        process = None
        started = time.monotonic()

        def probe():
            try:
                os.killpg(process.pid, 0)
                return True
            except ProcessLookupError:
                return False
            except BaseException as error:
                entry['errors'].append({'phase': 'group probe',
                    'error': str(error) or type(error).__name__})
                return None

        try:
            with (attempt / (label + '.stdout')).open('wb') as stdout, \
                    (attempt / (label + '.stderr')).open('wb') as stderr:
                process = subprocess.Popen(argv, cwd=tree, env=environment,
                    stdout=stdout, stderr=stderr, start_new_session=True)
                entry['pid'] = process.pid
                save()
                try:
                    process.wait(timeout=seconds)
                except BaseException as error:
                    entry['errors'].append({'phase': 'bounded wait',
                        'error': str(error) or type(error).__name__})
        except BaseException as error:
            entry['errors'].append({'phase': 'invoke',
                'error': str(error) or type(error).__name__})
        finally:
            if process is not None:
                try:
                    if probe() is not False:
                        entry['errors'].append({'phase': 'finalize',
                            'error': 'Owned HOST group remains or is uncertain'})
                        for sig in (signal.SIGTERM, signal.SIGKILL):
                            if probe() is False:
                                break
                            if probe() is True:
                                try:
                                    os.killpg(process.pid, sig)
                                except ProcessLookupError:
                                    pass
                                except BaseException as error:
                                    entry['errors'].append({'phase': 'owned HOST signal',
                                        'error': str(error) or type(error).__name__})
                            until = time.monotonic() + .5
                            while time.monotonic() < until:
                                process.poll()
                                if probe() is False:
                                    break
                                time.sleep(.02)
                    try:
                        process.wait(timeout=1)
                    except BaseException as error:
                        entry['errors'].append({'phase': 'bounded reap',
                            'error': str(error) or type(error).__name__})
                except BaseException as error:
                    entry['errors'].append({'phase': 'finalization',
                        'error': str(error) or type(error).__name__})
                entry.update(returncode=process.returncode,
                    reaped=process.returncode is not None, group_absent=probe() is False)
            entry['streams'] = {}
            for name in ('stdout', 'stderr'):
                try:
                    entry['streams'][name] = pin(attempt / (label + '.' + name))
                except BaseException as error:
                    entry['errors'].append({'phase': 'capture ' + name,
                        'error': str(error) or type(error).__name__})
            entry['elapsed_seconds'] = time.monotonic() - started
            entry['passed'] = (not entry['errors'] and entry.get('returncode') == 0 and
                entry.get('reaped') is True and entry.get('group_absent') is True and
                entry['elapsed_seconds'] < seconds and
                entry['streams'].get('stderr', {}).get('bytes') == 0)
            save()
        assert entry['passed'], label + ' failed; retain first attempt without retry'
        return (attempt / (label + '.stdout')).read_bytes()

    save()
    try:
        before = observe('repository', lambda: inventory(ROOT))
        observe('protected', lambda: protected_snapshot(ROOT))
        font_pin = observe('raw_font', lambda: pin(ROOT / 'vendor/pt23f/raw/ptfont.raw'))
        observe('compiler', lambda: pin(Path('/usr/bin/cc')))
        font = (ROOT / 'vendor/pt23f/raw/ptfont.raw').read_bytes()
        assert len(font) == font_pin['bytes'] == 580
        assert hashlib.sha256(font).hexdigest() == font_pin['sha256'] == FONT_RAW_SHA256
        assert [len(GROUPS[n][0]) for n in ('focused-causal43','default-factory','default-controller','causal26','publication56')] == [79,29,80,27,27]
        assert all(len(sources) == len(set(sources)) for sources, _, _ in GROUPS.values())
        assert all((ROOT / n).is_file() for label in selected for n in GROUPS[label][0])
        for directory in ('src', 'tests'):
            shutil.copytree(ROOT / directory, tree / directory)
        # Same reviewed static unsigned-char[580] form; no repo font is required.
        header = ('static const unsigned char pt_font[580] = {' +
            ','.join(str(byte) for byte in font) + '};\n').encode('ascii')
        assert len(header) == 1630 and hashlib.sha256(header).hexdigest() == FONT_HEADER_SHA256
        (tree / 'pt_font.h').write_bytes(header)
        expected = dict(before)
        expected['pt_font.h'] = pin(tree / 'pt_font.h')
        required['selected_tree_after'] = (lambda: inventory(tree, with_font=True), expected)
        result['selected_tree_before'] = inventory(tree, with_font=True)
        assert result['selected_tree_before'] == expected
        result['flags'] = CFLAGS
        result['sanitizer_options'] = SANITIZER_ENV
        save()
        environment = dict(os.environ)
        environment.update(SANITIZER_ENV)
        for label in selected:
            sources, oracle, seconds = GROUPS[label]
            group = {'label': label, 'sources': sources, 'oracle': oracle,
                'compile_seconds': 120, 'run_seconds': seconds, 'passed': False}
            result['groups'].append(group)
            executable = attempt / label
            assert call(label + '-compile', ['/usr/bin/cc', *CFLAGS, *sources,
                '-o', str(executable)], 120, environment) == b''
            group['executable'] = pin(executable)
            output = call(label + '-run', [str(executable)], seconds, environment)
            assert output == oracle.encode('ascii'), label + ' complete oracle differs'
            group.update(passed=True, actual_oracle=output.decode('ascii'))
            save()
            sys.stdout.buffer.write(output)
            sys.stdout.flush()
        assert len(result['commands']) == 2 * len(selected)
        assert all(c['passed'] for c in result['commands'])
        result['status'] = 'PASS_SELECTED_CURRENT_SOFTWARE_GROUPS_ASAN_UBSAN'
    except BaseException as error:
        result.update(status='FIRST_FAILURE_RETAINED_NO_RETRY',
            error=str(error) or type(error).__name__)
        raise
    finally:
        result['custody_errors'] = []
        for name, (callback, expected) in required.items():
            try:
                result[name] = callback()
                assert result[name] == expected, name + ' changed'
            except BaseException as error:
                result['custody_errors'].append({'phase': name,
                    'error': str(error) or type(error).__name__})
        if result['custody_errors']:
            result['status'] = 'INPUT_CUSTODY_FAILURE_RETAINED_NO_RETRY'
        save()
        print(attempt / 'receipt.json', flush=True)
        assert not result['custody_errors'], 'input/source custody failed; retain without retry'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--group', action='append', choices=tuple(GROUPS))
    selection.add_argument('--all', action='store_true', help='run all five genuine groups')
    parser.add_argument('--list-groups', action='store_true', help='list group bounds; no source access or tests')
    parser.add_argument('--show-selection', action='store_true', help='print selected labels; no source access or tests')
    arguments = parser.parse_args()
    selected = list(ALL_GROUPS) if arguments.all else arguments.group or ['default-factory', 'default-controller']
    if len(selected) != len(set(selected)):
        parser.error('duplicate group refused')
    if arguments.list_groups:
        if arguments.all or arguments.group or arguments.show_selection:
            parser.error('--list-groups is a standalone selection observation')
        for label in ALL_GROUPS:
            sources, _, seconds = GROUPS[label]
            print(f'{label}: {len(sources)} sources; compile120s run{seconds}s')
        return
    if arguments.show_selection:
        print('\n'.join(selected))
        return
    # The source tree is owned by the standard TemporaryDirectory context.
    # Durable evidence is separate, so context cleanup never erases streams.
    attempt = Path(tempfile.mkdtemp(prefix='pt-causal-current-regression-'))
    temporary_root = None
    controller_error = None
    try:
        with tempfile.TemporaryDirectory(prefix='source-', dir=attempt) as temporary:
            temporary_root = Path(temporary)
            run_current(selected, attempt, temporary_root / 'tree')
    except BaseException as caught:
        controller_error = str(caught) or type(caught).__name__
        raise
    finally:
        receipt_path = attempt / 'receipt.json'
        try:
            result = json.loads(receipt_path.read_bytes())
        except BaseException as caught:
            result = {'scope': 'HOST_ONLY_CURRENT_SOURCE_CONTAINED_REGRESSIONS',
                'status': 'INCOMPLETE_RECEIPT_RETAINED_NO_RETRY',
                'selected_groups': selected, 'commands': [], 'groups': [],
                'controller_errors': [{'phase': 'receipt readback',
                    'error': str(caught) or type(caught).__name__}]}
        removed = temporary_root is not None and not temporary_root.exists()
        result['temporary_source_cleanup'] = 'OWNED_CONTEXT_REMOVED' if removed else 'UNKNOWN_OR_FAILED'
        if controller_error is not None:
            result.setdefault('controller_errors', []).append(
                {'phase': 'temporary source controller', 'error': controller_error})
            if result['status'].startswith('PASS'):
                result['status'] = 'TEMPORARY_SOURCE_CONTROLLER_FAILURE_RETAINED_NO_RETRY'
        if not removed:
            result['status'] = 'TEMPORARY_SOURCE_CLEANUP_FAILURE_RETAINED_NO_RETRY'
        receipt_path.write_text(json.dumps(result, indent=2) + '\n')
        assert removed, 'temporary source cleanup failed; preserve attempt receipt/streams'


if __name__ == '__main__':
    main()
