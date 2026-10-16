# Prepared RAM-only native test route

## Pending completion-marker observation —16 October2026

A newly preserved staging attempt acknowledged its one Shell launch, then the
bridge reported that the completion file was missing. The transaction correctly
failed closed. Its exact resources were independently recovered and released;
no native result or rerun is inferred.

The protocol now observes a complete owned-directory reply while a Shell marker
is absent or shorter than its exact terminal length. It opens the marker only
after the terminal length appears, then retains every strict content, capture,
checksum and custody check. Unknown entries, directories, oversized markers and
actual error replies still fail immediately. A missing/partial file is pending,
not successful completion. Original30-second staging,90-second native and whole
transaction limits, one launch per phase and sticky failure remain unchanged.

The baseline reproduces the missing-file failure. Eight regressions cover
staging/native absence and partial writes, both deadlines, wrong terminal text,
foreign entries, oversized markers and actual directory errors without further
requests. All75 owned-protocol model tests and28 raw-protocol tests pass. The
updated private Studio packet passes35runner/27service/15entry checks; the
physical packet passes25runner/27service checks. Native execution of this new
observation path now passes once with the exact Studio master-protection fixture:
48 refusals and48 controls, RC0 and independent cleanup/release/final idle. An
offline check revalidates the raw capture and read-only source inventory. The
actual guest run does not inject marker disappearance; those transitions remain
host-model evidence. The same new observation path also completes the exact202,556-byte66-case
reader-memory fixture once on the A1200, followed by independent RAM/task
absence, public release, own-service exit and separate final idle. Safari stays
in View Only throughout. These are RAM-only functional results, not device stop,
DMA, exact timing or listening acceptance.
Directory replies are observations, not atomic filesystem or handle-close proof.
See [scoped evidence](../evidence/enhanced-editor/native-completion-observation/host.json).


Status: **Exact Load fixture and full owned-RAM copy/capture/cleanup route pass
in Amiberry. Independent restoration, release and final idle checks pass.
The exact sampler18 fixture also passes the incremental direct-upload route on
the real A1200, with independent cleanup and release.**
These helpers do not activate or unblock the older physical runner by themselves.
Product architecture, timing policy and editor layout are unchanged.

The installed SCRIPT helper chooses a short time-derived ID, creates two hidden
`T:` files, and treats capture deletion as best effort. Its directory wrapper can
also return a partial listing after a later page times out. Those behaviors are
unsuitable for proving exact temporary-file ownership on the real machine.

`owned_native_run.py` prepares an alternative using the existing bridge commands:

1. Record the full run identity, candidate hash, script hash and five exact file
   names before dispatch (eight for emulator copy mode). Require a successful `MAKEDIR`/`CreateDir` for a fresh
   RAM directory; never adopt a directory that already exists.
2. Upload frozen candidate/script bytes and an empty completion flag. Check
   guest CRC/length and read back the script exactly. The classic directory
   component stays below 30 characters.
3. Launch `C:Execute` on the uploaded script once through the guarded `LAUNCH`
   leaf. Native stdout and Shell controls have separate run-owned captures.
   Each control-producing command redirects directly to its capture; Execute's
   inherited stdout is not used. Completion flags follow those command writes.
   The selected Stack and native deadline remain explicit; the helper refuses
   native deadlines above 90 seconds. A launch acknowledgement is not completion.
4. Require the terminal completion flag, exact native lines, RC0, Stack query,
   file checksums and exact namespace. Retain the complete file bodies for local
   custody. Returning success does not delete anything or release ownership.
5. In a separate operation, require the caller's independent idle/custody check,
   recheck every exact file and delete only those names, then the empty directory.
   Independent final absence, transport restoration and guard release remain
   separate caller responsibilities.

There are no SCRIPT requests, `T:` writes, recursive deletes, background retry,
automatic recovery, network connection or emulator lifecycle operations in this
transaction. A failed exchange, cancellation, output check or local record write
permanently fails that transaction; subsequent calls cannot resume or relaunch it.

`native_file_protocol.py` validates raw bridge replies. It rejects partial or
inconsistent directory pages, duplicate names, oversized data and wrong paths or
offsets. File reads check ordinary-file metadata before and after and require the
declared number of bytes plus explicit EOF. This rejects premature EOF, including
the old daemon's failed-seek-as-EOF behavior. The actual wire order is
`FILE|path|offset|length|hex`; the installed parsed dictionary labels these two
numeric fields in reverse. No lossy parsed dictionary is used here. An explicit
SCRIPT naming/acknowledgement helper is retained for inspection only; it performs
no operation and is not used by the new runner.

`native_guarded_exchange.py` observes raw lines through the existing connection's
`on_rx` callback, installing its observation before `conn.send`. It preserves the
old callback and uses the shared guard's checked send leaf. It rejects duplicate
replies, overlapping observers, lost admission and timeout. It must be installed
only inside a newly reviewed private guarded service; it is not an independent
transport or a substitute for shared reservations and live target checks.

