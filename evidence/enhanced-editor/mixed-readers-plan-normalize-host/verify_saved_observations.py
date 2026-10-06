"""Verify this small saved projection only. No project or target execution."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import stat

MEMBERS = {'README.md', 'observations.json', 'current.stdout', 'current.stderr',
           'first-failure.stdout', 'first-failure.stderr', 'verify_saved_observations.py'}

def pin(path):
    before = path.lstat()
    if not stat.S_ISREG(before.st_mode):
        raise ValueError('Ordinary packet file required')
    body = path.read_bytes()
    after = path.lstat()
    if (before.st_dev, before.st_ino, before.st_mode, before.st_size, before.st_mtime_ns) != (
            after.st_dev, after.st_ino, after.st_mode, after.st_size, after.st_mtime_ns):
        raise ValueError('Packet changed during read')
    return {'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(), 'mode': before.st_mode}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--observations-sha256', required=True)
    args = parser.parse_args()
    if not re.fullmatch('[0-9a-f]{64}', args.observations_sha256):
        parser.error('Explicit approved SHA256 required')
    folder = Path(__file__).resolve().parent
    if {p.name for p in folder.iterdir()} != MEMBERS:
        raise ValueError('Exact seven-file packet required')
    before = {name: pin(folder / name) for name in MEMBERS}
    if before['observations.json']['sha256'] != args.observations_sha256:
        raise ValueError('Approved observation hash mismatch')
    data = json.loads((folder / 'observations.json').read_text())
    if data['status'] != 'PASS_CORRECTED_V4_HOST_GEOMETRY_ONLY':
        raise ValueError('Unexpected evidence tier')
    if data['counts'] != {'archive_files': 936, 'overlays': 4, 'generated_font': 1,
                          'source_inputs': 941, 'translation_units': 38,
                          'readonly_git_calls': 12, 'compiler_product_calls': 2,
                          'complete_markers': 6}:
        raise ValueError('Count projection mismatch')
    if set(data['bindings']) != MEMBERS - {'observations.json'}:
        raise ValueError('Complete packet bindings required')
    for name, expected in data['bindings'].items():
        if before[name] != expected:
            raise ValueError('Bound packet bytes/full mode changed: ' + name)
    if before['current.stdout']['bytes'] != 899 or before['current.stdout']['sha256'] != '02851866a360bc64be60302a3ff13f8a8d3be457dcb5fd60a27e891c2465be1c':
        raise ValueError('Complete current output mismatch')
    if before['current.stderr']['bytes']:
        raise ValueError('Current stderr is not empty')
    if before['first-failure.stdout']['bytes'] != 446 or before['first-failure.stdout']['sha256'] != '8694bd5db0cc82b2109fd37d9e64969bf208d0155a311173ec0e893821087a18':
        raise ValueError('Original partial output changed')
    if before['first-failure.stderr']['bytes'] != 1286 or before['first-failure.stderr']['sha256'] != '0ded53283b00accce2df5404176a8a6e2b7482745b26445a1f707e1159ce97e3':
        raise ValueError('Original diagnostic changed')
    current = data['current']
    if (current['compile_rc'], current['runtime_rc'], current['reaped_and_group_quiet'],
            current['complete_fixture_pass']) != (0, 0, True, True):
        raise ValueError('Current outcome mismatch')
    failed = data['first_failure']
    if (failed['compile_rc'], failed['runtime_rc'], failed['complete_fixture_pass'],
            failed['reaped_and_group_quiet']) != (0, -6, False, True):
        raise ValueError('Historical failure must remain separate')
    if data['targets'] != {'normalizer_native_build': 'NOT_RUN', 'amiberry': 'NOT_RUN',
                           'real_a1200': 'NOT_RUN', 'hardware_timing_audio_listening': 'NOT_QUALIFIED'}:
        raise ValueError('Target limits changed')
    if data['whole_private_custody_included'] or data['shared_target_authority']:
        raise ValueError('Projection cannot claim private custody or authority')
    if {name: pin(folder / name) for name in MEMBERS} != before:
        raise ValueError('Packet changed before completion')
    print(json.dumps({'status': 'PASS_SAVED_SEVEN_FILE_HOST_PROJECTION_ONLY',
                      'files': 7, 'complete_current_stdout_bytes': 899,
                      'preserved_first_failure_stdout_bytes': 446,
                      'preserved_first_failure_stderr_bytes': 1286,
                      'project_test_replays': 0, 'target_operations': 0,
                      'omitted_private_custody_reverified': False}, sort_keys=True))

if __name__ == '__main__':
    main()
