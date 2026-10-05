# Readers backend: native RAM diagnostic

The optional `native_exec_readers_backend_registration_test.c` diagnostic runs
the existing public ABI2 readers fixture through real Exec memory adapters. It
adds no playback backend or editor PLAY integration and preserves the original
musical schedule, Fast masters and independently owned selective Paula caches.

Masters, reader controls, source metadata and construction workspace use the
existing bounded Fast allocator. A zero-budget allocation must refuse without
Chip fallback. Every successful allocation is checked with `TypeOfMem`; the
final Fast-owned byte count must be zero. Selective signed8 playback copies use
the existing Chip allocator, including its per-allocation 512 KiB reserve check.
Those allocations must be Chip-only, with a positive allocation count and zero
owned bytes at completion. These assertions establish placement only when the
exact executable actually runs on a target.

The shared fixture uses genuine mono8/16/24 masters, immediately discards its
construction scratch and checks exact original-frame commands, independent
command detachment and reader retirement, forged identities and uncertain
replies. Only the shared public fixture is included in the wrapper. Queue,
reader adapter and whole-song producer remain separately compiled; no private
queue implementation or owner callback is exposed to the backend.

## Validation recorded so far

| Evidence | Result |
| --- | --- |
| Unchanged shared fixture and core | The existing seven ASan/UBSan host groups passed. Current complete-source review binds the unchanged compiled dependencies to that proof; the Exec wrapper was not executed on the host. |
| Exact Exec-wrapper cross-build | PASS, first/once: 42 RC0 commands, 31 ordered full-M queries, 79 canonical dependencies, 49 SDK dependencies and seven pinned runtimes. All 8,142 frozen source files remained unchanged. |
| Assertions and complete markers | 129 expanded assertions; 133 matching tokens include two macro definitions and two SDK declarations. Shared registration, Fast ownership and Chip ownership markers each occur once in both CPP and HUNK. |
| Saved proof | All 45 original/durable file pairs match byte-for-byte and in mode. Root and independent saved-byte review bind the actual result. |
| Amiberry and real A1200 execution | **NOT RUN.** Fast/Chip placement and native cleanup have not been observed. |
| Native launch stack | **UNKNOWN; 65,536 bytes NOT CLEARED.** Individual compiler frames are not a total call-chain, library or interrupt bound. |

The distinct executable is `PTExecReadersBackendRegistrationTest`, 218,724 bytes,
SHA256 `91c595486224b7fa583a6caedb79688488c5e43e062ad2c8b803d6a6c902f0f5`.
The earlier ordinary-malloc product and protocol1 compact diagnostic retain their
own exact scopes. Neither substitutes for execution of this wrapper. Its source
comment records its original source-preparation state; the cross-build above is
the subsequent actual compiler result.

See [saved evidence](../evidence/enhanced-editor/readers-exec-ram/README.md).
Separate stack assessment and launch clearance, fresh shared ownership and
recovery clearance, exact-candidate Amiberry testing, guarded cleanup and release
precede a separately scoped real-A1200 RAM run. Existing shared recovery holds
remain in force; a free lock or Safari viewing does not clear them.

This fixture performs no audio, Paula DMA, AmiGUS register or card-RAM operation.
Synthetic receipts do not qualify activation, voice-stop, IRQ exclusion, device
ordering/completion, sample-exact hardware timing or human listening.