One bounded, printable ASCII `ERR|` reply now raises `BridgeReplyError` with
the exact response attached. Input and execution transactions save it as a
numbered `input_error_reply` or `error_reply` in the private local dispatch
ledger before propagating failure. The generic message omits raw target data.
Ambiguous, duplicate or malformed replies are not promoted to exact error
evidence. Failure remains sticky: no retry, cleanup or deadline extension is
granted. A ledger-write failure also leaves ownership retained.

The new host matrix injects an error at each of all 135 exchanges across input
preparation, execution and both settlements through the real raw callback
adapter. Every case records the matching dispatch number, stops further sends
and preserves unrelated files. All 92 relevant host groups pass (12 raw exchange,
28 protocol, 33 execution, 19 input). This diagnostic change has not yet run on
either target.

The preceding revised Load emulator run completed both copy stages and reached
publication, then failed on a completion-file bridge response whose exact text
was lost. Its failure remains preserved. Exact owned-process cleanup, original
launcher restoration, independent absence and public release passed, archiving
all three holds. It does not qualify the revised native candidate.

## Evidence and limits

70 host tests pass across raw protocol parsing, transaction fault injection,
guarded exchange observation and the existing native capture contract. The
transaction tests fail every exchange in turn and verify no subsequent request,
retry or automatic cleanup; unrelated model files remain unchanged. These are
models, not evidence of real bridge, DOS, native ABI or hardware behavior.

The first private guarded-service integration timed out at its 60-second
preparation call while uploading the 424,732-byte candidate in 2,048-byte pieces.
The retained ledger has 104 dispatches and 103 replies, ending at an unconfirmed
write at offset 206848; it contains no LAUNCH. Cancellation and a guard-journal
lock refusal left the existing reservation, an interrupted operation and two
HOLD entries. Recovery requires separate approval and verified release. This is
a failed transport qualification, not a native fixture failure or pass. The
same candidate's earlier successful Load test used a different transport route.

Uploads now calculate their payload from the pinned daemon's 8,192-byte input
array, its terminating NUL, the complete command header and its 4,096-byte decoded
write array. The maximum offset is budgeted before chunking. For this candidate,
the binary upload decreases from 208 requests to 105. Every acknowledgement,
checksum, script readback, ownership check and time limit remains in place.
Host tests reconstruct exact bytes through the 2 MiB input limit, include the
longest permitted path and offset digit boundaries, and fail each exchange in
turn without further requests or cleanup. A short write acknowledgement is
rejected immediately. Fewer requests do not establish throughput or completion
within the preparation deadline; this changed uploader is Amiberry NOT RUN.
The original service used its frozen source copies throughout that run and its
approved recovery. Recovery subsequently stopped only the two owned processes,
restored the saved launcher files and removed the exact staging copy. Independent
checks and public release archived the interrupted operation and both HOLDs.
This does not convert the failed transfer into a pass.

An optional **emulator-only copy mode** avoids uploading the executable over the
command channel. Its source must be the caller-pinned file under the exact run's
`Dev:Tests/PT24G-<identity>/` directory. The caller must still verify the shared
mount and host file identity. The transaction checks source CRC/length, uploads
a small owned copy script, launches it once, verifies its bounded completion and
exact output, and checks both the RAM copy and source again. The source is never
changed or deleted by the transaction. Three additional RAM files belong to the
same run and are included in custody and exact settlement. Native execution
still uses the RAM copy and the original 90-second deadline. No SCRIPT helper,
hidden `T:` file, second transport or deadline extension is introduced.

Both upload and copy modes have host failure injection. Copy-specific tests
cover every failed exchange, wrong source, failed copy, uncompleted copy and
unchanged source after settlement. Malformed-path tests also found a redundant
nested repetition in the original path expression that could stall rejection.
The equivalent segment-based expression now completes a separately timed
malformed-input regression. This mode does not assume the real A1200 has the
emulator's shared mount and remains physical unsupported.

The first copy-mode Amiberry attempt acknowledged the staging script and returned
the exact STAGED flag, but declared the expected progress log empty in both file
metadata observations and its read/EOF replies. It stopped before the post-copy
candidate checksums or native launch. Copy integrity and native execution are
therefore unqualified. Its ledger contains 22 dispatches and 22 replies; the only
LAUNCH was the staging script. Its separately approved recovery stopped the two
owned processes once, restored the saved launcher files and removed the exact
staging copy. A transient Darwin
exiting state was initially refused; separate positive process absence preceded
completion of restoration with no repeated signals. Independent checks and public
release preserved all three HOLDs in the archive. No test ran during recovery.

The changed scripts now redirect each Echo and Stack query directly to its own
control file, closing each command's file before the terminal flag. The host
bridge model reads the uploaded scripts and discards inherited stdout, rather
than manufacturing expected logs. It reproduces the former empty-log/valid-flag
combination, passes the direct captures and rejects an injected empty stage log
before native launch. This model is not a Shell interpreter and does not establish
why the live log was empty. That historical host result preceded the separate
direct-capture run below. The original service sources were preserved throughout
its failure and recovery.

