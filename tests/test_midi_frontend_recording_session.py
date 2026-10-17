"""HOST-only genuine MIDI frontend/recording session; exact complete editor pair.

Pure clean-pair selection is not a clean C compile. Each actual source variant
qualifies separately. No native input/clock/CAMD/frontend or target authority.
"""
from pathlib import Path
import hashlib
import types
import os
import stat
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT
HELPER = CANONICAL / 'tests/test_native_mixed_causal_rolling_entry.py'
HELPER_SHA = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
SOURCES = ('tests/midi_frontend_recording_session_test.c', 'src/native/midi_frontend_recording_session.c', 'src/native/midi_frontend_context.c', 'src/native/midi_requester_model.c', 'src/editor/midi_setup_owner.c', 'src/editor/editor_midi_recording.c', 'src/core/record.c', 'src/core/record_pattern.c', 'src/core/record_channel.c', 'src/editor/midi_setup.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/pattern.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/editor/sampler_lifetime.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/sample_usage.c', 'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c', 'src/core/render.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/voice.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
          '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
HOST_FLAGS = ('-DPT_MIDI_FRONTEND_RECORDING_SESSION_HOST_ONLY=1', '-DPT_MIDI_FRONTEND_CONTEXT_HOST_ONLY=1', '-DPT_MIDI_SETUP_OWNER_HOST_ONLY=1', '-DPT_MIDI_REQUESTER_OWNER_HOST_ONLY=1')
COMMON_FROZEN = {'tests/midi_frontend_recording_session_test.c': '9b15e69d1d1af59e3af5f55f88703bf6acabaf4d4e428f7c3bdf23a8f58e270b', 'src/native/midi_frontend_recording_session.c': '7a88ced28cc666e4778dd11528d7774a635e92cadac858c68071c7f68d33d34f', 'src/native/midi_frontend_context.c': '1d1f21142de9b9364b629bbf1b5715819b7d4f6c70157c8704ad54ba0575d400', 'src/native/midi_requester_model.c': 'b5ef67da67d6c044ea90e21a007fed80ca7e3c2128292ad05ea0fddeca268316', 'src/editor/midi_setup_owner.c': '959d64e0166c3b0ea78f4f821b9754be0eba1afebf65e731022ef4641705af47', 'src/editor/editor_midi_recording.c': '96884997fa546fa72b5db55805f7cdcc7d1ff606a437af7a8aff8ad7597af9df', 'src/core/record.c': '891dca6c8ec9f3159ccd0a6ed08f04cf2900b977d95211d257771b9346ae0890', 'src/core/record_pattern.c': 'b7ff69d4a7c34baa7f33b06734064b5be82f3e3d7bb3792ed2453b0cc6521165', 'src/core/record_channel.c': '54a20cd3b55b663ba7b616b4a232f6d802a6a5eba152e9a9caef3377c3cb5e28', 'src/editor/midi_setup.c': 'ff254db741b4d0c98b289f2e1c2644ccd8084db55a0ec01f0dafdefdf3c5fa38', 'src/editor/song.c': '0e6c75429891da87865e3e2242057401db81129b87a4963dc3730f3f1786559c', 'src/editor/sampler.c': 'd3bba34d4725a6f9cc311fdf3ba2d2233ed84f9ba43d57dd1bc92226446bdc57', 'src/editor/slots.c': 'ac9e10cdf165688b98d4a5562966fb7eee8e8a52376de065e3c61251f57045f8', 'src/core/pcm_filtered.c': '198e436dd49fbc3984cc612554961bd70debad6b8ba4b4b9b4daa8020715a5c9', 'src/core/slices.c': '2951045844f6fdebdd80afe12fe7a5afc1f37bf837c2f49d840f14867835342f', 'src/core/wav.c': '499264a12362a271c20dcc13f4e5e38a470aa5a4ddce90a44c5193f4d8be916f', 'src/core/svx.c': '4712a8d846dd8e5a36fbbdb9b2db7c2fc71ed215be6b114da7d0285cebf69813', 'src/core/raw.c': '460fe21a893b38f14e4af797022f6409504b125f562509b59efef713dc704377', 'src/core/document.c': '64973ff3b3f2b2806ea523c752244689d3275aa9ec66b964bf4db74fea72e2b6', 'src/core/pp20.c': '3f03dac4f224f54a72437370d94e1cd09ce9222a3748a0e805181ae26c5f1465', 'src/core/pattern.c': '0d22aff4b63fae85fa44f063ba6c2eeba37467843e4fcceb493037036366431e', 'src/core/project.c': 'f1bad97dc044e3976936be95c4e42051e8b7a79aab643a1c984a14a1dd24c75a', 'src/core/mod_project.c': '793c9ece248b586dd75e7b3ffefefe446bfbc6d4cc476fc2a61a4a1ce8877b09', 'src/core/mod_inspect.c': '278f601c17ad72a095e6eb17c4f5bbe321bf342edebacf7598ca830f2743b511', 'src/core/channels.c': 'b6316f4a19e737d3f5a19bd04f22f43b0da72188d2a0623404dc4a27bc9c622d', 'src/core/pcm.c': 'ddb208ff5dd445c73d1b573eac1aed3e74bc654fefc2662627d4c0d760f99266', 'src/editor/sampler_lifetime.c': '975e50324af6d280b7d4c82a75a41d56d1bc940412a14ff2ba89a676887700bb', 'src/editor/workflow.c': '768dba18085aeb8e5c815a558ecf233f57652bd5762bbbe866d0ffbce519b6ca', 'src/editor/sample_range.c': '694e14d1a665f2a9b5c844a40e79a27ad1c56eedd5a2c6e379aace8f235980c4', 'src/editor/wave_summary.c': '3e4501324e5888cf6c7b3497c7c6dcb60ef8dcf75e4a53d4d57b1222ae06ff34', 'src/core/sample_usage.c': '14520084e266041ea257bc7e1b5145f9322c5549e4e4377f5b962232f089b160', 'src/core/event_resource.c': '77b4dd85cd2240c8ac0aa9037a5acfcbccc3fa7c70ae5e548c406fe063846970', 'src/core/flow.c': 'c1850d6202a01805876ce6afa903ac4ae145757410e7597d70f2601903e79783', 'src/core/pitch.c': '9088859aa6a2812510b3a28334420030add960afa5f2cca3b41946018f0c04d5', 'src/core/render.c': 'acc53af54275d5ec481ca2128dcf72fde305bf91ed28295fd781a81f42966936', 'src/core/timeline.c': '2296df1f1d13336bacb40cd867268dbd1c52d1ed0353256f5022da536516effd', 'src/core/frame_clock.c': 'e5d9c6c9dc7ea48daf6d099dc5ade8ec2260d8009f40eebd26fa32bd06490a22', 'src/core/voice.c': '9da52c6a82f9ea753655ed4d196946198316cb5327078ffbf354a6dc66e48c68', 'src/native/midi_frontend_recording_session.h': '1bfb585511abc4d5779edad3241254e41cd21f402275f155b094d29972c62fc8', 'src/core/mod_project.h': 'c28407161729fc0fea2398f79341a866f03913b092769674bd871bb23adb59ee', 'src/editor/sampler_lifetime_internal.h': '489b613ba9d848708948bda0a956f79bff249434740a466bc77786f659c7479d', 'src/core/render_storage_internal.h': 'ca1eb7fea1a37cba6ebfedd2e91f6e31a0c6d3f02feee383d184e982fad3953e', 'src/native/midi_frontend_context.h': '1bab68467e7e9069524b03de01c233b5ad36fc449798835e1e88f770aae9de21', 'src/native/midi_requester.h': 'e7da328fde1e40951e747f17a90d4fbe14364f738e6d1cfd2b238f0d0fba08c9', 'src/editor/midi_setup_owner.h': '5482af8449b8bec62f22d83bfaf432056af158cf4f62b573732373171b4ca17d', 'src/editor/sampler_internal.h': '557dc012f010b66ee8dc2ff87b2fe1d906b3d7d9fbb6685134ae87302dd00f1f', 'src/editor/editor_midi_recording.h': '956e889cab40db6ebea2248b02b9b7c15fe90b81a5ea29109aef5c3bd6a118b6', 'src/core/record_pattern.h': '305e23411b91d9c10de377d046adb9661ef94bf2ab4b6eae947eda87b3d34233', 'src/core/record.h': '04576dd8ce3d6437c141d689d6aed88a5754738aac8972f5104abf4b0278369f', 'src/core/record_channel.h': 'fbe1b23f057721ea99774e3230bc2b36dbac376386fe8372f83966d46a4e7c3b', 'src/editor/midi_setup.h': '5ee19232be94a6f64843cd74c181249fadad5482dede743383d69b901111abf8', 'src/editor/workflow.h': '1b8519a6152d636e2ddf2bb63221e03f9ffb03aa462363c0ae7edd5f95d0889e', 'src/editor/song.h': 'fdcc808b738049b0e47ab8b87bcf556b064511c5da17022f84a33e9d5948e1b2', 'src/editor/sampler.h': 'c641ee916de9b292c529bf558ce34ea96d3449d4255510a0122cf67e5822fae9', 'src/core/sample_usage.h': '0ed540356b9ff1bd2ae651de2ee183b6c0c45af84f352eb02fab07d9155565a0', 'src/core/pcm_internal.h': '1abbf89dfc2b640624d3f2cb16c2c715d818deb4a88f5284383d386a35c0188b', 'src/core/wav.h': '8dd0f4454304babb53eef5ecb97cde7bfe6569d292cc0e9a0f204c4a587f2028', 'src/core/svx.h': '824541cd25b72b5820e091cfc9f4e6a5f41351fd0630dddc09aefc0405036814', 'src/editor/sampler_workflow.h': 'a1eff0addfd1fded1185029f68bd9d6d885d5a421b702b40f76cb28af63fac1a', 'src/editor/sampler_master_guard_internal.h': 'f691504b22dd9250a3442953527470e4dd813a4d8eea900412b2afe1d3ab9a7a', 'src/core/pcm.h': '9b6996caf7c81bd53034f53e008e7681885ede2711dddf7c2027c3bab06cb10e', 'src/core/sinc_kernel.h': '058629396a0b22daa67f75c909adf9181fc4e52bfc668890fe2abc3231c3a185', 'src/core/slices.h': '31cd82785c2b86c41d9cde1f7a29056c9d70ec41fb7a9bfb1a5679ba762c24f7', 'src/core/raw.h': 'f9f47added4df41bb7b15aa3b1b7a4d8131a2c4aa19266436318483a41bc0f3b', 'src/core/document.h': '9b6613ca8109c59599635ef1fef3b36a90b30703b3e7e3aa4f2674722e69316a', 'src/core/pp20.h': 'd62562e79452f53a0dc9cdc753e7f337d0e305505408199e805f8a43f862dbf6', 'src/core/document_retirement_internal.h': 'eb033fe27358b69706b2772ff72abc6d9910e654a30b73e3725b697f232c65f1', 'src/core/pattern.h': 'f8c9abde09c4fe495ae77f25c070da647eab038845729edc83adec08c5678d53', 'src/core/project.h': 'e1a749800258b1f1db1dedcd7d5d36970abbf7494279cb3669212757f982e21a', 'src/core/mod_inspect.h': '9b80158f6e9db0ccde95ba16f58a6ed4292ebae056e500796b074842abf8a29f', 'src/core/channels.h': '9eadeee6befb0f291e62c654a8ccd5ed6c6e7030f8c8c93587566fb06eb99517', 'src/editor/sample_range.h': '4ceff1c3611c278ef56b9ead9a3674471d0d18ae1d67c4a032593cd8ee211dd8', 'src/editor/wave_summary.h': '4a3a18cbd568f953c0078be936c43ae468ed264251eb9600bb4297e6df6c8684', 'src/core/event_resource.h': 'f0f446bff489a6107ee2d44b904a395c6ab2e4738d8d4f4cff2ac8db070b4d9b', 'src/core/flow.h': 'db5a78efb5344d95f145d70a6c26c664a832410b8ea741c56f0de831d4163046', 'src/core/pitch.h': '7ec69432c14acb87bc3ba357fe1031023942ebe4ce35e2193d46c16f7068f8b4', 'src/core/pitch_tables.h': '436cb7721c10ef2ef010d7fa6c2b0e46ccec2a4dfc2f7ce9af76a2bb0eb7812a', 'src/core/render.h': 'aa531e26bcf3d3fbe6ddfb61a3ea86ae2c1aeb90016e41c4649076a33793bc2f', 'src/core/render_commands.h': 'ea176e8382ea808929d11571396d6214bb40d2fce8defc9b19569c1bb5c30d14', 'src/core/render_lookahead.h': '6ecdad4402eb721aad8d3b55d6d8fc87a9f8135962c8638f0f1919527c4fbf03', 'src/core/voice_internal.h': '02f7f0af75f1a60a7907fbdcbfa754f3d2a06e9073d2bb5e4888f36a66ce5973', 'src/core/timeline.h': 'ef1640b5c7dd9e48e1e13a9963c21c53b913d062b172a988e91abb7532defa64', 'src/core/frame_clock.h': '77bb574a681a492967a6dc938e5eab7e5f28c70f73fd18189de6ed379ccbc54c', 'src/platform/midi_endpoints.h': '89af1d6d0cd3824e825f3d6b6823bd24d5896db69e8408f004f43dabb5ea5eda', 'src/core/playback.h': 'e1dc3eca74ac19e73c79b8bc5c793dde3ec367fafb5b8632bf176fef98e3b6a2', 'src/core/recent.h': '6324b8256d1348ecd3e85d2142da28fcae2a7f8f1f5ce10fb7a1b812dcb6136b', 'src/core/voice.h': '386b10545fb8ae615b5a45718d90be60b0999b2d63fffd835b6be540d7dca479', 'src/core/midi.h': '0d1cf918cef5a9f53ef2f097e84648be7b753f150e662953db71108025065056'}
WORKING_EDITOR_PAIR = {'src/editor/editor.c': {'bytes': 69953, 'sha256': '6301ed250aa5f0518b360c448a2fb742ab48fdc8e828c1a2d3c69df3ab479ea6'}, 'src/editor/editor.h': {'bytes': 6699, 'sha256': '63fb2ab5b528b327a347811dc446bb95fbabf92a873199b832583380ddcec6c3'}}
HISTORICAL_CLEAN_GIT_EDITOR_PAIR = {'src/editor/editor.c': {'bytes': 69993, 'sha256': 'c569b9bb445b4af2df5bbe44f6d2fd07166c9d9c668272098aaa8ead32f47ec0'}, 'src/editor/editor.h': {'bytes': 6646, 'sha256': '5b93982695779bafe45a1093cd3b3f4c905524dda5a6556a00f410e528f7aa65'}}
EDITOR_INPUTS = ('src/editor/editor.c', 'src/editor/editor.h')
KNOWN_EDITOR_PAIRS = (('COMPLETE_CLEAN_GIT_PAIR', HISTORICAL_CLEAN_GIT_EDITOR_PAIR),
                      ('COMPLETE_CURRENT_WORKING_PAIR', WORKING_EDITOR_PAIR))
ORACLE = 'MIDI SESSION HOST PASS:11 groups;6 complete;5 retained;real modal/takes/reuse/shutdown;master/save/undo;reentry;NO INPUT/CAMD/TIMING/OUTPUT\n'
ORACLE_BYTES = 140


def select_editor_pair(observed):
    """Pure exact two-pair admission, never an arbitrary hash override."""
    assert set(observed) == set(EDITOR_INPUTS), 'complete editor C/H pair required'
    for kind, expected in KNOWN_EDITOR_PAIRS:
        if all(observed[name]['bytes'] == expected[name]['bytes'] and
               observed[name]['sha256'] == expected[name]['sha256']
               for name in EDITOR_INPUTS):
            return kind, expected
    raise AssertionError('unknown or mixed editor C/H pair; compiler admission refused')


class EditorPairAdmission(unittest.TestCase):
    def test_two_complete_known_pairs(self):
        for kind, pair in KNOWN_EDITOR_PAIRS:
            with self.subTest(kind=kind):
                observed = {name: dict(pin) for name,pin in pair.items()}
                self.assertEqual(select_editor_pair(observed), (kind, pair))

    def test_both_mixed_halves_and_unknown_hash_or_size_refuse(self):
        c,h = EDITOR_INPUTS
        bad = [
            {c: dict(WORKING_EDITOR_PAIR[c]), h: dict(HISTORICAL_CLEAN_GIT_EDITOR_PAIR[h])},
            {c: dict(HISTORICAL_CLEAN_GIT_EDITOR_PAIR[c]), h: dict(WORKING_EDITOR_PAIR[h])},
            {name: dict(pin) for name,pin in WORKING_EDITOR_PAIR.items()},
            {name: dict(pin) for name,pin in HISTORICAL_CLEAN_GIT_EDITOR_PAIR.items()},
        ]
        bad[2][c]['sha256'] = '0' * 64
        bad[3][h]['bytes'] += 1
        for index, observed in enumerate(bad):
            with self.subTest(refusal=index):
                before = {name: dict(pin) for name,pin in observed.items()}
                with self.assertRaises(AssertionError):
                    select_editor_pair(observed)
                self.assertEqual(observed, before)


def load_pinned_helper():
    before = HELPER.lstat()
    assert stat.S_ISREG(before.st_mode) and not before.st_flags & 0x40000000
    names = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_size', 'st_uid',
             'st_gid', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')
    facts = lambda value: tuple(getattr(value, name, 0) for name in names)
    descriptor = os.open(HELPER, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        assert facts(os.fstat(descriptor)) == facts(before)
        body = bytearray()
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            body.extend(chunk)
        assert facts(os.fstat(descriptor)) == facts(before)
    finally:
        os.close(descriptor)
    assert facts(HELPER.lstat()) == facts(before)
    assert len(body) == before.st_size and hashlib.sha256(body).hexdigest() == HELPER_SHA
    base = types.ModuleType('pt_midi_endpoint_frozen_helper')
    base.__file__ = str(HELPER)
    exec(compile(bytes(body), str(HELPER), 'exec'), base.__dict__)
    assert base.ROOT == CANONICAL and base.CFLAGS == CFLAGS
    return base


class MidiFrontendRecordingSessionHost(unittest.TestCase):
    def test_genuine_modal_recording_and_shutdown(self):
        base = load_pinned_helper()
        before = base.preservation_snapshot()
        base.evidence('midi-frontend-recording-session-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(HELPER)
        wrapper_before, _ = base.ordinary_pin(Path(__file__))
        old_units = base.SOURCES
        canonical_inputs = {}
        private_inputs = {}
        pins = {}
        try:
            self.assertEqual(len(SOURCES), 39)
            self.assertEqual(len(set(SOURCES)), 39)
            self.assertEqual(len(COMMON_FROZEN), 88)
            self.assertEqual(len(ORACLE.encode('ascii')), ORACLE_BYTES)
            for relative in EDITOR_INPUTS:
                fact, _ = base.ordinary_pin(ROOT / relative)
                canonical_inputs[relative] = fact
            pair_kind, editor_pair = select_editor_pair(canonical_inputs)
            frozen = dict(COMMON_FROZEN)
            frozen.update({name: pin['sha256'] for name,pin in editor_pair.items()})
            self.assertEqual(len(frozen), 90)
            for relative, sha in frozen.items():
                fact, _ = base.ordinary_pin(ROOT / relative)
                self.assertEqual(fact['sha256'], sha)
                private_inputs[relative] = fact
            base.ROOT = ROOT
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-midi-frontend-recording-session-host-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(set(pins), set(frozen))
                for relative, sha in frozen.items():
                    self.assertEqual(pins[relative]['sha256'], sha)
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                command = ['/usr/bin/cc', *CFLAGS, *HOST_FLAGS, *SOURCES, '-o', str(executable)]
                base.evidence('midi-frontend-recording-session-compile-command.json', {'argv': command,
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 120})
                start = time.monotonic()
                compiled = subprocess.run(command, cwd=private, env=environment,
                    capture_output=True, timeout=120)
                compile_seconds = time.monotonic()-start
                base.evidence('midi-frontend-recording-session-compile.stdout', compiled.stdout)
                base.evidence('midi-frontend-recording-session-compile.stderr', compiled.stderr)
                self.assertEqual((compiled.returncode, compiled.stdout, compiled.stderr), (0, b'', b''))
                base.evidence('midi-frontend-recording-session-fixture-command.json', {'argv': [str(executable)],
                    'cwd': str(private), 'env': environment, 'timeoutSeconds': 30})
                start = time.monotonic()
                actual = subprocess.run([str(executable)], cwd=private, env=environment,
                    capture_output=True, timeout=30)
                run_seconds = time.monotonic()-start
                base.evidence('midi-frontend-recording-session-fixture.stdout', actual.stdout)
                base.evidence('midi-frontend-recording-session-fixture.stderr', actual.stderr)
                self.assertEqual((actual.returncode, actual.stdout, actual.stderr),
                                 (0, ORACLE.encode('ascii'), b''))
                base.evidence('midi-frontend-recording-session-inputs.json', {
                    'sources': pins, 'orderedTranslationUnits': list(SOURCES),
                    'literalProjectInputs': 90, 'frozenSelectedInputsSha256': frozen,
                    'originalEditorPairInputs': canonical_inputs,
                    'editorPair': editor_pair, 'editorPairKind': pair_kind,
                    'pairAdmission': 'EXACT_ONE_OF_TWO_COMPLETE_KNOWN_PAIRS',
                    'historicalCleanGitEditorPair': HISTORICAL_CLEAN_GIT_EDITOR_PAIR,
                    'historicalCleanGitQualification': 'ACTUAL_SELECTED_PAIR_RECORDED_ABOVE; OTHER_HISTORICAL_PASSES_NOT_THIS_QUALIFICATION',
                    'originalHelperPin': helper_before, 'wrapperPin': wrapper_before,
                    'compileSeconds': compile_seconds, 'runSeconds': run_seconds,
                    'compileTimeoutSeconds': 120, 'runTimeoutSeconds': 30,
                    'groups': 11, 'complete': 6, 'retained': 5, 'oracleBytes': ORACLE_BYTES,
                    'ownership': 'Real frontend/setup owner/modal, recording take, editor/journal and original master; six complete and five whole rooted fault domains',
                    'scope': 'Actual selected complete editor pair. Genuine exclusive setup modal and stopped metadata take; failed Stop/local cancel and stopped-before-detach veto; positive detach-only original workspace reuse/tails; original24bit master/save/undo; metadata allocation failure; genuine journal-discard/final-release callback BUSY; CLOSED calls after former source/context free; retained child hook, allocator, modal/draft drift and allocation alias. No native/frontend/input/CAMD/clock/output/timing/listening or full compiler-header closure claim'})
                base.verify_sources(pins)
        finally:
            base.ROOT = ROOT
            base.SOURCES = old_units
            base.verify_sources(pins)
            for relative, fact in private_inputs.items():
                self.assertEqual(base.stable_fact(base.ordinary_pin(ROOT / relative)[0]), base.stable_fact(fact))
            base.ROOT = CANONICAL
            for relative, fact in canonical_inputs.items():
                self.assertEqual(base.stable_fact(base.ordinary_pin(ROOT / relative)[0]), base.stable_fact(fact))
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(base.ordinary_pin(HELPER)[0]))
            self.assertEqual(base.stable_fact(wrapper_before), base.stable_fact(base.ordinary_pin(Path(__file__))[0]))
            after = base.preservation_snapshot()
            base.evidence('midi-frontend-recording-session-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
