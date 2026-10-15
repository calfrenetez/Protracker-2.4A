# Prepared RAM-only native test route

Status: **HOST-qualified components; the first private Amiberry integration
stopped during preparation before native launch. Physical execution NOT RUN.**
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

65 host tests pass across raw protocol parsing, transaction fault injection,
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
malformed-input regression. Copy mode is **Amiberry NOT RUN; physical unsupported**
until a separately admitted run qualifies it. This mode does not assume the real
A1200 has the emulator's shared mount.

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