Directory replies are not atomic snapshots. The historical daemon does not check
`IoErr()` after `ExNext()`, so even a syntactically complete response cannot prove
that an arbitrary directory had no enumeration error. Fresh namespace creation
uses positive CreateDir success; it does not depend on that negative inference.
Source availability does not prove that the physical running daemon matches it.
The upstream receive layer also cleans lines before `on_rx`; this helper does not
claim raw serial integrity or authentication.

Before activation: bind a private guarded service and complete custody observer;
test the exact candidate and adapter in the shared 030 emulator; verify normal
cleanup and independent release; then obtain fresh physical identity/ownership
and an accessible AmiConnect session. Device/audio/timing/listening acceptance
remain separate from this CPU/software test route.

## Direct capture and host cleanup correction

The direct-capture Amiberry run completed staging and the exact 424,732-byte
Load fixture: RC0, Stack524288, all ten expected lines, 355 native-output bytes
and CRC059CB441. All eight files were backed up with verified checksums. The
host runner then overwrote its nested safety-check function with a checksum
dictionary. The next call raised TypeError before the task observation,
settlement or disconnect. This qualifies the observed native completion, not
a successful complete run or physical execution.

Separately approved recovery stopped only the two verified owned processes,
removed the exact staging copy and restored the saved launcher files. Independent
absence/restoration checks and public release archived the original HOLD. The
failed source, output, custody records and failure remain preserved. No test
was rerun as part of recovery.

A separate runner correction names the dictionary `native_checksum`, preserving
the callable safety check through cleanup. The full guest-phase host model now
runs the actual runner, registered private service and raw bridge transaction
together. It reproduces the old failure, exercises successful eight-file
settlement, namespace absence, staging removal and disconnect, and fails each
of the nine outer calls in turn without follow-up. Active-task, altered-custody
and nonzero-native-result cases retain the required files. Six test methods
cover fourteen scenarios; four service tests and the offline admission checks
also pass. Network, process and guard effects are forbidden in these models.
This is host integration evidence, with unchanged 90/140/250-second limits.

The separately prepared corrected runner then passed once in Amiberry with the
same candidate bytes and all ten native lines. Exact eight-file settlement,
independent RAM namespace absence, host staging removal, task observations and
disconnect completed. The owned emulator stopped, saved launcher files were
restored, and a separate observer verified capture/custody/settlement and current
absence before public release. The disconnected local service was then stopped
and an independent final idle check passed. No HOLD or ownership remains.

This completes the emulator copy route only. The physical upload path, full
editor frontend, hardware audio and timing acceptance remain separate. The
original failed runs retain their outcomes; recovery is not counted as a test.
The first read-only post-run observer used the wrong saved absence-record shape
and stopped before process checks or release. Its evidence is preserved; the
corrected schema was checked offline before completing observation.

## Smaller sampler direct-upload qualification

A separate 166,396-byte sampler18 fixture now passes the direct-upload route in
Amiberry with RC0, Stack262144 and eight expected lines (1,159 bytes, CRC808D90D6).
It uses five declared RAM files, without a shared-mounted copy. Exact settlement,
independent namespace absence, staging cleanup, disconnect, emulator shutdown,
restoration, separate public release and final local-service absence all passed.
The 90/140/250-second limits are unchanged.

The fixture tests ordinary-memory sample editing, import/export, 24-bit history,
budgets, retention and serialized persistence. Its source is the existing
sampler18 configuration at 78d6871, with typed replacement/action disabled.
Seventeen primary source files still match current code; the core difference
contains two checks gated by the disabled typed-action macro. This is a primary
source audit, not a complete transitive preprocessing equivalence claim. The
current sampler separately passes ASan/UBSan. Newer document-action integration
is not qualified by this fixture.

This establishes direct upload for the smaller candidate only; the larger Load
candidate's upload deadline is not qualified. The separately tested physical binding and its first transfer attempt are
recorded below. That attempt did not launch the native fixture; the later
incremental physical pass is recorded separately at the end.


## Physical sampler transfer attempt

The same sampler candidate was admitted through a physical-profile service after
18 full-phase host scenarios, four service tests, two orchestration tests and
offline construction/refusal checks passed. The real A1200 reported a 68030,
Exec 47.13, PAL and sufficient free Fast memory. Its installed bridge matched
the pinned 150,192-byte reference (CRC D75C2355). Safari was authenticated and
remained View Only.

Preparation hit the unchanged 60-second upload limit before any native launch.
The saved exchange ledger confirms 142,380 payload bytes acknowledged; the next
4,068-byte write has no confirmed reply. No executable checksum, native result,
settlement or cleanup is inferred. The run retained HOLD and its operation;
further physical calls stopped. The original failure and a separate, bounded
recovery scope are preserved. Recovery was subsequently approved: the exact
owned local service was stopped, process/transport absence verified, and the
exact host staging copy removed. After the Mac was unlocked, authenticated
Safari inspection found only the expected partial NativeTest, 146,448 bytes.
That exact file was deleted, its directory was positively observed empty, and
only that directory was then deleted. A separate RAM parent listing contained
only Clipboards, T and ENV. The temporary Shell was closed and browser control
returned to View Only. Independent local checks and one public release archived
both original HOLDs and the interrupted operation; final idle verification passed.
No rerun or reboot occurred. A reporting check initially looked in the wrong
archive directory after release; the actual history archive was independently
verified without repeating release. That failed check is preserved.
This original physical sampler attempt remains NOT RUN, with transfer INCOMPLETE.

