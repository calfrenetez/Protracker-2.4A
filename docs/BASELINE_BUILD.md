# Native 2.3F baseline: build and initial verification

19 September 2026. The user authorized proceeding after the updated handover.
The baseline is buildable and has an initial emulator smoke pass. This is not a
2.4G release or the full compatibility/hardening gate.

## Reproduce the build

Requirements: Git, Python 3, Make and a host C compiler. Tested on macOS arm64.

```sh
make bootstrap
make baseline
make test
make fixture
```

`bootstrap` clones and builds the pinned vasm under ignored `local/vasm/`.
It refuses a different revision or tracked local changes in that checkout.
Alternatively provide an installed compatible assembler:

```sh
make baseline VASM=/absolute/path/to/vasmm68k_mot
```

The build verifies every upstream file hash, creates a generated compatibility
source under `build/baseline/`, assembles it, checks the expected executable hash,
and retains the log and JSON manifest. An unexpected output fails acceptance.

| Input/output | Pin or result |
| --- | --- |
| Native source | `8bitbubsy/pt23f` at `4e9df4ada0e55869477e1e45d00227719010e0ea` (7 September 2026) |
| Last assembly-source change | `3cbdf515318a097c050588c25b58a02b08fa74cd` (9 May 2026) |
| vasm source | `AmigaPorts/vasm` at `291f6f08226c8843711e1319864526e6cd57ce06` |
| Assembler banner | vasm 2.0b; M68k backend 2.7c; Motorola syntax 3.19b; hunk output 2.17f |
| Executable | 200,704 bytes; `df8d615f3c38c79240a8c736c29b4ab1a53ffb18406c6814adf470e764b31a08` |
| Official executable | 200,704 bytes; `6b77d26b886c988af9816ed1710bab397848490cf7ec1f9085956fe103b2eb2e` |

Full input hashes and flags are in [baseline.lock.json](../baseline.lock.json).
The source snapshot contains 51 original tracked files. No tracker feature,
branding, effect or resource content was edited.

## Assembler compatibility finding

An initial direct vasm assembly launched but failed the interactive Disk Op
probe that worked with the official release. Matching section sizes and
relocations alone did not establish correctness.

The generated source now spells immediate-to-data-register operations explicitly:
`CMP` -> `CMPI` (968), `AND` -> `ANDI` (195), `ADD` -> `ADDI` (103), `SUB` ->
`SUBI` (43), and `OR` -> `ORI` (9). This affects 1,318 instruction spellings in
the build copy. It preserves labels, expressions, addressing, comments and the
original snapshot. Address-register operands and non-immediate instructions are
unchanged. This is a narrow pinned-source compatibility adapter, not a general
assembly translator.

Flags are `-devpac -m68020 -no-fpu -Fhunkexe -kick1hunks -hunkpad=0 -nosym`.
Devpac mode disables optimisations and uses zero alignment fill. `-m68020`
permits the upstream CPU-guarded long multiply/divide instructions; the source
retains its 68000 software fallbacks. This does not constitute 68000 runtime
testing. Kickstart-1-compatible hunks avoid short-relocation records.

After preparation, **every loaded code/data byte, section type/allocation/size,
and all 4,847 relocation entries match the official release**. Whole-file hashes
differ because relocation records are ordered differently. The comparator
requires exact code/data equality; it does not accept opcode resemblance as
equivalence. Both code hunks and the Chip-RAM data hunk match; both BSS
allocations also match.

To repeat the comparison, retrieve the official archive linked by the pinned
upstream README, extract it locally, then run:

```sh
python3 tools/compare_hunks.py build/baseline/PT2.3F /path/to/official/PT2.3F
```

The official download may change later; compare its hash with the locked
reference before interpreting results. The source-built artifact and comparison
report are retained under [evidence/baseline](../evidence/baseline/results.json).

## Observed checks

