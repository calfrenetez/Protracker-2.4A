# Standalone renderer bounded native memory

Host ASan/UBSan render/file/stem tests pass. Supported MOD/enhanced/PP20 input
formats produce identical24-bit WAVs using bounded loaders. Allocation peaks:
MOD/project542838, stems546414, PP20664104 under700000; final zero. All10 mix
and27 stem allocation failure points reject output with no leaked owned bytes
or staging. Existing destinations preserved. Existing native finetune golden
WAVs match. Native code uses production queried Fast-first master allocator and
allocated render/stem workspace APIs. UI and physical acceptance are separate.

Full pinned native build PASS. Coordinated shared030 run1790238715188999000
rc0/PASS:5760-frame24-bit native WAV matches host reference byte-for-byte, source
unchanged. Done and reference checks before exact owned cleanup, explicit release
to AmiConnect. No audio device/card/physical/lifecycle/config operations. This
qualifies the native mix CLI path; native stem CLI runtime remains separate.
