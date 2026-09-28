# Current master export and IFF import binaries

Five exact full-build fixtures pass shared030 in75.021s total: WAV/RAW/IFF master
export, IFF import and classic MOD streaming. Source and exact hashes are in
planned.json; this is the f303cefc84460adde81b0dacd08842352e7526ff build previously
verified against all350source/147binary hashes and207 passing host cases.

WAV covers8/16/24-bit mono/stereo; RAW16 formats; IFF export preserves metadata,
loops and odd padding. Each export preserves masters and protects existing
outputs with11240-byte bounded workspace. IFF import covers8 formats, multi-block
exact masters, allocation/fill/limit refusal and undo/redo. MOD covers65patterns,
direct/round8/TPDF exact bytes, legacy headers, loops, source/destination protection
and7164-byte workspace. Fast allocations respectively18/35/11/80/12, all released.

All return0. Independent later locked identity/running/all4DMAoff/exactownedpath
absence was verified after each before continuing; explicit AmiConnect RELEASE.
No input device, MMIO, audio output, native UI or physical acceptance. The physical
Amiga remained off. This extends runtime evidence, not implementation scope.