This failure does not invalidate the completed Amiberry evidence, but that
evidence cannot qualify physical upload throughput. No audio, timing or AmiGUS
operation occurred.


## Bounded incremental direct upload

Direct upload can now use a single live transaction split into calls of at most
four acknowledged frames (16,272 payload bytes). The caller supplies the exact
next byte offset. A stale offset, failed acknowledgement, cancellation, lost
ownership or failed evidence write stops the transaction without replay or
resume. The transaction deadline includes time between calls. The complete
candidate checksum and script readback still precede execution; native output,
custody, exact settlement and independent absence checks are unchanged.

Host checks pass: 11 focused incremental tests, the existing 70 protocol tests,
seven private service tests, and seven full-phase tests covering 34 scenarios.
The complete host model covers all 23 outer call failures and incorrect progress
replies. A synthetic slow transfer exercises a total above 60 seconds without
resetting its 140-second deadline; this is not measured hardware throughput.
The prepared runner retains native90/guest140/whole250 limits, using 20-second
begin/chunk calls and a 60-second final verification call. Host models alone
do not qualify the target integrations recorded below.


The incremental integration subsequently passed one exact 68030 Amiberry run:
166,396-byte sampler18 candidate, Stack262144, RC0 and all eight native lines
(1,159 bytes, CRC808D90D6). All eleven upload batches, full candidate verification,
five-file custody and settlement, independent RAM absence, staging removal,
disconnect, emulator shutdown and original launcher restoration passed under
the unchanged limits. A separate observer, public release, owned service stop
and final independent idle check also passed. This qualifies the changed
transfer route in Amiberry; physical acceptance is recorded separately below.


## Incremental sampler physical acceptance

The exact Amiberry-qualified sampler18 candidate passed once on the real A1200:
166,396 bytes, SHA256
`84c405dfed6996a4c13f476eb5cf457b4036e9a603f46ad52b5df38b2900e769`,
CRC A0A3172B. Fresh identity reported CPU68030, Exec47.13 and PAL; the installed
bridge matched the pinned reference. All eleven upload batches and final
candidate/script verification passed before the single native launch.

Native RC0, Stack262144 and all eight expected lines were confirmed (1,159
output bytes, CRC808D90D6). The fixture covers ordinary-memory 24-bit sample
editing and history, conversion, loop/slice edits, IFF/RAW import/export,
budgets, retirement inventory and serialized save/reopen behavior. Its existing
source/configuration limits above remain in force: this is sampler functional
acceptance, not newer typed New/Load integration or the complete editor.

All 23 outer calls completed with the original native90/guest140/whole250 limits.
Five-file custody, exact file/directory settlement, a separate RAM parent
listing, host staging removal and disconnect passed. An independent observer
verified the result before public release; the exact disconnected local service
then stopped normally and a separate final idle check passed. Safari remained
authenticated View Only throughout; browser control was never acquired. No
recovery, retry, reboot, audio or AmiGUS operation was performed in this run.

Physical Fast/Chip placement, DMA or device voice-stop, AmiGUS RAM capacity and
upload ordering/completion, exact musical scheduling and listening quality are
not established. The earlier incomplete physical transfer and its approved
recovery retain their original outcomes. All release evidence is preserved;
the shared guard has now reached its 64-archive bound and refuses a further
reservation until separately reviewed maintenance is complete.


## Approved harness capacity maintenance

After the physical sampler run, the shared guard reached its 64-record limit.
The user approved a bounded increase to 128, including leaving dormant Codex
tool adapters untouched. The exact guard constant and its matching documentation
were installed after idle/ownership/hash checks. A separate verification confirmed
all 64 original archive bodies, the state bytes and fence were unchanged. No
target call, process signal, state reset or archive deletion was involved.

The private candidate passed 88 distinct host checks (83 existing plus five
capacity checks). The first invocation retained one missing copied-wrapper error;
that one test passed after copying the unchanged fixture. New tests must load
freshly qualified services; existing loaded adapters are not treated as updated.
The 128-record bound, 64 KiB state bound and 32-HOLD bound remain finite.


## Exec Fast/Chip cache fixture preparation

The existing native selective Paula cache wrapper can now select fixed result
lines with `PT_TEST_NATIVE_MEMORY_FIXED_MARKERS=1`. It still checks the real
Fast-only pool, zero-budget refusal, TypeOfMem on allocations/releases, zero
remaining owned bytes, and the 256 KiB Fast reserve. Default diagnostic output
retains its memory totals and allocation count. This changes test reporting only.

Host checks exercise both output modes through the actual cache wrapper with
injected Exec memory calls under ASan/UBSan. Existing sampler/cache and separate
allocator models also pass. These models do not establish physical placement.
A fresh 21-unit native build and exact 28-input linker-map audit pass; the fixed
150,436-byte candidate subsequently passed exact Amiberry and real-A1200 qualification. The initial preprocessor timeout on
iCloud-backed configured SDK headers is preserved separately. The successful
build uses an unchanged resident copy of 18 configured NDK headers, without
reconstructed headers or another SDK. Amiberry must qualify the exact candidate
before a separate physical memory-placement run. No DMA or audio output is
part of this fixture.


