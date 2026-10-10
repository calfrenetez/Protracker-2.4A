# Independent STOP01 source review

10 October 2026. Reviewed frozen private candidate `outputs/causal-stop-host-prototype-20261010-v1/frozen-01` and its saved evidence. This is source correctness and HOST evidence review only. I did not compile, link, run a fixture, use Git, edit the candidate/repository/installed infrastructure, or perform any emulator, physical, network or shared operation.

## Verdict

No blocking source defect was identified for the proposed narrow private HOST after-first TRIGGER → STOP alternative. The source and saved cases support explicit opt-in capability, genuine completed-first/selected queue ACTIVE admission, selective actual registry clearing, cancellation and independent original lease retention. They do not qualify a native preparation aperture, IRQ integration, live exact musical scheduling, physical voice stop/quiet, audio or release acceptance. The STOP seam is an alternative second command; it does not implement a CONTROL → STOP third lineage, refill, controller/native integration or authorization reset.

## Exact binding and evidence

All four proposed paths independently match both their STOP01 pins and their immutable `frozen-01` copies:

| Path | Bytes | SHA-256 |
| --- | ---: | --- |
| src/core/mixed_readers_causal.c | 77804 | adc205c1de557ccff2d6537623381a27db6266083cb5580d7cf98c75785d68a7 |
| src/core/mixed_readers_causal_stop_internal.h | 3259 | 70216763b72622013cabe3f61e8a98610637cb6259b2db14e7bdab98bc7e3aac |
| tests/mixed_readers_causal_stop_test.c | 46732 | ec2627aa1925605a7d00a49430d6bda80b68947241b4c43b4d4c5cc7ef5efda5 |
| tests/test_mixed_readers_causal_stop.py | 3138 | 8ccbe0853d63f092909ab339acd30ee42abf2a8fb7b5b5340fc0793bf344ab1f |

I read the complete new STOP header, C delta (`ADOPTED_CONTROL_TO_STOP.patch`, 15591 bytes/SHA f9c9c1328ae9416dc9c89edf5a966903fa1b02acbad575716c56ccf5d2e66c6e), integrated causal preparation/publication/fire/service/close and factory paths, complete STOP fixture, prepared driver and executed overlay runner, README and manifests. The unchanged lower queue admission, targets-current, reader-key, command/reader receipt validation, STOP closure, reference release and original-retirement paths were examined as context. Prior CONTROL source review remains separate; I did not reinterpret its compiler-only result as a STOP result.

The independently inspected saved results are:

| Run | Cases | Compiler/execute | Exact stdout bytes/SHA | Closure |
| --- | ---: | --- | --- | --- |
| stop-08 | 45 | RC0 / RC0 | 542 / 4f6d720be59c9ca2ca3699995169a2b5865c0607f99521f0528170e9c2863593 | 71 before/after source paths equal |
| control-01 | 34 | RC0 / RC0 | 449 / fbc404dc529eaf0af68f5bf5241c5f58e8cbe86820e8a039405c81032b94b41f | 71 before/after source paths equal |
| legacy-01 | 26 | RC0 / RC0 | 521 / 296f5ae9a539b3d0ddfed133858c2ac1f4d5f66dd769b0745845135b9e99864f | 71 before/after source paths equal |

All three saved compiler stdout/stderr and execute stderr files are empty. Their source maps bind the exact final C and new STOP header above. Saved argv uses strict C99, -O1, -g, -Wall, -Wextra, -Werror, -UNDEBUG and ASAN/UBSAN. I statically verified the new normal repository driver's literal ORACLE equals the saved STOP stdout. The executed runner was the focused private overlay, not that prepared normal repository Python driver; after adoption the normal driver remains a separate integration check.

The STOP fixture contains copied CONTROL helper/case functions with an old main renamed and never called by the STOP main. Therefore its 45 cases do not establish the CONTROL regression by themselves. `control-01` and `legacy-01` are genuine separate runs of their own unchanged fixtures against the new causal C. The prior historical stop-06 44-case pass is not substituted for final stop-08.

All earlier numbered STOP failures remain preserved. Their source-before maps use the same final causal C SHA as stop-08: stop-01 incorrect comparison of STOP C time to original R time; stop-02 fixture receipt member typo; stop-03 premature holder-release expectation; stop-04 treating clock-free admission as a late refusal; stop-05 expecting BACKEND rather than actual queue LATE; stop-07 premature selected-R release before independent STOP C detachment. These logs and summaries were read. The final assertions match the unchanged queue transfer/retention contract; production guards were not weakened to make them pass.

## Contract and hazard assessment

1. Capability is explicit, fresh and once-only. STOP bind requires an empty owner/queue, exact original port parent/context capacity, STOP version/flags and a distinct publication callback. CONTROL and STOP binding exclude one another. The default prepublished two-TRIGGER successor API and its completion-match seam refuse a STOP-bound owner. Legacy ports are not implicitly upgraded. Already-bound or malformed refusal-only recursion can return INVALID before `task_enter` without fault; valid overlapping task/fire entry is detected. The header accurately permits the former and does not promise every recursion faults.

