# Editor bridge fixture: bounded physical preparation

The exact `PTEditorBridgesTest` passed once in shared Amiberry030. This adapter
prepares the same portable assertions for a separately coordinated RAM-only
real-A1200 run. Physical execution is **NOT RUN**. It does not launch the full
editor or qualify real audio devices.

| Saved qualification | Result |
| --- | --- |
| Candidate | 385144 bytes; SHA256 `21c5efc7fe5d066f56e81ea8cbcea8bc7e95892e8af0a55608a1410d882d9b1e` |
| Source commit | `5771d3ce1fcc3dee1967ec0c728e1fa654954bba` |
| Amiberry result | One launch, complete 624-byte stdout, RC0; 213.214 seconds within 420 seconds |
| Independent release | Normal verified release to EMPTY68; first host release-script refusal preserved separately |
| Assertions | Legacy editor15; checked lifetime39/control alias18; metadata bridge9/admission alias75 |
| Host adapter checks | 26 tests, including every transport failure boundary, cancellation and absolute deadline |

The native evidence and saved-byte verifier are in
[`editor-bridges-native`](../evidence/enhanced-editor/editor-bridges-native/README.md).
The compiler-only record remains historical; it was not rewritten as runtime
evidence. Timers, voices and the bus are injected; the bus uses ordinary RAM.
No physical memory placement, MMIO/sample-RAM completion, voice stop, IRQ,
musical timing, audio or listening acceptance follows from these results.

## Adapter contract

[`editor_fixture_physical_protocol.py`](../tools/editor_fixture_physical_protocol.py)
reuses a caller-supplied guarded shared transport. It owns no connection,
reservation, service, browser control or recovery operation. The caller must
pin the source, product and complete promotion packet, hold the current physical
lease/test lock, wrap each phase in the shared admitted operation, and check
live owner/target identity before **every** callback. The callback must impose
the requested timeout and reject uncertain or failed transport results.

`execute_once` first checks the exact saved native candidate/stdout/RC/flag and
independent normal release. Saved promotion is not live availability. It freezes
both upload files into fresh host custody and admits only a new namespace:
`RAM:PT-editor-bridges-<32 lowercase hex run identity>`. It verifies the two guest
uploads by CRC and size, sends one background launch with Stack65536, and polls
only explicit WAIT or complete RC0/COMPLETE replies. Completion must be observed
within an absolute 420 seconds including launch acknowledgement. It downloads
and verifies all four result files. Transfers and collection also have finite
per-call bounds. Stack size is configured, not measured.

Success means **settlement pending**. A separate `settle_completed` call requires
the exact six-file custody, exact namespace/launcher and caller-verified guest
idleness. It verifies every guest file before the first delete, removes only
those six files and their private directory, then separately checks directory
absence. There is no recursive deletion. Physical baseline/control restoration
and an independent guard release remain caller obligations.

The first failure or cancellation propagates. Neither function sends cleanup,
stop, reset, reconnect, restoration or a retry from an exception/finally path.
The surrounding shared operation must retain HOLD and first evidence on an
uncertain target result. Reusing a populated host custody directory refuses
before target contact. A later recovery needs its own reviewed scope.

## Physical and return phases

The launcher guard binds a reservation to one target. Do not retarget a physical
lease to Amiberry or reuse the old timing adapter's unconditional restoration
path. The prepared physical plan instead describes two separately admitted
phases under continuous peer hands-off:

1. Physical-only fixture, completed-only settlement, independent physical
   absence and disconnect. Its narrowly scoped release must explicitly retain
   the outstanding original030 transport-return obligation; it is not full
   shared-baseline restoration.
2. A new030 return-only lease, original profile/endpoint and idle guest checks,
   disconnect, bounded owned emulator stop and independent final release.

The phase-boundary plan requires shared coordination/review before admission.
It is not an executed wrapper or a global availability claim. No next phase is
automatically dispatched after any failure. Actual authenticated Safari access
and fresh physical ownership/recovery checks remain required; viewing Safari
does not clear another task's central DevBench session. Any acquired browser
control must be released to View Only.

The 26 host tests use a small stand-in executable and the actual saved stdout.
A separate preparation check passed with the real385144-byte product and the
complete saved native/release records. Both checks made zero target requests.
Run the portable host checks with:

```sh
python3 -B -m unittest discover -s tests -p test_editor_fixture_physical_protocol.py -v
```

This addition preserves the sample-master/cache architecture, exact-scheduling
decision, accepted UI and unrelated display work. Production native PLAY and
event-loop wiring, real backend completion/stop and exact activation still need
their own implementation and qualification.
