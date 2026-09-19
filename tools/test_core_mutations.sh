#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/fuzz
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Isrc/core tests/core_mutations.c src/core/channels.c src/core/pcm.c \
  src/core/wav.c -o build/fuzz/core-mutations
build/fuzz/core-mutations
