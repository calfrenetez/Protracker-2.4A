# CRC diagnostic HOST and Amiga build qualification

On 7 October 2026 the unchanged genuine 65-case RAM fixture passed on the host
with the new project nibble CRC and the diagnostic markers. The strict sanitizer
compile and run returned zero, with the complete original oracle and empty error
streams. The independent saved-evidence audit passed.

A separate Amiga crossbuild then passed all 34 bounded calls, including the same
28 production/fixture units and strict m68000/soft-float flags. All complete
streams, products, generated dependencies and tool/source custody checks matched.
The independent native-build audit also passed. The resulting HUNK is 239,724
bytes, SHA-256
`e263add279617ba08393eeb7f7d809ff759f85d1ea4165b1a5e85ed97dc37144`.

`observations.json` records the actual calls, receipt hashes, candidate and
preservation facts. The four host command streams and both independent audits
are copied unchanged here. Complete native build streams, dependency files,
products, map and disassembly remain in the hash-bound private build directory
named in the observations; no additional build or test was run to publish them.

This candidate has **not run in Amiberry or on the A1200**. Its separate reviewed
runner preserves the 90-second native deadline, Stack 131072 and complete
296-line admission contract. The separate bridge maintenance decision and fresh
shared target coordination are still required before execution. The typed causal
preparation proposal is a separate software change and is absent from this build.

The original candidate's 90-second attempt remains failed. Its later complete
output was preserved during the approved recovery and does not reclassify that
failure. Recovery, restoration, independent verification and explicit release
are documented in [approved recovery](../approved-recovery/README.md).

These checks establish host fixture and crossbuild results only. They establish
no native speed, timeout cause, live exact scheduling, IRQ/whole-stack/WCET,
physical memory/device behavior, DMA, card upload ordering/completion, voice-stop,
audio or listening acceptance.
