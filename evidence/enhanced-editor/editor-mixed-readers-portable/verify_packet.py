"""Pure offline small compiler packet verifier; no original body, product or target reads."""
from pathlib import Path, PurePosixPath
import hashlib
import json
import stat

BASELINE = 'a02d53f2047753fd8673cbcc377d772da1ceb531'
ROLES = {'current': 'controller-portable-v2', 'failure': 'controller-portable-v1'}
GENERATED = {'README.md', 'original-bindings.json', 'verify_packet.py'}
INVENTORY_COUNT = 21
TOTAL_COUNT = 22
STDOUT_PIN = {'bytes': 2273, 'sha256':
              'b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e'}
PRODUCT_PIN = {'bytes': 510392, 'sha256':
               'f0a35094b0e7c9f806f653155b6bfffcb875da9155a5f955691ba1263beb92a7'}
README_PIN = {'bytes': 2330, 'sha256': 'd52dcab8b523169d0048922238dd18ffd630e25d51344c1eee5f5dc0533caa0b'}
# Complete immutable origin pins and planned public projection identities.
SPECS = {
  "current/qualification.json": {
    "source": "controller-portable-v2/manifest.json",
    "recipe": "qualification",
    "original_pin": {
      "bytes": 2479762,
      "sha256": "959351939d9b5c332cb27df69a86a6a56d4e63f6a801553ee9eacd8cfb1e38ae"
    },
    "public_pin": {
      "bytes": 4481,
      "sha256": "4495aafdc62ad86369dae73e75d6207abdb8f96804e6f971332196efd7dc693c"
    }
  },
  "current/source-inventory.json": {
    "source": "controller-portable-v2/manifest.json",
    "recipe": "source_hash_inventory",
    "original_pin": {
      "bytes": 2479762,
      "sha256": "959351939d9b5c332cb27df69a86a6a56d4e63f6a801553ee9eacd8cfb1e38ae"
    },
    "public_pin": {
      "bytes": 158387,
      "sha256": "e15884c23a31fad07c5381506b9308de712b86a7da9d4c62af748d9c4e956f0e"
    }
  },
  "current/ordered-units.json": {
    "source": "controller-portable-v2/manifest.json",
    "recipe": "ordered_units",
    "original_pin": {
      "bytes": 2479762,
      "sha256": "959351939d9b5c332cb27df69a86a6a56d4e63f6a801553ee9eacd8cfb1e38ae"
    },
    "public_pin": {
      "bytes": 2962,
      "sha256": "6bb8f6ec08cce9fe2bbf55f5f81d0cfb4183a7eb755f0fe02c56199294b3a4b4"
    }
  },
  "current/compiler-calls.json": {
    "source": "controller-portable-v2/manifest.json",
    "recipe": "compiler_call_metadata",
    "original_pin": {
      "bytes": 2479762,
      "sha256": "959351939d9b5c332cb27df69a86a6a56d4e63f6a801553ee9eacd8cfb1e38ae"
    },
    "public_pin": {
      "bytes": 151099,
      "sha256": "6174f9a4211952ceec4d2dea1c9f8e3d9e3de610918b4b012a96092d0596c687"
    }
  },
  "current/tool-runtime-dependency-pins.json": {
    "source": "controller-portable-v2/manifest.json",
    "recipe": "tool_runtime_dependency_pins",
    "original_pin": {
      "bytes": 2479762,
      "sha256": "959351939d9b5c332cb27df69a86a6a56d4e63f6a801553ee9eacd8cfb1e38ae"
    },
    "public_pin": {
      "bytes": 60478,
      "sha256": "2ec67a6f0d79767d8fb4729f894ea3837ea9044aef171a2e0b005999925c692f"
    }
  },
  "current/compile-link.stdout": {
    "source": "controller-portable-v2/native/compile-link.stdout",
    "recipe": "raw",
    "original_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "public_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    }
  },
  "current/compile-link.stderr": {
    "source": "controller-portable-v2/native/compile-link.stderr",
    "recipe": "raw",
    "original_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "public_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    }
  },
  "failure/qualification.json": {
    "source": "controller-portable-v1/manifest.json",
    "recipe": "qualification",
    "original_pin": {
      "bytes": 2065287,
      "sha256": "cd7b1bba3acdb58ba65b05d857023fff59f058a034341b822ca7d23832242fca"
    },
    "public_pin": {
      "bytes": 4324,
      "sha256": "7bb0cc8e6eb4b79c116ea08faa2834b32d8f558b5342b65814d3fdb266383358"
    }
  },
  "failure/source-inventory.json": {
    "source": "controller-portable-v1/manifest.json",
    "recipe": "source_hash_inventory",
    "original_pin": {
      "bytes": 2065287,
      "sha256": "cd7b1bba3acdb58ba65b05d857023fff59f058a034341b822ca7d23832242fca"
    },
    "public_pin": {
      "bytes": 158387,
      "sha256": "926bb4c54be7e509d04b432e889f0bfa6439468a43025cff8b51446d8a516ced"
    }
  },
  "failure/ordered-units.json": {
    "source": "controller-portable-v1/manifest.json",
    "recipe": "ordered_units",
    "original_pin": {
      "bytes": 2065287,
      "sha256": "cd7b1bba3acdb58ba65b05d857023fff59f058a034341b822ca7d23832242fca"
    },
    "public_pin": {
      "bytes": 2962,
      "sha256": "cad545a4cd5e35ac98917dcc3d5c52d76230163c7768b74479621273af9ee10e"
    }
  },
  "failure/compiler-calls.json": {
    "source": "controller-portable-v1/manifest.json",
    "recipe": "compiler_call_metadata",
    "original_pin": {
      "bytes": 2065287,
      "sha256": "cd7b1bba3acdb58ba65b05d857023fff59f058a034341b822ca7d23832242fca"
    },
    "public_pin": {
      "bytes": 151191,
      "sha256": "f515626e9ec2888cd21ac23e95c987cafbf6d3a37b3fb6da13a20a3e1794f9cc"
    }
  },
  "failure/tool-runtime-dependency-pins.json": {
    "source": "controller-portable-v1/manifest.json",
    "recipe": "tool_runtime_dependency_pins",
    "original_pin": {
      "bytes": 2065287,
      "sha256": "cd7b1bba3acdb58ba65b05d857023fff59f058a034341b822ca7d23832242fca"
    },
    "public_pin": {
      "bytes": 60478,
      "sha256": "b067de9a832a929b69748825cb56d1b9f7423ac96309d7e2a9f88827195f9581"
    }
  },
  "failure/compile-link.stdout": {
    "source": "controller-portable-v1/native/compile-link.stdout",
    "recipe": "raw",
    "original_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "public_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    }
  },
  "failure/compile-link.stderr": {
    "source": "controller-portable-v1/native/compile-link.stderr",
    "recipe": "raw",
    "original_pin": {
      "bytes": 3261,
      "sha256": "bc3ba81125fe2df83e350b22c6b6c37e1926b3147241ad67e00dce1eef9386c3"
    },
    "public_pin": {
      "bytes": 3261,
      "sha256": "bc3ba81125fe2df83e350b22c6b6c37e1926b3147241ad67e00dce1eef9386c3"
    }
  },
  "expected-native.stdout": {
    "source": "controller-portable-v2/expected-native.stdout",
    "recipe": "raw",
    "original_pin": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    },
    "public_pin": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    }
  },
  "reviews/current-compiler-review.json": {
    "source": "portable-preparation-v2/review-final-v1/review.json",
    "recipe": "current_independent_review",
    "original_pin": {
      "bytes": 11306,
      "sha256": "d36bb2e96266534e95e0e3d2fd5204ce2aa7754a56290e5a1a710a932b51cfc1"
    },
    "public_pin": {
      "bytes": 9258,
      "sha256": "14e6a41addad63b57ab19e57aee370d0e52d1c887f29332925d854ac7326b645"
    }
  },
  "reviews/current-offline-verification.json": {
    "source": "controller-portable-v2/saved-byte-verification-root-v1.json",
    "recipe": "current_offline_verification",
    "original_pin": {
      "bytes": 3841,
      "sha256": "01b821e2acf1d1b656b09ddbeb7eb823c80b094ea209e3623ffbed7acae3a2f2"
    },
    "public_pin": {
      "bytes": 2703,
      "sha256": "505dd69dd3421e3a2e047bf80f290118d5923566c2c433a797a4e193a4b11d27"
    }
  },
  "reviews/failure-custody-review.json": {
    "source": "portable-preparation-v1/review-failure-v1/review.json",
    "recipe": "failure_independent_review",
    "original_pin": {
      "bytes": 6323,
      "sha256": "5bfdcb928de8e8c65b4eb4bb3be161f24387f51c096215c88298c7eb7637be40"
    },
    "public_pin": {
      "bytes": 5426,
      "sha256": "b1cc07c983f18c34f4fe2213c3964389ac0c104e02dc17b680062dd9d0487ddb"
    }
  }
}

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def pin_bytes(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

def digest(record):
    return {key: record[key] for key in ['bytes', 'sha256']}

def safe_name(name):
    p = PurePosixPath(name)
    require(isinstance(name, str) and bool(p.parts) and not p.is_absolute() and
            '..' not in p.parts and p.as_posix() == name, 'Unsafe relative packet name')

def validate_views(views, bodies):
    for role, folder_name in ROLES.items():
        q = views[role + '/qualification.json']
        inv = views[role + '/source-inventory.json']
        units = views[role + '/ordered-units.json']
        calls = views[role + '/compiler-calls.json']
        pins = views[role + '/tool-runtime-dependency-pins.json']
        expected_status = 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED' if role == 'current' else 'FIRST_FAILURE_RETAINED'
        require(q['status'] == expected_status and q['baseline'] == BASELINE and q['source_count'] == 937 and
                q['unit_count'] == 78 and q['compiler_call_count'] == 91 and q['dependency_count'] == 223 and
                q['source_stable'] is True and q['compiler_groups_quiescent'] is True and
                q['private_retained_members'] == (1164 if role == 'current' else 1147), 'Qualification schema')
        require(all(q[k] == 'NEVER_EXECUTED' for k in ['product_execution', 'native_execution',
                'emulator_execution', 'physical_execution', 'target_execution']), 'Never-executed boundary')
        require(q['product_available'] is (role == 'current') and
                (digest(q['product_pin']) == PRODUCT_PIN if role == 'current' else q['product_pin'] is None),
                'Current product pin/failure absence claim')
        require(q['expected_output_role'] == 'EXPECTED_NATIVE_ONLY_NOT_AN_EXECUTION_RESULT' and
                digest(q['expected_native_stdout']) == STDOUT_PIN, 'Expected output role')
        require(inv['source_count'] == 937 and len(inv['files']) == 937 and inv['before_after_identical'] is True and
                inv['source_bodies_public'] is False, 'Complete source hash inventory')
        require(units['unit_count'] == 78 and len(units['units']) == 78 and
                len(set(units['units'])) == 78, 'Complete ordered units')
        for name in [*inv['files'], *units['units'], *q['overlay_pins']]:
            safe_name(name)
        require(len(q['overlay_pins']) == 9, 'Exact nine overlay pins')
        require(len(pins['tools']) == 5 and len(pins['runtime_inputs']) == 7 and
                len(pins['dependencies']) == pins['dependency_count'] == 223 and
                pins['original_bodies_public'] is False and
                pins['helper_qualification'] == 'DRIVER_LOOKUP_ONLY_NOT_HELPER_INVOCATION', 'Complete byte-pin metadata')
        require(calls['call_count'] == len(calls['calls']) == 91, 'Complete91 call metadata')
        labels = ['compiler-version'] + ['helper-' + n for n in ['cc1', 'as', 'ld', 'collect2']] + [
            'runtime-' + n.replace('.', '-') for n in ['ncrt0.o', 'libnix20.a', 'libnixmain.a',
            'libnix.a', 'libstubs.a', 'libamiga.a', 'libgcc.a']] + [
            'dependency-%02d' % n for n in range(78)] + ['compile-link']
        require([c['label'] for c in calls['calls']] == labels, 'Exact91 ordered labels')
        compiler = pins['tools']['compiler']['path']
        folder = '$PRIVATE/outputs/editor-mixed-readers-prepare/' + folder_name
        flags = ['-std=c99', '-m68000', '-msoft-float', '-mcrt=nix20', '-Os', '-Wall', '-Wextra',
                 '-Werror', '-UNDEBUG', '-Isrc/core', '-I.', '-fbbb=-']
        env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'LANG': 'C', 'TMPDIR': '/private/tmp'}
        require(q['flags'] == flags and q['execution_environment'] == env and
                q['bounds'] == {'overall_seconds': 600, 'each_compiler_call_seconds': 120,
                'each_git_readback_seconds': 20, 'first_failure': 'RETAINED_NO_RETRY'} and
                q['subprocess_policy']['cleanup_reserve_seconds'] == 5, 'Flags/environment/bounds')
        for index, c in enumerate(calls['calls']):
            if index == 0:
                argv = [compiler, '--version']
            elif index < 5:
                argv = [compiler, '-m68000', '-msoft-float', '-mcrt=nix20',
                        '-print-prog-name=' + ['cc1', 'as', 'ld', 'collect2'][index-1]]
            elif index < 12:
                argv = [compiler, '-m68000', '-msoft-float', '-mcrt=nix20',
                        '-print-file-name=' + ['ncrt0.o', 'libnix20.a', 'libnixmain.a',
                        'libnix.a', 'libstubs.a', 'libamiga.a', 'libgcc.a'][index-5]]
            elif index < 90:
                argv = [compiler, *flags, '-M', units['units'][index-12]]
            else:
                argv = [compiler, *flags, *units['units'], '-o', folder + '/native/PTEditorMixedReadersPrepareTest']
            failed = role == 'failure' and index == 90
            require(c['argv'] == argv and c['cwd'] == folder + '/source' and c['execution_environment'] == env and
                    c['returncode'] == (1 if failed else 0) and c['status'] == ('FIRST_FAILURE_RETAINED' if failed else 'PASS') and
                    c['process_session'] == 'OWNED_NEW_SESSION' and c['product_execution'] == 'NEVER_EXECUTED' and
                    type(c['driver_pid']) is int and c['driver_pid'] > 0 and c['driver_pid'] == c['owned_pgid'] and
                    c['driver_reaped'] is True and c['compiler_process_group_quiescent'] is True and
                    c['owned_group_cleanup'] == [] and c['call_budget_seconds'] == 120 and
                    0 < c['timeout_seconds'] <= 115 and 0 <= c['elapsed_seconds'] < 120, 'Exact recorded call metadata')
            for stream in ['stdout', 'stderr']:
                require(c[stream]['path'] == folder + '/native/' + c['label'] + '.' + stream,
                        'Exact redacted original raw-log path')
                if stream == 'stderr' and not failed:
                    require(c[stream]['bytes'] == 0, 'Recorded successful stderr')
        last = calls['calls'][-1]
        require(q['last_compile_returncode'] == last['returncode'] and
                digest(q['last_compile_stdout_original']) == digest(last['stdout']) and
                digest(q['last_compile_stderr_original']) == digest(last['stderr']), 'Last-call provenance')
        for stream in ['stdout', 'stderr']:
            name = role + '/compile-link.' + stream
            require(SPECS[name]['original_pin'] == digest(last[stream]), 'Original final raw-log binding')
        require(bodies[role + '/compile-link.stdout'] == b'' and
                (bodies[role + '/compile-link.stderr'] == b'' if role == 'current' else
                 SPECS[role + '/compile-link.stderr']['original_pin']['bytes'] == 3261), 'Actual final compile logs')
    require(pin_bytes(bodies['expected-native.stdout']) == STDOUT_PIN and
            len(bodies['expected-native.stdout'].splitlines()) == 7, 'Full expected seven lines')
    cur = views['reviews/current-compiler-review.json']
    receipt = views['reviews/current-offline-verification.json']
    old = views['reviews/failure-custody-review.json']
    require(cur['review_status'] == 'BOUNDED_PASS_SAVED_COMPILER_QUALIFICATION' and cur['findings'] == [] and
            cur['qualified_attempt'] == 'controller-portable-v2' and
            digest(cur['actual_manifest']) == SPECS['current/qualification.json']['original_pin'] and
            digest(cur['product']) == PRODUCT_PIN, 'Independent compiler review provenance')
    require(receipt['status'] == 'PASS_INDEPENDENT_OFFLINE_SAVED_BYTES_PORTABLE_NEVER_EXECUTED' and
            receipt['compiler_calls'] == receipt['subprocess_calls'] == 0 and
            receipt['source_count'] == 937 and receipt['unit_count'] == 78 and
            receipt['observed_compiler_calls'] == 91 and receipt['dependency_count'] == 223 and
            digest(receipt['manifest']) == digest(cur['actual_manifest']) and
            digest(receipt['product']) == PRODUCT_PIN and digest(receipt['expected_stdout']) == STDOUT_PIN and
            all(receipt[k] == 'NEVER_EXECUTED' for k in ['product_execution', 'native_execution',
            'emulator_execution', 'physical_execution', 'target_execution']), 'Offline private-verifier provenance')
    require(old['status'] == 'BOUNDED_PASS_FAILURE_CUSTODY_ONLY' and old['findings'] == [] and
            old['compiler_result'] == 'FIRST_FAILURE_RETAINED' and old['native_qualification'] == 'NOT_QUALIFIED_NO_PRODUCT' and
            digest(old['manifest']) == SPECS['failure/qualification.json']['original_pin'], 'Independent failure custody')
    for name, view in views.items():
        require(view['origin']['original_pin'] == SPECS[name]['original_pin'] and
                view['origin']['private_file'] == SPECS[name]['source'] and
                view['projection'] == SPECS[name]['recipe'], 'Projection origin binding')

