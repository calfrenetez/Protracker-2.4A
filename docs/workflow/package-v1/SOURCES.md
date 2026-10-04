# Sources and provenance

**Reference check:** 3 October 2026. These sources establish the interaction ideas, not native-Amiga implementation, performance or hardware test results.

## S1 — ZooperTracker project page

Author/publisher: zooperdan. The page identifies a desktop application derived from the ProTracker 2 clone and lists sample organisation and unused-data cleanup screens.

URL:
```text
https://zooperdan.itch.io/zoopertracker
```

Used for the ZT1 design reference. The more conservative ownership, preview, undo and scope rules in this package are requirements for our project, not verified descriptions of that desktop program.

## S2 — ZooperTracker 1.3 release notes

Author/publisher: zooperdan. Release listed as 2 August 2026. The notes introduce Ctrl-left-click selection from a pattern note and Select Loop, and record a fix so selection-to-new-slot copying uses the selected region rather than old clipboard content.

URL:
```text
https://zooperdan.itch.io/zoopertracker/devlog/1614509/zoopertracker-13-update
```

Used for ZT2/ZT3 interaction references and the stale-clipboard regression case. Context-sensitive inheritance, routing-aware destinations and atomic copy behaviour are additional 2.4G requirements, not claims made by those notes.

## S3 — ZooperTracker 1.2 release notes

Author/publisher: zooperdan. Release listed as 23 May 2026. The notes document opening a sample in the sampler from its list and centring the waveform on either loop bracket.

URL:
```text
https://zooperdan.itch.io/zoopertracker/devlog/1531297/zoopertracker-12-update
```

Used for sample-list navigation and loop-boundary controls. Several ideas available in 1.3 were introduced before 1.3; this addendum does not attribute every feature to the 1.3 release.

## P1 — User approval and existing project decisions

The current conversation approves precisely the three ranked additions in this package. Project context establishes the A1200/ACA1234 50 MHz 68030/128 MB Fast RAM target, native 2.3F foundation, maximum four Paula channels, up to 16 tracks and per-track Paula/AmiGUS/MIDI destination selection, plus emulator-first/AmiConnect testing.

These are user/project constraints, not claims about ZooperTracker. Verify later explicit user-authorised decisions in the receiving project's own records before resolving a genuine conflict. The packaging session did not inspect or change the actual 2.4G development repository.

## P2 — Existing reconstruction audit handover

The supplied document `PROTRACKER_AUDIT_HANDOVER_V2.md` was read to preserve baseline, testing and hardware-safety boundaries. This package neither supersedes nor reissues that audit. It deliberately does not bundle another copy to avoid competing specifications.

SHA-256 of the exact supplied audit Markdown read during packaging:
```text
5a2ea78f03f4140841753a4b3bb2dc71d7393836a83e9c7b21bc3b0e0edb3331
```

The date printed in that earlier audit remains its own provenance; it is not the issue date of this feature addendum.

## What was and was not verified here

Verified: the referenced public feature descriptions, consistency of the three-feature scope with the conversation, package contents, internal test IDs, and file hashes/ZIP integrity.

Not performed: compilation or modification of 2.4G, disassembly, emulator execution, real-Amiga execution, sound capture, latency measurement, memory benchmarking, or validation of the user's installed AmiGUS firmware. The package includes requirements and templates only. All initial test statuses are `NOT_RUN`.

No application binaries, third-party source code, music, ROM images, proprietary assets or firmware are included. Future code reuse must follow the actual licences and the project's established rules; this document is not a legal clearance for copying source or assets.
