# Frozen sampler mixed-reader factory final review

**BOUNDED PASS. No blocking findings in the requested factory ownership, guard, proof, drain, close, and pressure scope.** This is a read-only review of current source and previously saved host results. No compiler, test executable, product, emulator, target, or device was executed during this review. Only this report directory was written.

Repository: `$PRIVATE/work/Protracker-2.4A`.
Core baseline and current HEAD: `c4cbc0419a920e70ed4db1194373a75a6b1a871e`.
Saved attempt: `outputs/sampler-mixed-readers/v1/attempt-g2k6au4m/run.json`.

## Exact source and evidence identity

All four current files are byte-, size-, and mode-identical to their actual frozen copies and to both the source-ready and run manifests:

| File | Bytes | SHA-256 |
|---|---:|---|
| `src/editor/sampler_mixed_readers.h` | 9694 | `77477f67b51c8284099b611c720ae1a7c5d136774484fa049745e76c129a635e` |
| `src/editor/sampler_mixed_readers.c` | 40675 | `5c1eaa92dbf2912d122e11eb3819abf996d0a39139c9f5b59f260ecba7635021` |
| `tests/sampler_mixed_readers_test.c` | 41970 | `205bcf033f69c8d8a1eab296e0f35449ba4249ef5f06aa20d07524266c8e937d` |
| `tests/test_sampler_mixed_readers.py` | 5573 | `843d41aa19bb06772e8a294dd868ff011cb1d79fdce01f54255f6dc662060cbb` |

The actual frozen tree has 927 files, with zero mismatches against all 927 recorded inputs. All 922 inherited `src`/`tests` files match baseline bytes and executable bits. The five additional files are precisely these four overlays and `pt_font.h`; that generated font matches the pinned baseline font transformation. The archive uses mode 0664 while extracted files use 0644; this affects group write permission only, with no content or executable difference. All 16 protected current paths match the saved run manifest. Their bytes were not changed by this review.

`source-ready.json`: 3066 bytes, SHA-256 `b731a63889ef2eb53b4b185a15a596e6dd7ff4785cf9f2ba47fcce1b826b0b1d`.
`attempt-g2k6au4m/run.json`: 163328 bytes, SHA-256 `10fdd5480289d99e01a3d6bca817a43f35ea0be666253ed143750138fd39cec1`.

The saved core used by this attempt is baseline `src/core/mixed_scheduled_readers.c` SHA-256 `8efb581095b936059fbf2e3a92fb02261ef9323496f813408f82fc98a2c55b84`, and its header SHA-256 `39cfe1b89604d851d975c6c34a2f30df8bed0b244a69cadb420146a24c22b596`. Current working core, activation, editor, and native overlays are outside this factory attempt.

## Review conclusions

1. **Each trigger owns an independent persistent master reference and derived lease.** Factory `PIN` retains into `r->pin` before starting the prepared Chip/card job (`sampler_mixed_readers.c:652-671`). Those jobs retain their own TEMP references in the exact baseline helpers. HIT and completed-job cancellation retire TEMP references while retaining the factory's master reference and transferred cache lease. `resources()` cancels TEMP jobs, unpins the derived lease, and then retires the persistent master exactly once (`392-405`). Duplicate sample triggers have separate reader records and retain operations even when their version/resource pointers coincide. The tests establish genuine masters independently before factory open (`test:152-163`), exercise 16 readers across 24 depth/endian/proof-order combinations (`324-350`, `921`), and use literal selected-channel, signed conversion, endian and padding oracles (`262-312`).

2. **Released caller pins and released sampler current ownership are specifically covered.** The six `smf_persistent_master_lifetime` cases unpin caller references after READY, then enqueue/publish, release sampler current ownership, assert current slots are NULL, and read retained master PCM while the appropriate command/reader references still exist (`test:795-834`, `937`). Both proof orders run for 8-, 16-, and 24-bit masters. The independent original PCM is restored before later save assertions, avoiding a save oracle over freed promoted storage.

3. **Route slots, cache slots, and RAM-arena indices remain distinct.** Channel routing obtains Paula physical slots and sequential AmiGUS route slots (`c:314-320`, `341-375`). The card action derives its arena index by matching the genuine `pt_cache_data()` pointer to `arena.block[]`, while recording cache slot/serial separately (`596-614`). Subsequent current checks validate the saved arena index and lease identity (`493-498`). The fragmentation fixture explicitly proves cache slot 2 maps to arena block 0, with logical size 6, full reserved size 8 and address 0 (`test:505-542`).

