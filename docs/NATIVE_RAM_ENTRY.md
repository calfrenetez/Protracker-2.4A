# Owned native RAM readers diagnostic

`PTExecReadersRamEntryTest` is an opt-in diagnostic for the genuine scheduled-readers queue, activation ledger and checked Fast-memory allocator. It is separate from editor PLAY and the broad default build. The four readers access immutable ordinary RAM; it never enables Paula DMA, uploads AmiGUS samples or produces audio. Its CIA adapter acquires a free timer vector through cia.resource and performs only the admitted timer operation.

The diagnostic retains the original frame 4096 at 48,000 Hz grid, epoch and full owner keys. A task prepares a bounded packet; the owned CIA interrupt attempts its RAM activation inside the original window. A missed window produces refusal. There is no rebasing, immediate task-side activation or catch-up.

Normal cleanup requires positive source quiet, command detachment, four independent reader retirements, queue/ledger closure, zero checked ownership and restoration of the original signal/request/task ownership. Uncertain post-arm cleanup enters a passive HOLD that retains the original Process, loaded code, stack, libraries and callback contexts. A host timeout or a free command lock cannot authorize unloading it.

## Local validation and build

The focused host test is `tests/test_native_ram_entry_model.py`. It compiles eight production C units plus two simulated-OS fixture units with AddressSanitizer and UndefinedBehaviorSanitizer, then runs 76 fresh-process cases (54 resource-lifetime cases and 22 task-stack admission cases). These include 18 allocation failures, port/request/device failures, quiet refusals, successful ownership retirement and uncertain-close holds. The C IRQ trampoline, clock, Exec services and stack interval are simulated. This test provides software resource-oracle evidence only.

`tools/build_native_ram_entry.py --cc PATH_TO_PINNED_AMIGA_GCC --out FRESH_OUTPUT_DIRECTORY` builds exactly this diagnostic using the existing portable68000/soft-float/libnix20 convention. It keeps separate objects, dependency files, local stack annotations, the link map and command/result records. The builder never executes the product. Use a fresh output directory; preserve a failed build before correcting its source.

The older `tests/native_ram_port.h` fixture has the same internal guard and function namespace. It must never enter this diagnostic's source closure. New port/CIA files remain together in `src/native/readers_ram/`; includes select that exact version.

## Acceptance limits

The private corrected host matrix passed 54 cases and the native nine-object link passed. The first private host compilation failure (missing simulated `exec/errors.h`) remains preserved separately. The repository-layout host matrix also passed 54 cases. Its standalone native build passed all 22 tool commands (eight C units, one GAS unit and link), with 93 dependency path records (88 resolved files) stable at the final check. The resulting 59,924-byte HUNK is byte-identical to the private linked candidate. See `evidence/enhanced-editor/native-ram-entry/` for the saved receipts. These results are host/compiler evidence only.

Native execution is NOT RUN. A linked HUNK does not guarantee Fast placement, adequate task or system interrupt stack, timer precision, source-stop behavior or interrupt WCET. Local stack annotations and the 44-byte assembly save prefix are not total stack bounds.

The current task admission reads live m68k SP before explicit memory availability, allocation or timer acquisition. It requires a DOS Process, checked nonzero word-aligned bounds/SP, and at least 32,768 bytes of declared downward headroom. Raw `tc_SPUpper` remains the task-identity field; the borrowed usable interval excludes its documented upper-plus-two convention. This is diagnostic admission policy, not measured capacity or total task/system/IRQ stack clearance. CRT/main frames already exist before this check.

The changed candidate passed one repository-layout ASan/UBSan matrix of 76 fresh cases, including 19 pre-resource stack refusals and three threshold admissions. One pinned native build passed 22 commands and produced a 59,992-byte HUNK (SHA256 `b2e473d9e162358976dfddae8ca1651f1874e5687f8eba88187c9db1d7e18d36`). Saved disassembly confirms the live SP instruction and unsigned threshold before availability queries. These are host/compiler checks; this HUNK remains native NOT RUN. The original 54-case/59,924-byte candidate evidence stays separate. See `evidence/enhanced-editor/native-ram-entry-task-stack/`. The separate task/IRQ sampled-stack design remains unimplemented.

Before one bounded Amiberry experiment, complete exact-candidate residency/startup/ABI and practical task/IRQ stack checks, use a normal DOS Process with retained lifetime on uncertainty, enforce the durable hold through shared launch/stop/restart/reconnect/retarget/transport paths, and obtain fresh AmiConnect ownership/recovery clearance. Never poison a live OS stack to manufacture a canary. Observe the exact original schedule and preserve any miss or uncertain outcome; do not retry automatically.

Physical A1200 testing requires that exact candidate's emulator evidence, fresh separately scoped authorization/ownership/live guards, guarded cleanup, independent absence and explicit release. Host, compiler, emulator, physical timing and listening acceptance remain distinct. This diagnostic does not qualify Paula playback, AmiGUS card-RAM capacity/ordering/voice stop or application timing.
