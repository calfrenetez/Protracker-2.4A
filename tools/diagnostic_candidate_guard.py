"""Host-only exact candidate guard; call before creating or accessing a guest."""
import hashlib
import json
import subprocess
from pathlib import Path

ARTIFACTS = (
    ('build/diagnostic/AmiGUSTest', 'build/diagnostic/build.json'),
    ('build/diagnostic/PTDiagOwnershipTest', 'build/diagnostic/ownership-fixture-build.json'),
    ('build/diagnostic/PTDriverWindowTest', 'build/diagnostic/driver-window-build.json'),
    ('build/diagnostic/PTDriverExecTest', 'build/diagnostic/driver-exec-build.json'),
    ('build/diagnostic/PTPcmReadTest', 'build/diagnostic/pcm-read-build.json'),
    ('build/diagnostic/PTWavetableReadTest', 'build/diagnostic/wavetable-read-build.json'),
    ('build/dev/PTAmiGusNativeAbiTest', 'build/dev/amigus-abi-build.json'),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_diagnostic_candidates(root):
    """Require exact bytes and both working/committed transitive source inputs.

    Build metadata does not establish native execution or hardware acceptance.
    No target operation, lock acquisition, build or repair is performed here.
    """
    root = Path(root)
    lock_path = root / 'amigus-sdk.lock.json'
    lock_hash = digest(lock_path)
    committed_lock = subprocess.check_output(['git', 'show', 'HEAD:amigus-sdk.lock.json'], cwd=root)
    if hashlib.sha256(committed_lock).hexdigest() != lock_hash:
        raise RuntimeError('Diagnostic SDK lock differs from committed candidate')
    for name, expected in json.loads(lock_path.read_text())['files'].items():
        if digest(root / 'vendor/amigus-sdk' / name) != expected:
            raise RuntimeError('Diagnostic SDK header changed: ' + name)
    checked = {}
    for binary_name, manifest_name in ARTIFACTS:
        binary = root / binary_name
        manifest = json.loads((root / manifest_name).read_text())
        actual = digest(binary)
        if (actual != manifest.get('binary_sha256') or
                binary.stat().st_size != manifest.get('binary_bytes')):
            raise RuntimeError('Diagnostic binary differs from manifest: ' + binary_name)
        if not manifest.get('inputs'):
            raise RuntimeError('Diagnostic source inputs missing: ' + manifest_name)
        if manifest.get('sdk_lock_sha256') not in (None, lock_hash):
            raise RuntimeError('Diagnostic SDK manifest changed: ' + manifest_name)
        if binary.name in ('AmiGUSTest', 'PTPcmReadTest', 'PTWavetableReadTest', 'PTAmiGusNativeAbiTest') and manifest.get('sdk_lock_sha256') != lock_hash:
            raise RuntimeError('Diagnostic SDK provenance missing: ' + manifest_name)
        for name, expected in manifest['inputs'].items():
            committed = subprocess.check_output(['git', 'show', 'HEAD:' + name], cwd=root)
            if digest(root / name) != expected or hashlib.sha256(committed).hexdigest() != expected:
                raise RuntimeError('Diagnostic source differs from committed build: ' + name)
        checked[binary.name] = actual
    return checked
