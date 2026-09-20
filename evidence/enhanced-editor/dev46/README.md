# dev46: tremolo reference rendering

7xy/E7x supported in shared WAV/sample-bounce rendering. Stored volume remains
separate from modulated output. Nibble memory, wave controls, reset suppression,
pattern delay and native ramp dependency on vibrato phase are preserved. Fresh7
also follows PerNop pitch restoration. Accepted main-screen layout is unchanged.

57 host checks passed in96.417 seconds. Eight retained dev45 fixtures yield eleven
per-channel PCM comparisons (184320 stereo24 frames), including all four waveform
selectors. The independent oracle reads native output volume, hardware pitch and
fresh-note DMA flags. It extends the immutable source ramp into a full2048-frame
reference loop at48kHz so late effect ticks remain audible. This is reference PCM
policy, not a claim of analogue Paula waveform equivalence. Build hashes verified.

No physical tests performed.

Native run tremolopcm1789936129546357000: all eleven comparisons passed in
263.328 seconds and matched host output exactly. Guarded QUIT and
fresh process/HDF/socket checks at20:33:46 UTC verified release and exact original
launcher restoration. AmiConnect received explicit release for its next window.
