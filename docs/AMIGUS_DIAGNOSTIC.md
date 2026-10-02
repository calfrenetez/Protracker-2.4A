# AmiGUSTest 0.3 — guarded idle AHI ownership window

1 October 2026. The user explicitly approved temporarily unloading the unused
resident AmiGUS.audio driver, checking direct PCM ownership, then restoring it.
`--idle-ownership` implements this single transaction. It pins the existing
amigus.library1.1, refuses an unknown driver ID/version, either open-user count
or a delayed expunge, and requests only normal Exec `RemLibrary` with the guard
snapshot and removal serialized by `Forbid`/`Permit`. It never calls a foreign
owner's `FreeCard`, edits library flags, flushes unrelated libraries, kills a
player or changes saved settings. After confirmed absence it runs the existing
PCM/wavetable checks. Known failures/cancellation also run restoration through
normal `OpenLibrary`/`CloseLibrary`, then verify the fresh resident node.
Uncertain card release or driver restoration retains the diagnostic Task,
base library and owner addresses indefinitely; the harness retains target/files
and does not retry or clean an uncertain transaction.

The physical runner additionally binds the installed AHI file to the official
RC6 020 NO_LOG snapshot and checks both public open counts before staging.
The native guard repeats those counts at removal. Exactly matching native
candidate checks are required before physical selection. Eight sanitized host
failure/refusal/restoration scenarios pass. On shared030, the same Exec callback
code passes busy/unknown refusal, normal removal and Open/Close restoration with
a separately named owned temporary library, followed by absent-node/Fast-zero
checks. The real diagnostic's three missing-library modes return5 as expected.
This fixture supplies its own reload; it does not qualify loading the real AHI
driver from disk or real PCM ownership. Initial independent file cleanup FAILED
and remains recorded; separate inspected empty-only recovery and subsequent
independent cleanup PASS. Evidence: `../evidence/enhanced-editor/amigus-idle-window/`.
The separately coordinated physical idle-driver transaction now **passes**:
run1790891886482150000 on the realMini returned0; both PCM/wavetable checks
passed with confirmed final release, then resident4.23/020 AHI was reloaded
with openCnt0 and delayed-expunge clear. Post-run bridge inspection and unchanged
installedCRCBA67FEF4 verified restoration. Exact RAM cleanup, separate absence
and return to the connected idle shared030 passed. Evidence:
`../evidence/enhanced-editor/amigus-physical-idle-ownership/`. Audio/register/IRQ
and human listening acceptance remain open.