def read(path):
    require(stat.S_ISREG(path.lstat().st_mode), 'Nonordinary public packet input')
    data = path.read_bytes()
    require(b'/' + b'Users/' not in data, 'Unredacted HOME/private path')
    return data

def main():
    root = Path(__file__).resolve().parent
    files = json.loads(read(root / 'files.json'))
    require(set(files) == set(SPECS) | GENERATED and len(files) == INVENTORY_COUNT, 'Exact small allowlist')
    actual = set()
    for path in root.rglob('*'):
        mode = path.lstat().st_mode
        require(stat.S_ISREG(mode) or stat.S_ISDIR(mode), 'Nonordinary public entry')
        if stat.S_ISREG(mode):
            actual.add(path.relative_to(root).as_posix())
    require(actual == set(files) | {'files.json'} and len(actual) == TOTAL_COUNT, 'Unexpected/missing public file')
    bodies, views = {}, {}
    for name, record in files.items():
        safe_name(name)
        body = read(root / name)
        require(pin_bytes(body) == record, 'Public inventory byte mismatch')
        bodies[name] = body
    rows = json.loads(bodies['original-bindings.json'])
    require(len(rows) == len(SPECS) == 18 and len({row['published'] for row in rows}) == 18 and
            {row['published'] for row in rows} == set(SPECS), 'Exact original binding set')
    for row in rows:
        spec = SPECS[row['published']]
        require(row['original_pin'] == spec['original_pin'] and row['public_pin'] == spec['public_pin'] ==
                files[row['published']] and row['recipe'] == spec['recipe'] and
                row['private_source'] == spec['source'], 'Distinct original/public pin binding')
    for name, spec in SPECS.items():
        require(pin_bytes(bodies[name]) == spec['public_pin'], 'Hardcoded planned projection changed')
        if name.endswith('.json'):
            views[name] = json.loads(bodies[name])
    validate_views(views, bodies)
    require(pin_bytes(bodies['README.md']) == README_PIN, 'README changed')
    print(json.dumps({'status': 'PASS_PUBLIC_PORTABLE_PROJECTIONS_AND_FINAL_LOGS_ONLY',
                      'inventory_entries': INVENTORY_COUNT, 'total_files': TOTAL_COUNT,
                      'current_compiler_result': 'PASS_COMPILER_LINK_ONLY_NEVER_EXECUTED',
                      'failure_result': 'FIRST_FAILURE_RETAINED_RC1_NO_PRODUCT',
                      'private_original_bodies_verified': False, 'product_execution': 'NEVER_EXECUTED'}))

if __name__ == '__main__':
    main()