2. Admission uses real first completion and queue observations. `stop_first_current` checks first identity/registration, actual commit outcome/times, all twenty completed/predicted/original owner keys and retained active original readers. The typed request exposes only predecessor, original absolute frame, unique route/slot and genuine new command holder. Keys derive internally from completed first metadata, and each selected target must pass the actual queue ACTIVE/key getter, including its holder's current callback. First service observations must therefore be genuine; caller keys, ready flags, replacement reader/card/sample geometry or claims cannot authorize STOP.

3. Caller capacities and scratch guards precede writes. Request/output/new C context must be disjoint from the complete owner, original port, allocator context, queue and all retained command/readers/spans. Their original extents are also guarded against each other and all transformed local scratch. The request snapshot uses memcpy before callback comparison. Genuine enqueue writes the actual caller ticket slot. A real queue OK stays the ownership-transfer authority even if a callback later faults the owner; the wrapper records the transferred ticket instead of pretending refusal and leaking or double-releasing C.

4. First completion and disposed identities are retained as metadata. First may fire alone only in explicit CONTROL/STOP modes. STOP publication uses a distinct typed callback and original port context; its contract requires the port's own completed-first evidence and all twenty actual registry slots to agree before copying. The model independently records/compares those facts. A disposed predecessor's event/binding pointers remain opaque identities and are never dereferenced by the after-first STOP/fire gate; poisoned predecessor cases exercise this boundary.

5. Publication closure is admission closure, not an actual stop. The unchanged lower queue sets selected readers `closed` at successful STOP admission; this forbids later admission. Its publication gate intentionally uses the original active target with admission=false, so the pending STOP remains eligible. The causal owner sets selected `closed` only after accepted or retained-uncertain publication. A clean raw0 returns before that owner change; raw0 plus callback fault retains uncertainty. `causal_current` intentionally checks retained ACTIVE/adopted/cancelled metadata without using closed as a retirement fact, so the pending accepted STOP can fire. `stop_first_current` remains strict before publication, while one-successor state prevents further admission thereafter. This difference is coherent and not a closure bypass.

6. Fire remains finite and uses retained metadata. Original first/last tick boundaries and the full expected registry must agree. EARLY invokes no commit; late fire fails. Successful commit validates selected actual slots are zero/inactive and untargeted slots and adoption are unchanged before owner registry changes. Only then selected original reader state becomes DRAINING. First R observed/issued remain the original trigger times; STOP C observed/issued are its own stop times. No cache, representation, sample acquisition, upload or new reader is created by STOP. Complete master-save beforeimages and cache/pin/lease/allocator counters are checked in the actual fixture.

7. Command, reader and source custody remain separate. Completion/registry clearing does not free original leases. A genuine positive R retirement consumes reader proof; any existing C reference still keeps its holder alive until independent C detachment. Cancel-before-issue is covered both for selected and expected-only original R. Expected-only pending commands must be quiet even though those slots are not selected by STOP; unknown proof retains ownership and prevents another fire. Successful C detachment after raw0 plus callback fault does not imply original R quiet. Separate model source UNKNOWN then later read-only source quiet preserves the source-registration close requirement. Source proof is never inferred from STOP C/R counts.

8. Failure does not establish a fallback stop. Partial/zero/malformed/adoption/late/reentrant commit cases fail/suppress and retain original holders until independent proofs, even if model actual registry has already changed. No automatic retry, time rebasing, rollback to an assumed physical state or sticky fault reset occurs. When lower queue publication detects the original deadline, the wrapper returns exact LATE and the port publication is not called. Admission is deliberately clock-free; genuine transferred unpublished C is then cleaned through the existing local shutdown path, not called untransferred.

## Evidence boundaries and follow-on checks

The 24 lifetime cases cover mixed 4 Paula/12 card, selected subset and 16-card arrangements across 8/16/24-bit masters and 8/16-bit card caches; additional cases cover refusal/aliases, actual port registry, proof order, selected/expected-only cancellation, clean/uncertain publication and failed commits. This is a bounded modeled-device HOST test set, not exhaustive proof of all callbacks or hardware. Correctness still requires the declared immutable caller/context extents, truthful port quiet/commit callbacks and external serialization of task versus fire. The library cannot make a dishonest port's proof physically true.

Compiler-only m68k work, if subsequently performed by Root, must bind these exact source/header bytes and remains separate from linked/native runtime qualification. This review itself did none. Native preparation aperture, IRQ/residency/WCET/aggregate stack, actual Paula DMA/AmiGUS completion/order/voice-stop, exact live scheduling, silence/listening and real-A1200 release are not established here. The current after-first task seam does not solve the missing native deadline mechanism. Broader affected tests or normal-driver checks can be added when Root adopts the proposed scope; no additional repeated fixtures were run by this reviewer.

`REVIEWED_PINS.json` records independently read byte/hash pins and the saved result checks. This verdict authorizes no live or installed change; it reports no source blocker for the stated private HOST candidate.
