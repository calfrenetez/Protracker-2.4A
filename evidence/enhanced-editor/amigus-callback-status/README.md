# Reject unconfirmed AmiGUS bus callback results

Ownership and synchronous register writes require exactly 1. Negative errors and
unexpected positive values previously passed boolean checks, allowing failed
transfers to appear successful. Both the raw arena and reservation-backed owner
now reject them. Failed uploads remain unpublished; ownership loss stays latched
and active leases continue to block reservation release until safely unpinned.

The regression first failed against the old implementation. Twelve focused host
tests and five wavetable/editor integration tests pass. Both native fixtures
passed on the coordinated shared 030 with RC0, one Fast allocation each, zero
owned bytes, no Chip fallback and all four audio DMA channels inactive. Build
manifest inputs independently match the staged source. Fixtures use fake library
and register callbacks: no real MMIO, interrupt, audio or physical acceptance.

Sample RAM: 43304 bytes, SHA256
f6bfc765bfefd09416e597e4dbc5d32c27b7a3db6e5230475eb0e057c11bc34c.
Wavetable owner: 55996 bytes, SHA256
8be0fe593a94166b4efc50771e4303ed290c2f99623be32397f4e419193bd8a2.

Both runners reported immediate removal, but separate host checks found empty
run directories again. The first sample-RAM rmdir failed ENOTEMPTY and an early
release notice was immediately retracted; a fresh guarded cleanup then succeeded.
The owner run changed its guest working directory to RAM before marking done,
but the empty directory still reappeared. That change is a precaution, not a
fix for the unexplained filesystem discrepancy. Each run was finally released
only after guarded exact empty-directory removal and independent host absence
checks. Initial failure and final checks are retained here. No guest reset,
resume or fixture retry was performed. The physical Amiga stayed off/unprobed.
