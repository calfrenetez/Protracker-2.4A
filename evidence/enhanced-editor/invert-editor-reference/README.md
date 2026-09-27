# Editor-owned EFx queue against captured reference audio

The two `invert-four-clock-delay` fixtures now run through the actual editor
ownership adapter, sampler binding, private producer, pump and two-block queue.
Expected PCM comes from the pinned replayer's recorded writes and registers,
not another production renderer. The existing independent oracle is shared in
`tests/invert_reference_oracle.h`; its comparison logic is unchanged.

Each case checks:
- Complete queued playback with 1/17/256-frame host partitions; native 17/256.
- A deliberately held consumer lease, full queue and retained pending producer
  block. Repeated blocked steps preserve pending PCM and the leased block.
- Natural completion drains all frames without calling output-stop prematurely.
- Mid-delayed-row pattern and sample edits close producer ownership before the
  output-stop callback and before any master/event change. The callback compares
  the entire original serialized project at that boundary.
- The held output block survives editing. Queue close refuses while leased;
  releasing the lease permits cleanup and no waiting output survives abort.
- Undo restores the exact serialized project, and a fresh full playback again
  matches every reference frame, demonstrating reset of private mutation state.
- A private-bank budget refusal preserves the serialized project. Enhanced
  serialization reloads and re-encodes identically. All tracked allocations end.

The fixture runs against a Git export of committed production sources so local
uncommitted display changes are preserved and excluded. The native manifest
records source commit, 92 dependencies, compiler/runtime identity and binary hash.
All 88 production dependencies are checked against the exported commit.
This is the editor ownership/controller API, not a native UI PLAY launch or a
real device consumer. The output-stop callback verifies ordering only; no MMIO,
interrupt installation, hardware cancellation or physical performance is claimed.

Reproduce host fixture:
`python3 -m unittest discover -s tests -p test_editor_invert_reference.py`.
Build native fixture:
`python3 tools/build_editor_invert_reference.py --cc <pinned compiler>`.
After fresh coordination, run each separately through the shared harness:
`tools/shared_infra_invert_handoff.py --delay --editor --case inv_delay_four`
and the equivalent `inv_delay_disable` case. Each reserves 240 seconds and uses
shared lock/live guards plus completion/DMA-off/exact cleanup checks.

No production source change or device output enabling was needed. Private EFx
still refuses 16/24-bit master sources; ordinary Studio retains full-precision
master mixing. Physical Paula/A1200/AmiGUS acceptance remains separate.

Validation on 27 September 2026:
- All 26 related host sanitizer tests passed in 76.014 seconds.
- Native `inv_delay_four` passed RC0 in 93.62 seconds;
  `inv_delay_disable` passed RC0 in 78.76 seconds. Each had a separate freshly
  coordinated 240-second window, with no timeout, retry or reset.
- Both report 77 Fast allocations, zero final owned bytes and budget refusal
  without Chip fallback. All four DMA channels were off before exact owned
  cleanup; both windows were explicitly released to AmiConnect and Scott.
- The 203768-byte native binary has SHA-256
  `b54554fb0cbfa759e1f57cc6864f0f7aae8730066d444f192560f6bfa3d5f715`.
  Its production export is commit `a6cbf319af2b85f5705bd6161b3d20888d9f9155`.
