# Loaded AHI driver explains idle PCM reservation

1 October2026. User reports no known active player/test. Two fresh coordinated
read-only realA1200 windows completed, with sharedlock/physicalCPU/Plipbox/qualified
bridge checks. Task bridge supplies mostly unknown names, so no taskowner was
identified from that list. No physical ownership retry, stop, unload, reset,
register, IRQ, output or configuration change occurred.

Loaded AmiGUS.audio4.23/30.8.26/020ID has openCnt0. ahi.device6.6 openCnt0.
amigus.library1.1 loaded000ID/openCnt1. Installed AHI driver CRCBA67FEF4/11028
matches officialRC6 020-NO_LOG snapshot SHA
3ad6ec0b598ebbb215121a72a3945f3360f64963df133d53d45a65b144c84c27.
Installed base library earlier matched020-NO_LOG; its loaded000variant indicates
that file installation does not establish which optimized base is resident.

Pinned official RC6 source review (source hashes and URLs in source-review.json):
AHI CustomLibInit reserves PCM with its librarybase as owner; CustomLibClose
frees it. Ordinary LibClose decrements its count and calls LibExpunge only when
already marked for delayed expunge. LibExpunge calls CustomLibClose once its
open count reaches zero. AHIsub_FreeAudio decrements UsageCounter, not the card
reservation. Thus a loaded unused AHI subdriver can retain PCM without a player.
This strongly explains observed busy0x101, but we did not dereference the private
owner pointer or claim a definitive runtime owner-address match.

Both inspections returned DevBench to connected localhost shared030/running68030/
all4DMAoff; no pending script, Task/control/reservation or recovery hold. Explicit
RELEASE ACK149 and next release sent. No own paths staged on physical target.

Concrete next choice: temporarily unload ONLY the unused AmiGUS.audio driver
through normal cooperative library teardown (refuse any active/open user), run
separately qualified direct PCM checks and restore it afterward; or keep normal
AHI service resident and defer direct PCM ownership testing. No forced FreeCard,
process kill, broad library flush, reboot, installation or saved preferences change.
A scoped unload helper would need host/native qualification before use. Current
follow-up remains paused for this runtime-audio-service choice.
