# Native editor build and stack storage

The editor entry point keeps its view cache, Paula state and nine path buffers
in static storage. Their addresses remain stable for one application lifetime,
and this removes about 15 KiB from main’s automatic storage. The program still
expects one main invocation per fresh executable load.

The change consists of three static qualifiers. Buffer sizes, initial strings,
layout, call order and musical scheduling are unchanged. The previously
uninitialized chosen-path buffer is now zero initialized; it is consumed only
after a successful selection or an explicit recent-path assignment.

Stem export now requests GNU feature declarations only on Linux. This retains
Linux’s existing behavior and avoids conflicting newlib and Amiga timer timeval
declarations in the native build. No SDK header or compiler warning was changed.

The complete clean editor build passes 345 dependency, preprocessing, code,
assembly and inspection stages across 69 C units. The genuine replay object and
runtime libraries also link successfully into a static executable candidate.
The working editor entry point is qualified separately with the existing
editor/view/pattern headers; its display edits are preserved outside this commit.

The clean main’s individual stack report falls from 18,700 to 3,632 bytes;
its frame reservation falls from 18,552 to 3,484 bytes. These measurements do
not establish a whole-task stack minimum. Other routines, nested calls, startup
and system libraries require additional space. A supported startup stack
allocation is being qualified separately.

These results do not qualify application execution, Amiberry GUI behavior,
physical A1200 operation, audio ownership, playback timing or listening.
The compact evidence files in ../evidence/native-editor-build/ keep those
boundaries separate from compiler and linker results.