The exact cache candidate exited RC0 on Amiberry and then on the real A1200,
with all four expected lines and unchanged 90/140/250-second limits. Separate
observers verified cleanup, ownership release and idle resources after each run.
The physical fixture checks genuine Fast and Chip allocation placement, bounded
cache conversion, stale versions, pressure and retained leases. Safari remained
authenticated View Only. Earlier local preparation refusals are retained separately.
These results do not establish full-editor integration, DMA/audio, exact musical
scheduling, AmiGUS sample-RAM behavior or listening acceptance. See the scoped
exec-cache-native-preparation evidence for candidate and verification hashes.


## Focused typed document publication diagnostics

The isolated New/Load publication entry reuses the actual action, checked-owner
and bridge fixture without its broader CLI. Load now checks byte-for-byte project
encoding after final-address owner publication. Host sanitizer checks pass both
New and Load and the full existing action regression suite. Production warnings
remain strict; unused shared test helpers are suppressed only for the fixture.

A fresh 84-unit native Load build passes all 253 compiler/link commands and the
91-input map audit. Function/data sections and linker garbage collection reduce
the candidate to 221,108 bytes; fixture-only whole-program localization removes
uncalled test entry points. All production action code and assertions remain.
The earlier missing-header failure and larger successful build are separate
records. This candidate is target NOT RUN until exact Amiberry qualification,
followed by a separately admitted A1200 test. It supplies no full frontend,
actual output, IRQ, exact scheduling or listening acceptance.


The exact 221,108-byte Load candidate was subsequently attempted once in
Amiberry. Fourteen upload batches and final preparation verification completed,
but the unchanged total guest-phase deadline expired during the execute request.
Native completion, output capture and cleanup were not confirmed. Ownership,
two cancellation HOLD records and the in-flight obligation remain retained.
A concrete run-owned recovery is prepared and awaits human approval; no retry
or deadline change occurred. Physical qualification remains NOT RUN.

The separately linked New variant is 213,052 bytes and remains never executed.
It reuses the exact verified 83 production objects with a separately compiled
fixture selector; it inherits no runtime acceptance from the Load attempt.


The human-approved recovery subsequently completed: only the two exact owned
processes were stopped, matching staging was removed, and original launcher files
were restored. Separate observation, one public release and independent final
verification passed. Both original HOLD records and the in-flight obligation are
preserved in the release archive. No test was rerun and native completion remains
unconfirmed. A separate erratum corrects an inherited narrative label in a cleanup
fact; it does not alter the failed result, signed records or acceptance boundary.


A distinct corrected-copy qualification then used the unchanged Load binary.
The current explicit-file progress script passed staging, including the exact
flag, log and source/destination checksums. This qualifies the tested copy step
only. Native execution subsequently reached its unchanged 90-second completion
limit without confirmed output or cleanup. The new run retains three HOLDs and
requires a separately approved recovery. The earlier recovered direct-upload
attempt remains separate, and no physical Load test has run. A transfer-only
explanation is insufficient; the next native diagnostic must locate execution
progress within the existing bounds, without treating a timeout as a pass.


The corrected-copy run's separate human-approved recovery is complete. Exact
owned processes and staging were removed, saved launcher files restored, and
independent observation, public release and separate post-release checks passed.
All three original HOLD records remain in the archive. The native timeout is
still unconfirmed runtime acceptance; no rerun or physical test was performed.

## Bounded diagnostic progress and continuing recovery authority — 15 October 2026

`execute(diagnostic_progress=True)` requires a plan with `progress_sidecar=True`.
It records at most the final 256 bytes of that run's separate `progress.log` every
20 seconds while completion is pending. The file is created before launch and
closed after each fixture update; the active stdout handle is never inspected.
Every metadata/read request spends the original native deadline. These snapshots
are unconfirmed observations, never a return code, completion, custody or cleanup
proof. A failed read or malformed/short suffix ends the transaction. Even every
PASS line cannot replace the completion gate. Copy mode accounts for all nine
files, including final sidecar custody and exact cleanup; the default path is
unchanged.

The publication fixture reports 17 internal boundaries, plus BEGIN and final
PASS, with `PT_DOCUMENT_ACTION_TRACE=1`. It reuses the typed New/Load assertions
and byte-exact project roundtrip. The 224,372-byte trace build retains the same
83 production objects. It adds no product output callback, DMA, device write,
musical timing tolerance or frontend integration. Host sanitizer checks cover
New, Load and the complete traced Load, including its final closed sidecar.

The first trace attempt stopped during staging because the journal mutex was
busy. A separate stdout-progress attempt launched but its first metadata request
failed; no progress or completion was captured. The first sidecar attempt also
stopped during staging on a proven journal-lock collision. All three runs were
separately cleaned, independently observed and publicly released under the
standing authority below; failed evidence remains preserved.

