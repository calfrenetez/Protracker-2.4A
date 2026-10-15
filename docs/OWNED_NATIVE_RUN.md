# Prepared RAM-only native test route

Status: **Exact Load fixture and full owned-RAM copy/capture/cleanup route pass
in Amiberry. Independent restoration, release and final idle checks pass.
Physical execution NOT RUN; the real-machine upload route is still separate.**
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
recorded below; physical native execution remains unqualified.


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
The physical sampler result is NOT RUN, with transfer INCOMPLETE.

This failure does not invalidate the completed Amiberry evidence, but that
evidence cannot qualify physical upload throughput. No audio, timing or AmiGUS
operation occurred.
