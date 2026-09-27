# Full-precision recovery file transactions

An explicit new recovery file wraps the existing streamed PTG project saver with
versioned document identity, revision, saved revision, timestamp and original-path
metadata. All master precision and optional project extensions are retained.
Existing destinations, including the original song, are never replaced. Restore
stages the complete validated candidate and requires matching identity before
replacing the open document; it removes only recovery metadata and marks dirty.
Metadata identifies a candidate; it does not establish external source freshness.
Automatic scheduling, recovery selection and native UI integration remain pending.

Host ASan/UBSan fixture and three shared runner guard tests passed. The native
68000-compatible binary is 71260 bytes, SHA256
`d0bb6ac2919455c79f488f1ca6249df144a287f418c2ac90b2c530c113fe6b68`;
all29 dependency hashes matched staged source. Shared030 run
`render-files-1790551903911536000` passed RC0 with98 Fast allocations returned to
zero owned bytes. It checks exact master roundtrip, all allocation failures,
file/memory budgets, malformed/duplicate/unknown metadata and identity mismatch.
The staged source fixture remained byte-identical. Guarded cleanup plus a separate
live identity/running/all-DMA-off inspection confirmed run and launcher absent;
the window was explicitly released. No physical hardware or audio acceptance.
