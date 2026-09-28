# Native recovery adapter

The production AmigaDOS adapter now supports explicit opt-in configuration,
canonical source identity, bounded directory discovery, idle-only timed snapshots,
clean-state removal and later undo resnapshotting. FileInfoBlock storage is
explicitly longword aligned; generated directory names fit older filesystems.
Configuration/media reads and time are private inputs in this fixture. It uses
real DOS traversal and file transactions without changing guest ENV or clock.

Corrected shared030 run render-files-1790554837565478000 passed with candidate
94980 bytes, SHA256 c75c8cb57e603282886e38471d99f3216df1c9daaf67354d25598c363911b427.
All41 dependency hashes matched staged sources before launch. The subsequent
seed-helper addition is a different binary and is qualified separately.
The expected-negative probe exited20 with zero tracked memory before the functional
fixture launched. Functional RC0,365 Fast allocations returned to zero, exact
source preservation, DMAoff and independent run/launcher absence all passed.
The coordinated window was explicitly released. Host ASan/UBSan and3 runner
cleanup/paused-state guard tests passed. Native editor UI is a separate gate.

The original run render-files-1790553901251866000 failed snapshot discovery and
aborted. Its allocator state was unverified, so it remained on hold until a
separately coordinated recovery-only owned stop/start, hash-checked staging
cleanup and independent post-start checks completed. Evidence is retained in
prior-failure. That failed candidate remains unqualified. Inspection found a
concrete two-byte FileInfoBlock alignment defect; the corrected candidate passes.
No physical device, listening, real AmiGUS or performance acceptance is claimed.
