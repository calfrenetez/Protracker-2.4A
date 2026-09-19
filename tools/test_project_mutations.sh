#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/project-tests
compiler=${CC:-cc}
$compiler -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc/core \
  tests/project_test.c src/core/project.c src/core/channels.c src/core/pcm.c -o build/project-tests/project-fixture
build/project-tests/project-fixture build/project-tests/mixed.ptg
$compiler -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc/core \
  tests/project_mutations.c src/core/document.c src/core/project.c src/core/mod_project.c \
  src/core/mod_inspect.c src/core/channels.c src/core/pcm.c -o build/project-tests/project-mutations
build/project-tests/project-mutations build/project-tests/mixed.ptg
