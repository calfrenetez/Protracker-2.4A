# Selective Paula sample preparation

Host ASan/UBSan: three focused groups passed both in the working checkout and
in the isolated indexed candidate (playback preflight, sync, channel behaviour).
The candidate excludes unrelated uncommitted display work. All151 recorded source
hashes match the index; all7 built binaries match their manifest.

Native shared030 run `paula-cache-1790561279907765000`: all5 fixtures RC0.
New coverage: 255 stored masters, unused stereo24/48k omitted from both Fast
staging and Chip caches, active unsupported-reference refusal preserving old
playback, live-sync stop/release, and byte-exact enhanced save. Existing effects,
cache reuse/pressure/failure, source preservation and timer ownership pass.
CIAA fallback execution remains NOT RUN because Workbench owns timerB.
Runner cleanup plus independent subsequent locked checks confirm running guest,
all4DMAoff and exact run/launcher absence. Explicit release sent to AmiConnect.

PTPaulaTest125420 bytes, SHA256
`619a3decfca096ff81c6a0f2d25132d6d58ec8fe6e1678191080aef6a492ad45`.
PT24GEdit263848 bytes, SHA256
`db6c7145f3b6ed1785b4f200bf7c598631ec1362c4899e0c5d0276f23231f510`.

Full editor run `paula-highres-ui-1790561413163720000` PASS: three-track,
255-sample fixture, DMA0/1/2 active and3off, low-bit24 edit stopsall, exactundo,
patternrestart, exactsave/reopen/play/secondexactsave. Guest bitmap inspected:
classic layout retained, fourth track empty. Both editor exits RC0. Allfive ENV
presence/bytes restored; independent cleanup proves running guest, allDMAoff and
exact run/launcher absent. Explicit release completed.
Fixture SHA256 `304eaf2bf60af9074e0d7a300dc91663a620ef3b3b97d3e98b6448806b939726`.
No physical testing, listening acceptance or positive AmiGUS qualification.