Exec lifecycle contract: [Commodore exec.library autodocs](https://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node059D.html).
The pinned public RC6 source and earlier physical inspection are recorded in
`../evidence/enhanced-editor/amigus-idle-driver/`.

## Original 0.1 qualification (historical)

19 September 2026. Native CLI executable built; emulator absent-library path
verified. Physical AmiGUS discovery/ownership, audio, capture, interrupts and
endurance are **NOT RUN**. This is the first diagnostic increment, not completion
of the handover's hardware milestone.

## Build and run

```sh
AMIGA_CC=/path/to/m68k-amigaos-gcc make diagnostic
make test
```

On AmigaOS, run from a Shell (the diagnostic has no Workbench icon entry):

```text
AmiGUSTest --discover
AmiGUSTest --ownership
```

No arguments means `--discover`. Unknown arguments fail with DOS return code 20.
Discovery uses `OpenLibrary("amigus.library", 1)` and `AmiGUS_FindCard`; it reports
library version/revision, card type, hardware/firmware revisions, firmware date
and block presence. Library loading can perform its own device initialization;
this is not a promise of electrically passive bus inspection.

Ownership uses separate PCM and wavetable reservations. It checks a competing
owner is rejected, a wrong-owner release does not steal the reservation, and
release/reacquisition works with both identities. Existing owners cause SKIP.
The codec is never reserved. The diagnostic writes no device registers, installs
no interrupt, plays no sound and performs no firmware update. Ctrl-C is checked
between bounded stages; cleanup attempts to free both diagnostic owner IDs.
`FreeCard` returns void, so a driver which ignores releases can be detected during
reacquisition but cannot be forcibly repaired by this tool.

Return codes: 0 = requested checks passed; 5 = unavailable/busy/incomplete;
20 = failed check, cancellation or bad invocation. Logs begin `AMIGUSTEST`,
identify the mode and end with a machine-readable `SUMMARY` line. PASS applies
only to the named mode; the `SCOPE` line explicitly leaves audio/interrupts
untested. Library-unavailable is not counted as a hardware pass. Enumeration is
limited to 16 cards and rejects cycles; unknown card types are discovery-only.

## Evidence and limits

- GCC `13.4.0b 20260811114005`, `-m68000 -msoft-float -mcrt=nix20 -Os`, warning-free
  with `-Wall -Wextra -Werror`. Binary: 13,952 bytes; actual inputs/compiler digest
  and output digest in [build evidence](../evidence/diagnostic/build.json).
- Nine host unittest groups pass, including an ASan/UBSan C test of 18 ownership
  scenarios: busy foreign owner, each reserve failure, cancellation, exclusivity
  violation, invalid release behavior, sticky ownership and invalid input.
  These use a mock driver, not an AmiGUS card.
- Thirteen native executions under isolated Amiberry 8.3.0, 68030/no FPU,
  Workbench 3.2.3: discovery and ownership correctly returned missing-library
  SKIP/5, ten further launches did likewise, invalid arguments returned 20.
  Exact logs and emulator configuration are in
  [emulator evidence](../evidence/diagnostic/emulator.json).
- The native positive-card/API paths and actual 68000 execution remain untested.
  A 68000 compiler flag is not a 68000 runtime acceptance result.
- Native tests use the private disk and temporary launch hook described in
  `tools/test_diagnostic_emulator.py`. Other emulator profiles are refused.
  Coordinate exclusive emulator use with the AmiConnect task before invoking
  that runner. ProTracker released its instance after the recorded test.

## SDK review

Pinned public interface: `d8c9a0429f41cd5f3dbadae34ef438e45c9c3718`.
[Official release rc-6](https://github.com/necronomfive/AmiGUS-pub/releases/tag/rc-6),
dated 30 August 2026, was the latest release checked on 19 September. No driver
or firmware was installed or changed. Header/SFD match that release.

Both Zorro II (`0x7000`) and Mini (`0x7001`) return the same public card structure.
The API supplies discovery, resource reservation and interrupt ownership;
playback/capture still requires the documented register interface. The presence
of an address is not evidence of a tested voice count, sample format or rate.

Two details found in upstream source affect future work:

1. `ChangeCardReservation` can reserve one requested block and fail another.
   Requests here contain exactly one block. Future multi-block acquisition must
   track each acquisition and unwind explicitly.
2. The public header's interrupt prose mentions d1 while its typedef uses a0.
   The current AmigaGuide also specifies a0. Verify the compiled driver call
   path before enabling interrupts; do not take the stale prose as the ABI.

Only the public header/SFD and required license texts are vendored. See their
[provenance and license note](../vendor/amigus-sdk/README.md).

## AmiConnect integration

The running AmiConnect development task is implementing a native authenticated
capability layer and Mac MCP bridge informed by AmiMCP/amiagent; it does not
require a second amiagent daemon. Its current scope includes upload/download and
AmigaDOS execution with working directory, deadline, bounded output, actual DOS
return code and explicit running/timed-out state. That should replace the need
for a special hard-coded diagnostic profile when delivered and enabled.

At the inspected checkpoint only capability discovery was implemented in the
bridge; generic upload/exec were pending. Existing fixed profiles cannot launch
this tool. No foreign profile has been repurposed. No changes were made to the
actively edited AmiConnect checkout and no physical machine command was sent.

Required consumer sequence once those capabilities are ready: discover support;
stage this exact binary under a unique run directory; verify size and digest;
run `--discover`, then approved `--ownership`; collect bounded output and DOS
return code separately from transport status; validate the summary; never retry
an uncertain launch or treat a timeout as proof that the process stopped.

Next hardware increment: actual discovery/reservation on each available card,
then interrupt lifecycle and one voice, followed by measured 4/8/16 voices.
Positive physical results and human listening remain separate acceptance gates.


## Physical discovery adapter (1 October 2026)

The user now authorizes real A1200/AmiGUS testing. Current production-adapter
PTAmiGusDiscovery15524-byte candidate SHA39036e5f... has fresh shared030 missing-
library qualification and separate independent cleanup evidence in
`../evidence/enhanced-editor/amigus-current-discovery/`. Positive physical card,
reservation, interrupts, cache transfer and audio remain distinct open gates.

Use `tools/shared_infra_amigus_discovery.py --emulator-result <result.json>
--independent-cleanup <result.json>` with a separately coordinated physical window.
It validates exact binary/source/SDK hashes and both emulator evidence records
before selecting hardware, takes the shared nonblocking lock, uses existing
harness target/bridge guards, stages a checksummed immutable copy in a fresh RAM
directory and performs discovery only. Known finished scripts permit exact file/
empty-directory cleanup, a separate absence check and return to emulator. An
uncertain async script retains the target and paths for recovery; no cancellation,
reset, automatic test retry, recursive deletion, installation or firmware change.
Two host refusal/uncertain-execution tests PASS. Actual physical run not yet claimed
by this preparation record.


Physical production-adapter discovery completed: two probes each library1/cards1/
pcm_cards1/closed1; RAM cleanup, separate absence and target return PASS. Evidence
`../evidence/enhanced-editor/amigus-physical-discovery/`. This supersedes the older
physical-unavailable state, but does not qualify AmiGUSTest ownership or audio.


## AmiGUSTest0.2 final-release gate

Before physical reservations, final release is now checked with the verified RC6
NULL-owner reserve semantics, separately from void FreeCard issuance. Busy/error
retains native library/card/Task/owner addresses in a cooperative HOLD. Runtime
library1.1 and installed file checksum matching the official verified RC6
variants are required by the physical adapter. Unknown contract refuses before
reservation. The NULL probe does not acquire a new owner or free a foreign owner
under that verified implementation. Evidence and conditional contract/source link:
`../evidence/enhanced-editor/amigus-ownership-release/`.

Host20 injected failures PASS, pinned build PASS; native/physical ownership
NOT RUN in this preparation record. Use separately coordinated shared adapter
`--amigus-diagnostic`, then physical adapter `--ownership --library-contract
build/dev/amigus-ownership-library-contract.json` with matching emulator and
independent cleanup evidence. Retained/asynchronous uncertainty never authorizes
file cleanup, target return, exit, process cancellation or automatic retry.


## Actual ownership qualification — 1 October2026

Current diagnostic0.2 native20case fixture/privateABI/missinglibrary modes pass.
Initial sharedfilesystem cleanup FAILED; separate exact empty-directory recovery
and independent locked verification pass. Original failure retained in
`../evidence/enhanced-editor/amigus-current-ownership/`.
RealMini physical ownership completedRC5: PCM foreignbusy0x101 skipped without
release/probe; wavetable competing/wrongowner refusal, reacquisition and confirmed
finalrelease PASS. Knownfinished RAMcleanup + separateabsence/targetreturn pass,
windowreleased. Overall ownership INCOMPLETE; awaiting existing audio owner's
release, no automatic physical retry. Discovery previously passed. Registers,
sample RAM transfer, IRQ, output, physical timing and listening remain open.
Evidence `../evidence/enhanced-editor/amigus-physical-ownership/` supersedes older
physical NOT RUN statements only for these exact named checks.


Guest-aware cleanup pilot also FAILED independent absence despite successful
AmigaDOS deletion and2s host observations. Separate inspected empty-only recovery
and locked subsequent verification pass; explicit windowrelease ACK147. Root
cause remains unproven. Method retained behind experimental --guest-cleanup-pilot,
not adopted as default; ten host guards pass. See
`../evidence/enhanced-editor/guest-cleanup-pilot/`. Original failures preserved.

### Version0.9 wavetable status

`--wavetable-status` is a separate bounded read-only Mini diagnostic: eight global
IRQ/mask reads under a WAVETABLE lease, confirmed own release, no MMIO writes,
voice-bank selection or idle-AHI unload. It refuses an unverified library
revision or hardware/firmware descriptor. Status observations do not establish
all-voice stop, sample-RAM capacity/readback or playback. The synthetic reader
fixture is `PTWavetableReadTest`; both its exact bytes and the real missing-library
mode must pass native qualification before a separately coordinated physical run.
