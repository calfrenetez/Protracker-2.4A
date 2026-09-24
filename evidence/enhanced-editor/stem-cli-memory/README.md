# Native stem CLI acceptance

Reuses the previously cross-built PT24GRender from commit26132a2; SHA recorded
in result.json matches render-cli-memory/build.json. No product binary changes.
Coordinated shared030 run1790239218097864000: rc0, two one-row24-bit stems,
5760 frames each, exact host WAV comparison. Intentional repeat returns20 and
preserves existing directory/files. Input unchanged; no unexpected files or
orphan stem staging; completion/comparison preceded exact owned cleanup.
Explicit resource release sent to AmiConnect. No audio/card/physical/lifecycle/
configuration operations; physical performance and editor UI remain unqualified.

Host allocation-failure fixture now explicitly checks both WAV temporary files
and stem staging directories. Every10 mix/27 stem allocation failure passes,
zero owned bytes; format parity and memory ceilings also pass.
