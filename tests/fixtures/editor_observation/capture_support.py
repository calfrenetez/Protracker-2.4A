import os, stat, hashlib, json, dataclasses, enum
from pathlib import Path
FACTS=('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')

def need(ok, why):
    if not ok:
        raise RuntimeError(why)

def read(path, maximum=2097152):
    path = Path(path)
    before = path.lstat()
    need(path.is_absolute() and '..' not in path.parts and stat.S_ISREG(before.st_mode) and (before.st_nlink == 1) and (before.st_size <= maximum) and (not before.st_flags & 1073741824), 'Resident ordinary bounded file required')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        opened = os.fstat(fd)
        body = bytearray()
        while len(body) <= maximum:
            part = os.read(fd, min(65536, maximum + 1 - len(body)))
            if not part:
                break
            body.extend(part)
        after = os.fstat(fd)
    finally:
        os.close(fd)
    facts = lambda s: {k: getattr(s, k, 0) for k in FACTS}
    need(facts(before) == facts(opened) == facts(after) == facts(path.lstat()) and len(body) == after.st_size, 'File changed during bounded read')
    return (bytes(body), {'path': str(path), 'bytes': len(body), 'sha256': hashlib.sha256(body).hexdigest(), 'fullFacts': facts(after)})

def pinned(ref):
    need(type(ref) is dict and set(ref) == {'path', 'bytes', 'sha256', 'fullFacts'}, 'Exact full4 input required')
    body, actual = read(ref['path'])
    need(actual == ref, 'Bound input changed')
    return body

def jsonable(value):
    if dataclasses.is_dataclass(value):
        return jsonable(dataclasses.asdict(value))
    if isinstance(value, enum.Enum):
        return value.value
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, dict):
        return {key: jsonable(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [jsonable(item) for item in value]
    if value is None or type(value) in (str, int, float, bool):
        return value
    raise TypeError('Unexpected metadata value type: ' + type(value).__name__)

def save(path, value):
    body = (json.dumps(jsonable(value), sort_keys=True, separators=(',', ':'), allow_nan=False) + '\n').encode()
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 384)
    with os.fdopen(fd, 'wb') as stream:
        stream.write(body)
        stream.flush()
        os.fsync(stream.fileno())
    return read(path)[1]
