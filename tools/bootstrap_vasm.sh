#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
DEST="$ROOT/local/vasm"
PIN=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["assembler_source_commit"])' "$ROOT/baseline.lock.json")
URL=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["assembler_source_url"])' "$ROOT/baseline.lock.json")
if [ ! -e "$DEST" ]; then
    mkdir -p "$ROOT/local"
    git clone "$URL" "$DEST"
    git -C "$DEST" checkout --detach "$PIN"
fi
if [ "$(git -C "$DEST" rev-parse HEAD)" != "$PIN" ]; then
    echo 'Existing local/vasm has a different revision; preserve it and resolve the path explicitly.' >&2
    exit 1
fi
if [ -n "$(git -C "$DEST" status --porcelain --untracked-files=no)" ]; then
    echo 'Pinned assembler source has local changes; refusing an unrecorded toolchain.' >&2
    exit 1
fi
make -C "$DEST" -j2 CPU=m68k SYNTAX=mot
printf '\nAssembler ready: %s/vasmm68k_mot\n' "$DEST"
