# Synthetic format hardening corpus, version 1

Generate a fresh corpus from code/:

    python3 -B tests/format_hardening/make_corpus.py /tmp/pt24g-new-corpus

The destination must not exist. No third-party audio is used. The fixed seed is
0x24a019; version, exact generator SHA256, parameters, declared RAW format,
expected acceptance/refusal, decoded PCM oracles and file hashes are emitted in
CORPUS_MANIFEST.json. The TSV is the bounded C driver's input inventory.

Default: 148 cases, under 1 MiB each and 64 MiB total including PCM reference
bytes. The generator enforces these limits; `--max-cases` may lower the cap (a
cap below the recipe count refuses generation rather than silently omitting
cases). A RAW byte string's validity depends on its explicit declaration.

The selected existing project-v1 and MOD fixtures stay byte-for-byte unchanged.
No generated executables or ephemeral runtime paths belong in the source patch.
The delivered report contains the manifest; fixtures are regenerated during the
focused unittest entry point, not checked in as redundant binary payloads.