A narrow shared-harness fix now waits at most 0.5 seconds to acquire its host
journal mutex, then rechecks the lock inode and private-file properties before
reading and validating current state. It never retries an operation. The command
and transport locks remain nonblocking. All 67 existing guard/access tests and
six new contention tests pass, including new HOLD visibility, sustained-lock
refusal, replaced inode, changed permissions and non-contention I/O failure.
Installation preserved originals at verified idle and left loaded idle services
untouched. Exact native qualification remains a separate gate.

The user's current instruction authorizes continuing development, testing and
routine recovery of positively verified run-owned resources without repeated
approval requests. Recovery still requires exact process identities, saved
originals, exclusive ownership, bounded cleanup, an independent observer and
public guard release. A failed transaction is never resumed or relabelled as a
pass. Changed ownership or unverifiable restoration is outside routine recovery.


The unchanged 224,372-byte sidecar candidate subsequently passed one fresh
Amiberry run through the corrected harness: RC0, all 19 lines, byte-exact project
roundtrip, nine-file custody and exact cleanup. Native90/guest140/whole250 limits
were unchanged. Separate release and final idle observations passed; no owned
resources remain. The progress snapshots localize the long phase after document
publication but do not establish a function-level performance cause. Real-A1200
qualification and full-frontend integration remain separate and untested for this
candidate. The scoped evidence includes the exact shared-journal source patch.

## RAM staging preparation and physical Load limit — 15 October 2026

The same Load candidate subsequently transferred to the A1200 and launched once,
but the outer 140-second guest deadline expired shortly afterwards. Its native
completion and return code are unconfirmed. Saved host ledger file times show
about 93 seconds between the 59 WRITEFILE dispatch/reply pairs; these times also
include host validation and recording, so they are not a network benchmark.
The failed transaction is preserved and cannot be resumed. Local service and
host staging cleanup passed. Physical task/file observation and independent
release remain pending because interactive Safari input is unavailable; Safari
was returned to View Only. No physical Load pass is claimed.

The copy helper now additionally accepts exactly
`RAM:PT24G-<same first 16 run-id digits>-in/NativeTest`. This disjoint source
namespace must already belong to the caller through a separately qualified,
bounded transfer. A matching name or checksum alone grants no ownership.
Shared `Dev:Tests` behavior and default direct upload remain unchanged.

The source is checked before and after one copy into the fresh execution
namespace. The helper only reads the source through checksums; it never writes,
deletes or adopts it. The caller retains responsibility for exact staging and
archive custody/cleanup after native execution. A source change, wrong run,
destination alias, failed copy, ambiguous reply or timeout refuses progress.
There is no implicit reconnect, retry, deadline reset or installation action.

All 33 transaction host tests pass, including failure injection at every
exchange of the RAM-copy route, source preservation, prelaunch wrong-byte
refusal and exact namespace restrictions. The four publication sanitizer forms
now include traced New and traced Load with separately closed progress files.
The New trace cross-build is 216,360 bytes and remains native NOT RUN.

The new RAM-copy route remains **Amiberry and physical NOT RUN**. Before use,
finish the held physical recovery, qualify source transfer and the exact changed
route in Amiberry, then admit a new physical operation. Transfer is a separately
bounded preparation operation; execution retains native90/guest140/whole250.
Host models and an archive extraction check do not establish physical transfer
speed, native completion, frontend behavior or audio/timing acceptance.

### Owned RAM input preparation — host implementation

`tools/owned_native_input.py` now supplies the missing Dev-to-RAM preparation
step for an emulator qualification of that route. It takes the exact execution
plan, a separately owned same-run Dev source and a third distinct command ID.
It creates the separate `-in` directory positively, refuses an existing directory,
and performs one explicit copy. It checks the source before mutation and again
after the copy, verifies the copied candidate, and retains four exact files:
`NativeTest`, `Prepare-input`, `ready.flag` and `prepare.log`. It does not launch
the native fixture, connect a service or modify its Dev source.

The input remains owned throughout the execution transaction. Separate settlement
requires the reviewed caller's positive execution-idle and saved-custody proof,
then rechecks every input file and the source before deleting the four exact
files and empty input directory. An unknown member, altered file, failed record,
ambiguous reply, cancellation or timeout retains the failed stage for recovery;
the failed operation cannot be replayed. Native RC0 or an input-ready flag alone
does not permit input cleanup. Public guard release and independent final
absence remain separate caller obligations.

Preparation spends at most 60 seconds, with at most 30 seconds waiting for the
copy to complete. The stage's entire lifetime, including reader use and
settlement, is capped at 250 seconds from construction. No deadline restarts
after preparation. These are ceilings within the caller's unchanged
native90/guest140/whole250 limits, not extra time allowances. The staged route
therefore requires the caller to start both transactions inside the same outer
guest window and retain its original deadline.

Host qualification passed 17 new input groups plus the 33 existing execution
groups. The combined model creates RAM input through the real helper, runs the
real execution transaction, settles execution first and then settles input.
It injects failure at every exchange, including cleanup, and checks that the
Dev source and unrelated files survive. Additional cases cover changed input,
unexpected directories, stale ownership assumptions, cancellation, late or
timed-out settlement proof and unchanged deadlines.