| Check | Result and boundary |
| --- | --- |
| Source snapshot integrity | PASS: all 51 files match the pinned upstream tree. |
| Fresh assembler bootstrap | PASS: pinned assembler built with the host compiler in this repository. |
| Repeat build | PASS: two fresh-assembler builds produce the same locked executable SHA-256. |
| Official binary comparison | PASS: exact loaded code/data and relocation equivalence; file bytes are not identical. |
| Host tooling tests | PASS: seven tests cover preparation scope/idempotence, strict hunk comparison, truncated/trailing records, invalid/different relocations and synthetic MOD bounds. |
| Emulator launch | PASS: classic 2.3F interface visible. |
| MOD load/play/stop | PASS: synthetic four-channel module loaded, pattern rows progressed, scopes animated and all four emulated Paula DMA channels were active during play. |
| Metadata edit/save | PASS: title changed and a new 2,172-byte MOD saved; every byte after the title remained identical to the input. |
| Reopen/play | PASS: saved title visible after reopening; playback progressed again. |
| Normal exit | PASS: quit confirmation returned to Workbench. |
| Cold relaunch | Recorded separately in the evidence manifest; not an endurance test. |
| Pattern/sample editing, complete effect/timing corpus, fuzzing, save-failure tests | NOT RUN: still required for the broader compatibility gate. |
| Physical AmiGUS/MIDI/audio quality/endurance | NOT RUN. No listening-quality or measured Studio-capacity claim. |

Environment: locally patched AmiBerry 8.3.0 (2026.08.19), SDL 3.4.14, PAL/AGA,
68030, no FPU, 2 MB Chip and 128 MB emulated Z3 Fast RAM, Workbench/ROM 3.2.3.
This memory/bus model is not an ACA1234 timing benchmark. The test used a private
copy of the existing disk, a dedicated shared folder and disabled emulated
networking. Original OS media and the physical machine's files were not altered.

The IPC helper refuses to control configurations whose description is not
`ProTracker isolated baseline`. Mouse deltas are bounded to avoid counter
overflow. Test actions used the application's file/editor interface, not guest
memory writes. Screenshots and saved-file comparisons supplement state probes.
The current helper supports automation; it is not yet a complete unattended
compatibility suite. The launch stub detaches, so its shell return is explicitly
not proof of tracker exit.

## 2.3FC audit and licensing

The maintainer's [release directory](https://psk.usermd.net/) supplied
`PT2.3FC.lha` (13 entries). It contains a binary, readme, config/help, sample
modules and templates, but no assembly source or separate licence for FC changes.
No FC code or bundled music was imported. A source-level diff remains blocked
on source/licence availability.

Its readme describes sample-based colouring, brightness/blank-zero changes and
SPREAD/100-pattern fixes. The pinned 2.3F changelog already includes a newer
SPREAD fix and an M!K! large-pattern loader fix; overlap is a candidate for
regression testing, not proof that all FC loader changes are included. Optional
colouring remains a later design choice.

The native snapshot retains its BSD-3-Clause licence and original contributor
credits. This includes original graphics, compressed graphics, embedded replay/
relocation resources, help, icons and historical utilities as supplied by
upstream. Imported executable utilities are preserved as provenance and are not
run or needed for this build. The assembler stays outside the repository; its
own licence permits M68k/AmigaOS commercial-target use. AmigaOS/ROM media are
local licensed dependencies and are not packaged. The test MOD is generated
from an original triangle waveform; no third-party song is required.

## Real-machine automation finding and next gate

The existing A1200 AmiConnect endpoint returned `status=ok`. This proves HTTP
reachability only. The inspected local implementation has authenticated,
opt-in upload/run/log profiles for IDS tests and Aqua Bonk, with fixed filenames
and arguments; it is not a general shell and currently has no dedicated
ProTracker/AmiGUSTest profile. Its documentation labels physical acceptance of
that extension outstanding. No token was exposed or committed and no program
was deployed by this baseline run.

Next: establish the dedicated diagnostic execution/result contract, check the
installed physical service capabilities and versions, and validate a harmless
bounded diagnostic before card ownership/interrupt tests. In parallel with that
dependency work, expand the generated MOD/effect and loader/edit regression
corpus. Only then freeze the clean baseline and begin the backend changes.
