"""Pure three-file public projection verifier; no private custody or target proof."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import stat

BASE = Path(__file__).resolve().parent
MEMBERS = {"README.md", "observations.json", "verify_saved_observations.py"}
EXPECTED = {'schema': 1,
 'packet': 'EDITOR_PAIRED_COMPILER_STACK_OBSERVATIONS',
 'evidence_tier': 'HOST_COMPILER_PER_FUNCTION_METADATA_ONLY',
 'status': 'PASSED_SAVED_METADATA_REVIEW_CANDIDATE_NEVER_EXECUTED',
 'counts': {'source_inputs': 937,
            'translation_units': 78,
            'saved_dependencies': 223,
            'compiler_driver_calls': 90,
            'driver_lookups': 12,
            'unit_compile_calls': 78,
            'read_only_git_readbacks': 8,
            'pre_post_input_checks': 180,
            'raw_stack_usage_tables': 78,
            'frame_row_observations': 1281,
            'static_rows': 234,
            'dynamic_bounded_rows': 1047,
            'unbounded_dynamic_rows': 0,
            'retained_files_without_manifest': 355,
            'files_including_manifest': 356,
            'pinned_tools': 5,
            'pinned_runtime_inputs': 7},
 'recorded_host_times_seconds': {'metadata_attempt_elapsed': 67.73527683300199,
                                 'largest_compiler_call_elapsed': 0.8846877500036499,
                                 'largest_git_readback_elapsed': 0.023907124996185303,
                                 'offline_verifier_elapsed': 2.629556291998597,
                                 'meaning': 'Recorded host process elapsed times; no musical '
                                            'scheduling, IRQ or target execution bound.'},
 'top_five_frame_row_observations': [{'compiled_unit': 'src/core/pcm_filtered.c',
                                      'row': 3,
                                      'function_location': 'pcm_filtered.c:8:20:pt_pcm_resample_filtered_progress.part.0',
                                      'source_location': 'pcm_filtered.c',
                                      'line': 8,
                                      'column': 20,
                                      'function': 'pt_pcm_resample_filtered_progress.part.0',
                                      'frame_bytes': 16572,
                                      'qualifier': 'dynamic,bounded'},
                                     {'compiled_unit': 'src/core/render.c',
                                      'row': 23,
                                      'function_location': 'render.c:438:23:pt_render_stream',
                                      'source_location': 'render.c',
                                      'line': 438,
                                      'column': 23,
                                      'function': 'pt_render_stream',
                                      'frame_bytes': 5536,
                                      'qualifier': 'dynamic,bounded'},
                                     {'compiled_unit': 'src/core/mixed_readers_activation.c',
                                      'row': 22,
                                      'function_location': 'mixed_readers_activation.c:262:33:pt_mixed_activation_fire',
                                      'source_location': 'mixed_readers_activation.c',
                                      'line': 262,
                                      'column': 33,
                                      'function': 'pt_mixed_activation_fire',
                                      'frame_bytes': 4692,
                                      'qualifier': 'dynamic,bounded'},
                                     {'compiled_unit': 'tests/editor_mixed_readers_prepare_test.c',
                                      'row': 66,
                                      'function_location': 'sampler_mixed_readers_test.c:640:13:smf_live_expired',
                                      'source_location': 'sampler_mixed_readers_test.c',
                                      'line': 640,
                                      'column': 13,
                                      'function': 'smf_live_expired',
                                      'frame_bytes': 4384,
                                      'qualifier': 'dynamic,bounded'},
                                     {'compiled_unit': 'src/platform/pp20_import.c',
                                      'row': 4,
                                      'function_location': 'pp20_import.c:39:24:pt_pp20_file_load',
                                      'source_location': 'pp20_import.c',
                                      'line': 39,
                                      'column': 24,
                                      'function': 'pt_pp20_file_load',
                                      'frame_bytes': 4168,
                                      'qualifier': 'dynamic,bounded'}],
 'candidate': {'name': 'PTEditorMixedReadersPrepareTest',
               'bytes': 510392,
               'sha256': 'f0a35094b0e7c9f806f653155b6bfffcb875da9155a5f955691ba1263beb92a7',
               'execution': 'NEVER_EXECUTED'},
 'source_origin': {'baseline': 'a02d53f2047753fd8673cbcc377d772da1ceb531',
                   'overlay_count': 9,
                   'meaning': 'Immutable saved controller source origin; distinct from the '
                              'recorded repository admission HEAD.'},
 'recorded_admission': {'head': '375cda12f49f3b15d75a69b48601443fbb010589',
                        'index_empty_in_saved_before_after_readbacks': True,
                        'live_git_refresh_by_public_verifier': False,
                        'shared_target_authority': False},
 'private_review_checks': {'protected_paths': 16,
                           'current_overlay_paths': 9,
                           'preserved_history_directories': 17,
                           'saved_whole_custody_passed': True,
                           'all_driver_receipts_rc0_empty_stderr_reaped_and_quiet': True,
                           'link_calls': 0,
                           'new_executables': 0,
                           'qualification_replays': 0,
                           'public_packet_contains_complete_private_custody': False},
 'provenance': {'metadata_manifest': {'bytes': 1565636,
                                      'sha256': '22faceb843e0490c73c2344d2ac48edcbd7b36fecaaff2285c72acbfc0dafd3a'},
                'independent_final_review': {'bytes': 14575,
                                             'sha256': 'a8d2ba9e8e1034ed827cb43f9845ee0f13331a0fef27a1332a7637cd9ea4cdb8'},
                'offline_driver_receipt': {'bytes': 1152,
                                           'sha256': '6198e72a0c4c727cb765f444186868aa529f2a1b2dbfd6447db42c9c56796907'},
                'offline_verifier_stdout': {'bytes': 637,
                                            'sha256': '20051fbdc1a18aada3fc8665faeb1b8c5c43449439dc2b2b5f5afe615127de98'}},
 'offline_verifier_observation': {'status': 'PASS_INDEPENDENT_OFFLINE_SAVED_STACK_METADATA_ONLY',
                                  'once_only': True,
                                  'returncode': 0,
                                  'stderr_bytes': 0,
                                  'stdout_bytes': 637,
                                  'stdout_sha256': '20051fbdc1a18aada3fc8665faeb1b8c5c43449439dc2b2b5f5afe615127de98',
                                  'driver_reaped_and_owned_group_quiet': True},
 'preserved_reviewer_checker_refusal': {'classification': 'REVIEWER_RELOCATED_REFERENCE_COMPARISON_ERROR',
                                        'candidate_failure': False,
                                        'first_report': {'bytes': 2787,
                                                         'sha256': '464d07ea88396bfba5480c16da13ff53890593fee508f1bf08ad2affe64977b5'},
                                        'resolution': 'Original reviewer refusal retained '
                                                      'privately; copied references then checked '
                                                      'by exact bytes and full mode. No candidate, '
                                                      'caller or metadata attempt was replayed.'},
 'scope_limits': ['Per-function compiler frame observations only; row count is not a '
                  'unique-function or call-chain count.',
                  'No measured stack high-water or complete call-chain, library or interrupt stack '
                  'bound.',
                  'No target ABI/layout, WCET, musical timing, Paula/AmiGUS placement, device '
                  'completion or voice-stop acceptance.',
                  'No Amiberry, real A1200, native execution, hardware or listening acceptance.',
                  'Public verification checks only these projections and the two bound packet '
                  'files; it does not recheck omitted private sources, dependencies, tools, raw '
                  'tables, logs or whole custody.'],
 'omitted_private_bodies': ['Source pool and SDK/runtime inputs',
                            'Compiler binary and controller candidate binary',
                            'Raw stack tables and compiler/Git logs',
                            'Complete manifests, review custody inventories, private absolute '
                            'paths and process identities',
                            'Private freeze, shared ownership and target authority records']}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def read_regular(path):
    require(path.is_absolute(), "Absolute packet path required")
    for ancestor in (path.parent, *path.parent.parents):
        value = ancestor.lstat()
        require(stat.S_ISDIR(value.st_mode) and
                not (getattr(value, "st_flags", 0) & 0x40000000),
                "Nonordinary or dataless packet ancestor")
    before = path.lstat()
    require(stat.S_ISREG(before.st_mode) and
            not (getattr(before, "st_flags", 0) & 0x40000000),
            "Nonordinary or dataless packet file")
    data = path.read_bytes()
    after = path.lstat()
    identity = lambda value: (value.st_dev, value.st_ino, value.st_mode,
                              value.st_size, value.st_mtime_ns,
                              getattr(value, "st_flags", 0))
    require(identity(before) == identity(after) and len(data) == before.st_size,
            "Packet file changed during read")
    return data, {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                  "mode": stat.S_IMODE(before.st_mode), "full_mode": before.st_mode}


def unique_object(pairs):
    value = {}
    for key, item in pairs:
        require(key not in value, "Duplicate JSON object key")
        value[key] = item
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--observations-sha256", required=True)
    args = parser.parse_args()
    require(re.fullmatch(r"[0-9a-f]{64}", args.observations_sha256) is not None,
            "Explicit approved observations SHA256 required")
    require({path.name for path in BASE.iterdir()} == MEMBERS,
            "The packet must contain exactly the three approved files")
    data, observation_pin = read_regular(BASE / "observations.json")
    require(observation_pin["sha256"] == args.observations_sha256,
            "Observations differ from the separately approved SHA256")
    require(observation_pin["mode"] == 0o644 and observation_pin["full_mode"] ==
            stat.S_IFREG | 0o644, "Observations full mode differs")
    observations = json.loads(data.decode("utf-8"), object_pairs_hook=unique_object)
    require(type(observations) is dict and set(observations) == set(EXPECTED) | {"bindings"},
            "Projection member set differs")
    require(json.dumps({key: observations[key] for key in EXPECTED}, sort_keys=True,
                       separators=(",", ":")) ==
            json.dumps(EXPECTED, sort_keys=True, separators=(",", ":")),
            "Fixed projected observations differ")
    bindings = observations["bindings"]
    require(type(bindings) is dict and set(bindings) ==
            {"README.md", "verify_saved_observations.py"}, "Bound packet member set differs")
    packet_pins = {"observations.json": observation_pin}
    for name in ("README.md", "verify_saved_observations.py"):
        expected = bindings[name]
        require(type(expected) is dict and set(expected) ==
                {"bytes", "sha256", "mode", "full_mode"} and
                type(expected["bytes"]) is int and expected["bytes"] > 0 and
                isinstance(expected["sha256"], str) and
                re.fullmatch(r"[0-9a-f]{64}", expected["sha256"]) is not None and
                type(expected["mode"]) is int and expected["mode"] == 0o644 and
                type(expected["full_mode"]) is int and expected["full_mode"] ==
                stat.S_IFREG | 0o644, "Malformed bound file pin")
        _, actual = read_regular(BASE / name)
        require(actual == expected, "Bound file bytes or full mode differ: " + name)
        packet_pins[name] = actual
    require({path.name for path in BASE.iterdir()} == MEMBERS,
            "Packet member set changed while reading")
    for name, expected in packet_pins.items():
        _, actual = read_regular(BASE / name)
        require(actual == expected, "Packet bytes or full mode changed during verification: " + name)
    print(json.dumps({"status": "PASS_PUBLIC_STACK_OBSERVATIONS_PROJECTIONS_ONLY",
        "files": 3, "observations": observation_pin, "source_inputs": 937,
        "translation_units": 78, "saved_dependencies": 223,
        "compiler_driver_calls": 90, "read_only_git_readbacks": 8,
        "frame_row_observations": 1281, "candidate_execution": "NEVER_EXECUTED",
        "limits": "Small projected semantics and bound README/verifier bytes only; omitted private sources, dependencies, tools, raw tables, logs and whole custody were not rechecked."}, indent=2))


if __name__ == "__main__":
    main()
