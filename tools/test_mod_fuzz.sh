#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/fuzz
${CC:-cc} -std=c99 -g -O1 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -Isrc/core \
  tests/mod_fuzz_driver.c src/core/mod_inspect.c -o build/fuzz/mod-mutations
build/fuzz/mod-mutations
