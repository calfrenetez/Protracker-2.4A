# Compatibility & Future-Proofing Checklist

`FINAL_SCOPE_2.4G.md` is authoritative. This file is an operational checklist.

At compatibility checkpoints:
1. Record current known-good AmiGUS core/library/driver versions.
2. Check official AmiGUS releases and assess relevant changes.
3. Never auto-flash FPGA firmware; request explicit approval if a core update is required.
4. Run AmiGUSTest and applicable tracker regressions on approved new combinations.
5. Update the Zorro/Mini compatibility matrix.
6. Update the build/dependency manifest.
7. Run golden MOD/project/sample corpus.
8. Run resource lifecycle/leak tests.
9. Run save-failure/recovery tests.
10. Record CPU/Chip/Fast RAM budgets.
11. Produce a release compatibility report.

Prefer capability detection and graceful degradation over brittle version checks.