A private service integration also passed eight input cases and seven existing
service cases. It binds both transactions to the same lease and lifetime, saves
the input custody, and checks a separate execution-absence record plus fresh
task/custody proof before input deletion. Six further private guest-phase groups
passed with a 224,372-byte model payload, including interruption at all 13 outer
call boundaries. The guest phase independently checks the four input custody
files, obtains a fresh task observation, settles execution before input and
confirms both RAM namespaces absent before removing the Dev source. The full
run entry remains unconditionally inhibited. Original lifecycle source is
unchanged; its qualification and fresh exact-candidate admission are still
required. No service process or target connection was used by these tests.
This helper implements neither MiniFTP nor archive extraction; those need
separately verified physical staging and cleanup.

**Amiberry and real-A1200 results for this new input helper remain NOT RUN.**
The earlier physical Load run still requires scoped recovery and independent
release. The latest Safari check confirms the Mac is locked. Host progress does
not clear that recovery hold or establish native timing, audio or listening.


### Owned-input lifecycle and saved cleanup checks — HOST packet

The next inactive emulator packet now validates the exact current Load candidate,
its same-run RAM input path, distinct copy commands and both complete recovery
inventories. It records the four input files separately from the nine execution
files, including in the proposed cleanup witness. Selected service input pins
now include the input helper. No old service identity, admission, process ID or
release fence is inherited.

Thirteen lifecycle/contract host groups pass using the actual orchestration
with private files and modeled process/transport effects. They cover original
file existence, bytes and metadata restoration; original absence restoration;
changed-process refusal; changed restoration bytes; retained custody after guest
failure; actual asynchronous cancellation; unchanged guest140/whole250 bounds;
and rejection of altered input/execution/source inventories. The six guest
integration groups also pass, including all 13 outer-call timeout positions.
An independent saved-evidence observer checks input preparation, all four
custody bodies, the separate execution-absence record, task/owner proof, input
settlement and final absence of both RAM namespaces. It rejects either retained
namespace, each altered custody file and an altered owner. It grants no public
release or live ownership authority.

Seventeen service/factory groups pass: eight input cases, seven existing service
cases and two factory checks. Factory tests model admission and forbid process,
network and guard-journal operations; the normal logging setup may append to the
existing host log. They do not start a service or qualify a live connection.
The independent observer is also inactive until current admission and actual
fences are bound. Native90, guest140 and whole250 remain unchanged.

These 36 host groups qualify separate lifecycle, guest and service models.
They do not establish a complete live run. The exact Load candidate is unchanged;
no production C or frontend behavior changed. The new packet remains inhibited
pending the existing physical recovery and independent release, fresh live
checks and service identity/admission. Its private disk pin must be refreshed
from current evidence before admission. Amiberry and A1200 qualification of
this new input route, plus physical MiniFTP/archive staging, remain NOT RUN.

### Document snapshot comparison — HOST and native build

The document-action snapshot comparator now checks four longwords per loop.
It still compares the complete object representation, including padding, with
bounded word and byte tails and a byte fallback for odd addresses. Ownership,
mutation detection, preparation work64 and native90/guest140/whole250 limits are
unchanged. This reduces loop bookkeeping; no target speedup is claimed yet.

A private instrumented HOST Load diagnostic counted 501 replacement phase3
steps. Its action phase4 comparator counters reached 2,044 calls and 266,807,408
requested bytes by commit (40 calls and 5,221,280 bytes were already recorded at
publication). These are cumulative requested spans in the host layout, not bus
traffic or Amiga timings. The instrumentation is absent from production code.

The complete document-action sanitizer regression passes, including cancellation,
alias refusals, changed delegated snapshots and final-address publication. The
actual comparator also passes a memcmp oracle over 1,296 length/alignment pairs
and 44,112 single-byte differences, with exact allocation tails and boundaries
around 16 bytes, 64 KiB and 1 MiB. The test's stale short trace expectation was
updated to the already-existing diagnostic stages. All four focused New/Load
forms pass separately with ASan/UBSan and strict compiler warnings.

The pinned 68000 compiler emits four inline longword comparisons per full block
and reports the same four-byte snapshot-function stack use. Both complete native
fixtures link with 83 production units and 91 audited link inputs. The revised
Load is 224,436 bytes, SHA-256
`32e3c750f141c13baaa4dc115013ea8c9459dd009e8993e79fbf93d69322860a`;
New is 216,424 bytes, SHA-256
`81a828ccf65ec8dee84051be7e556a453b85f0035e22b954effee162b34345ac`.
Both revised candidates subsequently passed their separate Amiberry fixtures
through the owned RAM input path. Exact revised A1200 runs remain NOT RUN.
Historical candidate evidence stays bound to its original bytes; the old
physical timeout remains a failure despite late recovery observations. Its
cleanup and independent ownership release are complete.

## Separate single-file input upload — 15 October 2026

