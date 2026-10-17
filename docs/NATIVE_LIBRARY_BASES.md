# Native editor library-base ownership

The editor manually opens `intuition.library`, `graphics.library` and the ASL
file requester's `asl.library`. Their library-base globals now have explicit
zero initializers, producing strong BSS
definitions instead of tentative COMMON symbols. This makes ownership explicit
and avoids sharing a base slot with a linker-selected automatic library opener.
An early exit before the manual opens must not close an automatic opener's
reference. The existing manual open/close sequence is retained.

The library-name and timer-device arguments also use the SDK's declared
`CONST_STRPTR` type. Strict C99 warnings remain enabled; no SDK declaration,
minimum library version, device unit or call argument was otherwise changed.

Private copies of both the clean editor entry point and the version containing
the existing display work pass dependency extraction, preprocessing, code
generation, assembly and object inspection with the genuine 68000 toolchain.
Each editor-entry object has one four-byte strong BSS definition for each of
its two library bases.
The display changes remain separate from this five-line source correction.

These initial results are object-level checks. Subsequent full-editor build
and link results are recorded below. Library-reference lifecycle, whole-Task
stack sufficiency, Amiberry execution and physical-A1200 execution remain to be
qualified. The requester fixture's separate linked/runtime results do not
qualify the editor application.

The ASL requester and Paula playback unit also pass all five strict native
compiler/object stages. The requester has a four-byte strong BSS `AslBase`;
the ASL and `audio.device` names use `CONST_STRPTR` without changing their
arguments or control flow. The actual Paula object retains the expected
five-argument replay call and return-value check. This checks the object-level
calling convention, not device ownership or successful playback.

Those two units were initially among 44 completed units in a 69-unit editor
build that stopped because the private SDK copy lacked `machine/endian.h`.
That failed result is preserved. A later build using genuine SDK headers,
a Linux-only stem-export feature guard and reduced automatic main storage
completed all 69 units and linked with the ordinary replay object. No compiler
warning was disabled. Current provider, startup and application runtime checks
remain separate; see [the native editor build notes](NATIVE_EDITOR_BUILD.md).

The compiler/object results are recorded in [clean-object.json](../evidence/native-library-bases/clean-object.json), [working-object.json](../evidence/native-library-bases/working-object.json) and [asl-paula-objects.json](../evidence/native-library-bases/asl-paula-objects.json).
