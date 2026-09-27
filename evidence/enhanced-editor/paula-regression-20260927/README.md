# Committed-snapshot Paula regression, 27 September2026

The full native build from commit1ac4856 produced139executables. All288listed
source hashes match that commit and all139binary lengths/SHA256 values were
independently verified. Build evidence does not mean all139programs ran.

Five binaries from that export passed shared030 run
paula-cache-1790548469572334000, each RC0 under the60second combined bound:
selective sample caches, PCM conversion, bounded upload, preview conversion,
and actual emulator Paula/replay ownership. See result.json for exact hashes.
They cover independent Chip allocations, Fast metadata, omission of unused
payloads, refresh/reuse, partial allocation rollback, true24 master-preserving
mono/stereo preview, cancellation, repeated restart/edit/Stop, unsupported routes,
CIA exhaustion and preservation of another timer owner.

CIAA fallback execution was explicitly NOT RUN because Workbench already owned
CIAA timerB. No existing timer vector was replaced. This limit remains open.
Both post-test and cleanup audio checks show all4DMAoff. The caller now reuses
running-state refusal and observed-absence cleanup guards. Exact run/launcher
cleanup and separate absence checks passed; AmiConnect window explicitly released.
No physical Amiga/AmiGUS, native Studio output, listening or performance acceptance.

The wrapper was loaded from tools/shared_infra_paula.py with ROOT set only in
that process to build/dev/qualification-1ac4856. No installed shared configuration
changed. Broad host regression results are recorded separately when complete.