`InputPlan(execution, None, None)` prepares the same run's fresh RAM input
namespace directly from the frozen candidate bytes. It refuses an existing
directory, uploads only `NativeTest`, checks its checksum/size and exact one-file
inventory, then retains custody through execution. This path has no preparation
script, launch command, FTP server or archive extractor. Its maximum candidate
size is 512 KiB so upload, custody and settlement fit the fixed256-exchange bound.
The existing Dev-copy plan and four-file inventory remain unchanged.

The default preparation ceiling remains60 seconds. Only upload mode can select
an explicit ceiling up to120 seconds. That is bounded within the same original
stage lifetime (at most250 seconds); ready state, reader waiting and settlement
never reset it. This does not extend any native90, guest140 or whole250 execution
budget. A caller must account for the separate transfer and retained source
before admitting execution and refuse insufficient time. The guarded integration below accounts for preparation before execution;
the helper alone opens no connection.

Independent execution-idle and saved-custody proof are required before exact
input deletion. Source mutation, extra files/directories, failed acknowledgments,
expiry and uncertain readers retain ownership without retry. Ten new host groups
pass, including every one of151 combined-model exchange failures, maximum-size
accounting and explicit deadlines;19 copied-input and33 execution regression
groups also pass. The subsequent emulator qualification below covers native staging; physical
transfer remains NOT RUN for this mode.


### Guarded upload integration and Amiberry qualification

A separate upload preparation call now runs before the guest execution phase,
inside the same owner operation and original250-second overall budget. Upload
preparation is bounded by120 seconds; native90 and execution/settlement140 are
unchanged. The service creates input and execution transactions together, retains
the same deadlines and refuses repeated input preparation. The execution phase
requires the exact saved ready result and custody, then checks the RAM source
before copying. Separate task, custody and execution-absence proofs precede
one-file input cleanup and independent absence of both RAM namespaces.

Fifty-five host groups pass across lifecycle, full guest, service and factory
models. They cover all14 upload-route outer-call timeout positions, existing
13-call copy behavior, changed owner/custody, missing or repeated preparation,
and cancellation by the original overall timeout across the stage boundary.
One initial test used an overly short wall-clock delay and expired at the wrong
boundary under load; the corrected test deliberately expires the same original
timeout context after execution starts. Earlier failure logs remain separate.

The exact216,424-byte New fixture passed once in Amiberry through this route:
RC0, Stack262144, all19 expected lines,767 captured bytes/CRC1B45D25F. Exact
one-file input and nine-file execution cleanup, saved launcher restoration,
separate observation, public release and final idle check passed. No owned
emulator/service, locks, reservation or recovery hold remain.
See [compact evidence](../evidence/enhanced-editor/document-upload-input-emulator/result.json).
This qualifies the emulator RAM transfer and synthetic New publication flow.
The real-A1200 route, full frontend, playback, timing and listening remain separate.

The separate224,436-byte Load fixture subsequently passed through the same
upload route in a fresh emulator run: RC0, Stack262144, all19 lines and769 output
bytes/CRC4EC504EA. All55 host groups were rerun for its exact packet. Exact input
and execution cleanup, independent observation, public release and final idle
checks passed; no test resources remain owned.
See [Load upload evidence](../evidence/enhanced-editor/document-upload-load-emulator/result.json).

The physical counterpart is prepared with44 passing host groups plus an offline
construction check. Its18 outer-call timeout positions include connection,
CPU/OS/memory and installed-bridge checks before upload; its original overall
budget spans upload and execution without resetting. Exact physical New transfer,
execution and cleanup remain NOT RUN, and entry stays inhibited pending fresh
Safari access and live admission. No live service or lease was created for that
prepared physical packet.
See [physical preparation evidence](../evidence/enhanced-editor/document-upload-physical-host/result.json).


## Explicit external-transfer admission — 16 October 2026

`InputPlan(execution, None, None, external_transfer=True)` identifies a separate
MiniFTP/archive handoff. `prepare_transferred(observer)` requires the trusted
caller to independently record fresh exclusive namespace creation, exact transfer
and extraction, completed transfer and current ownership. The callback receives
the exact plan and manifest and must return literal `True` before any protocol
read. A matching pathname or checksum alone never grants ownership. The helper
then checks the exact one-file inventory and frozen candidate checksum/size,
without writing, creating a directory, launching or deleting.

Normal `prepare()` refuses this explicit mode; existing upload/copy behavior and
refusal of pre-existing RAM remain unchanged. Missing, false, late, cancelled or
unrecordable proof, changed contents, extra members and transport failures latch
FAILED and retain files. Original preparation/lifetime deadlines, staging and
exchange bounds remain. Execution-idle/custody settlement and independent release
are still required before input deletion. Archive and extraction artifacts remain
the caller's separate cleanup obligations. This callback is a caller contract,
not authentication supplied by a checksum or this helper.

All41 HOST groups pass:12 new external-admission groups,10 existing upload and19
Dev-copy regression groups. Models forbid sockets, subprocesses and signals.
The new seam has not yet been integrated into the live owned service or tested
with MiniFTP, Amiberry or the A1200. Existing exact native component passes use
the previous upload route and do not qualify this new route.
See [external-admission HOST evidence](../evidence/enhanced-editor/rolling-native-entry/external-input-host.json).
