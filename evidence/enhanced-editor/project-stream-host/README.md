# Bounded enhanced-project save

Host ASan/UBSan project and stream suites pass, including exact original-encoder
bytes/CRC for mixed precision masters and metadata, sink refusal, source alias
and invalid-project refusal, allocation failure, destination protection, short
or overflowing producers, verification mismatch, interrupted/short reads and
staging cleanup. One fixed7184-byte host heap workspace plus core block1024.
Sample export and existing safe-save regression suites pass.

Full pinned native build passes; latest Exec fixture compiled directly after
additional refusal assertions. Emulator attempt was NOT RUN: shared_guest
identity guard raised RuntimeError('Not the shared 030 emulator') before Guest
construction. No guest files or commands, no reset/relaunch. Coordination was
explicitly released; AmiConnect confirms availability remains unverified.
No runtime, physical storage, audio or visual UI acceptance is claimed.
