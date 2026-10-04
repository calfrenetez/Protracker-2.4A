# ProTracker 2.4G — 030 workflow addendum

**Package:** PT24G-030-WORKFLOW-v1  
**Issued:** 3 October 2026  
**Status:** Approved feature scope and proposed implementation/acceptance requirements. No application code or real-hardware test results are included.

## Purpose

Add the three agreed ZooperTracker-inspired workflow improvements to the **existing native Amiga ProTracker 2.4G project**:

1. **ZT1 — Sample manager and conservative unused-sample cleanup.**
2. **ZT2 — Loop/selection toolbox:** select loop, jump to loop boundaries, and copy the current selection into a genuinely free sample slot.
3. **ZT3 — Pattern-event-to-sample/instrument navigation.**

The principal target is the user's A1200 with ACA1234 **50 MHz 68030 and 128 MB Fast RAM**. These additions must not introduce a requirement for an FPU, an 060, a desktop runtime, or an upgraded audio engine.

This is an **addendum**, not a replacement master specification, a new tracker project, or a new reconstruction audit. It refines existing editor, slicing and undo work rather than counting those as separate newly approved features.

## Use in the existing development chat

Attach this ZIP to the current 2.4G Codex chat and paste `CODEX_START_HERE.txt`. Codex should read the project's own instructions and existing scope first, then integrate this document into the existing backlog and implementation. Do not create a new project or switch the historical source baseline.

## Files

| File | Purpose |
|---|---|
| `CODEX_START_HERE.txt` | Paste-ready instruction for the existing development chat. |
| `PT24G_030_WORKFLOW_ADDENDUM.md` | Authoritative requirements for these three additions. |
| `ACCEPTANCE_TESTS.md` | Detailed fixture and test requirements, including real-A1200 validation. |
| `TEST_MATRIX.json` | Machine-readable planning matrix. Every initial result is `NOT_RUN`. |
| `SOURCES.md` | Primary feature references, user/project constraints and provenance. |
| `MANIFEST.json` | SHA-256 hashes and sizes of all other package files. |

## Boundaries that must survive implementation

Keep the project's exact native 2.3F baseline pinned and pristine. Do not switch to 2.3FC or the desktop clone. Retain up to 16 tracks, one selected output destination per track (Paula, AmiGUS or MIDI), and no more than four simultaneous Paula channels, unless the existing project contains a later explicit user-authorised decision.

Do not add Jam Mode, automatic BPM/key detection, sound-similarity merging, chord-to-arpeggio entry, extra export modes, a new file format, or general sample renumbering as hidden scope. Existing features remain governed by the master requirements.

Cleanup must be preview-first, conservative and undoable. Sorting the sample list must never silently renumber instruments. Copying must use the **current selection**, not an old clipboard. A zero sample field must never make navigation guess from the currently selected editor sample.

Implement and test incrementally. Use Amiberry first and the existing AmiConnect workflow for the real A1200. Report missing access honestly; do not label emulator-only results as real-hardware validation.
