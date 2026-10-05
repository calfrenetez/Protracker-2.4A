# Native-entry local hold policy

`tools/entry_hold_policy.py` provides a bounded local journal and exclusive reservation for a target and exact run. It records the candidate, target/epoch, Process/task, controller and launch-handle bindings. Its API records caller-supplied observations; hashes, booleans and matching markers do not authenticate Amiga facts.

A timeout, missing completion, uncertain owner or source quiet, or invalid observation retains HOLD. The first HOLD marker stays unchanged. Durable per-tail fences invalidate earlier closure claims, and corrupt or unreadable history creates a permanent corruption latch with no local repair or release path. New closure claims must be distinct and complete. A held run also needs a separate exact-target recovery observation before local release. Completed release history cannot be reused, and run/candidate/log records are preserved.

The local release commit appends RELEASED, removes the exact active metadata slot, then syncs its directory. A storage exception can occur after the slot disappears. The caller must retain its external unresolved hold: an absent slot or RELEASED record does not prove a central barrier was released.

The sealed private V2 fixture passed all 28 host cases once, with RC0 and its complete MODEL footer. These use synthetic observations and disposable files. They cover ordering, exclusivity, credentials, bounded limits, corruption, partial storage, repeated uncertainty/release and preservation. The repository-path fixture also passed all 28 cases once after its import-path integration. The reusable policy bytes and 28 case bodies are unchanged.

Shared lifecycle wrappers are not connected to this module. `central_lifecycle_enforced`, `authenticity_verified`, `target_cleanup_permission`, `stop_unload_retry_permission` and `native_launch_clearance` remain false. Future central integration must preserve possibly live Process/HUNK/code/runtime/context/request/stack owners and block stop, unload, reset, open-path deletion and automatic retry after uncertainty.

Authentic target observations, real crash/power-loss durability, full Process/HUNK retention, native timing, task/system/IRQ stacks, residency and WCET remain unqualified. This local host proof does not qualify Amiberry, a physical Amiga, application PLAY or a 65536-byte launcher stack. It changes no playback scheduling, sample-memory core, classic layout or shared harness.

Saved results and source bindings are in `evidence/enhanced-editor/native-entry-hold-policy/`. Run the local fixture with `python3 -B tests/test_entry_hold_policy.py`; this performs no guest operation.
