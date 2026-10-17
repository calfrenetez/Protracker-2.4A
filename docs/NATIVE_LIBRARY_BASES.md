# Native editor library-base ownership

The editor manually opens `intuition.library` and `graphics.library`. Its two
library-base globals now have explicit zero initializers, producing strong BSS
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
Each object has one four-byte strong BSS definition for each library base.
The display changes remain separate from this five-line source correction.

These are object-level checks. Full application linking, library-reference
lifecycle, whole-Task stack sufficiency, Amiberry execution and physical-A1200
execution of this editor change remain to be qualified. The requester fixture's
separate linked/runtime results do not qualify the editor application.

The compiler/object results are recorded in [clean-object.json](../evidence/native-library-bases/clean-object.json) and [working-object.json](../evidence/native-library-bases/working-object.json).