4. **Complete output guards precede writable faults, callbacks and writes.** `output_apart()` protects full pool storage, captured source/context capacities, all registered commands and readers including RETIRED records, current sampler version capacities, and full Chip ledger extents (`c:106-114`, `157-175`). APIs admit output/input extents before `reentry()` and callbacks. Receipt services use private local receipts and recheck before publishing them (`751-789`). The live alias fixture checks seven owned/borrowed locations and proves underlying service call counts are unchanged on rejection; retired controls remain guarded until close (`test:609-638`). A separate fixture adds newly owned PCM capacity and guards it after both old project and sampler table pointers are replaced with sentinel addresses (`723-770`).

5. **NULL receipt output allows stale-source draining without former-table traversal.** `service_ready()` checks fixed source identity only when an external receipt is requested, while a NULL output reaches the exact borrowed queue service (`c:751-789`). Fixed identity checks precede former project-table walks elsewhere (`125-148`, `502-509`). Captured cancellation and release paths do not walk former project tables. Expiry fixtures cover 20 preparation phases and live queue draining, using sentinel table pointers plus changed generation; the live non-NULL receipt remains untouched, while NULL services retire the exact domains (`test:478-503`, `640-673`). Queue/backend/sampler fixed owners still must remain alive through draining, as the header requires.

6. **Command detachment and reader retirement remain independent proofs.** Factory command release only retires its command context (`c:516-530`); resource release occurs in the separate reader release callback (`531-543`). In the exact baseline queue, readers release only when independently retired and with zero command references (`mixed_scheduled_readers.c:282-294`); command and reader envelopes are separate (`298`, `381-404`). Tests run both proof orders, retain 16 readers when reader retirement precedes command detach, preserve unadopted readers after command detach, and suppress malformed receipts while allowing exact-domain draining (`test:324-350`, `544-607`, `880-908`). CONTROL/STOP construction retains only original numeric keys (`c:577-584`), with factory tests covering both operations (`test:674-710`).

7. **Consumed close returning 0 is handled as consumed storage.** Reader/command handles are cleared before their final allocator release; registrations and budget counts are removed before those releases (`c:406-417`, `816-844`). Chip ledger entries are removed before Chip release (`265-277`). Pool close sets the caller slot NULL before its final allocator callback and returns from a local fault variable, so it does not read the released pool (`846-872`). Saved fixtures explicitly induce Chip and pool release reentry, require return 0 with a NULL pool, and verify repeated NULL close is safe (`test:836-846`, `910-917`). LIVE handles and retained registrations cannot be force-closed.

8. **32 retained readers and replacement pressure are explicitly tested.** The test detaches and closes each of two successive 16-trigger command contexts while retaining both reader generations. It asserts 32 queue readers, rejects a 33rd allocation attempt with unchanged sentinel output and no allocation callback, drains the earlier and later reader generations separately, then closes all 32 handles (`test:848-878`). Source admission budgets every reader and registration before allocation (`c:438-440`). This proves software lifetime/capacity behavior, with the tracker itself remaining capped at 16 channels.

## Saved host result and limits

The runner archives the exact baseline and overlays only the four reviewed files (`runner:6-9`, `26-36`). Its saved factory, paired-core and ABI2 compile/run calls all record status PASS and return code 0. Recorded compiler flags include C99, `-Wall -Wextra -Werror`, and AddressSanitizer/UndefinedBehaviorSanitizer. All six stderr files are empty. Factory stdout includes the factory suite PASS and the inherited mixed-reader suite PASS; paired stdout contains the separate baseline mixed-reader PASS; ABI2 stdout reports independent command detach and persistent reader retirement. Log fingerprints and complete saved call arguments are in `verification.json`.

Applicable repository instructions and shared onboarding were read, including the definitive 2.4G scope. Host review proceeds under the current task's authorization; it does not resume a target session. No target ownership was acquired, no shared lock or service was changed, and Scott priority was left undisturbed.

Acceptance is confined to this factory's exact four overlays on the exact baseline with saved HOST_ONLY software evidence. It does not qualify the current combined working tree, native compiler/ABI, editor/song integration, production backend, hardware/device, IRQ, stack, timing, playback/listening, or physical operation. Genuine objects, stable complete callback-context declarations, exclusive borrowed queue use, revision/generation discipline, and owner lifetimes remain the explicit caller contract.
