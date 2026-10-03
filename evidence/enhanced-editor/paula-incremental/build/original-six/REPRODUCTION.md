The compile-commands.json arguments arrays preserve the original ordered compiler,
flags, translation units and output arguments for all six existing candidates.
Commands were reconstructed from the two build scripts and checked against the
manifest, without invoking the compiler. Included C fixtures are dependencies of
the native wrapper and are not added as separate translation units.

Optional reproduction uses the preserved source export and requires a new output
file, leaving the qualified candidate untouched:

    python3 reproduce-native.py --target PTExecPaulaPreflightTest --output-dir /private/tmp/paula-reproduction

The reproducer checks compiler and source-dependency hashes first, then compares
its new binary with the preserved candidate hash. Reproduction is a host build;
it makes no target connection and does not execute the Amiga program.
