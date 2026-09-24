# Converter bounded memory

Host ASan/UBSan suites pass: exact classic/project roundtrip, corrupt-input
refusal, existing-destination protection, precision-only rounded8 and fixed TPDF
conversion with unchanged source. Maximum-length classic sample conversion to
both MOD and enhanced project succeeds under a700000-byte allocator ceiling:
peak664104 bytes, final owned bytes0. Earlier retained input plus complete output
would exceed that limit. Current output uses the fixed streaming workspace.

Native PT24GConvert cross-compiles using the pinned m68000/nix20 compiler with
warnings as errors and production Fast-preferred master allocator. Runtime NOT
RUN: shared030 availability remains unverified after the previous identity
refusal. No guest or physical operations were attempted this milestone. No
UI, storage performance or physical acceptance is claimed.
