# AmiGUSTest 0.1 — discovery and ownership preparation

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
