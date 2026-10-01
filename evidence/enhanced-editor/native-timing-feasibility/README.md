# Host timing feasibility evidence

1 October 2026; base commit `b43f1e272a82339a445e9df8e6e8295cb8291f47`.
No native candidate, emulator window, CIA ownership, audio or physical operation.

`frame_window.c` links the production elapsed-clock conversion and independently
checks the ceiling/floor oracle at all four edges of each target frame window.
It checks 184,200 windows across 709379/715909 Hz and 44100/48000 logical frames
per second. The epoch exceeds the 32-bit counter range. ASan/UBSan and
warnings-as-errors pass, exit code 0. `frame-windows.json` contains the results,
compiler/flags and exact source input hashes.

Reproduce from the repository root (use any private output path):

```sh
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  evidence/enhanced-editor/native-timing-feasibility/frame_window.c \
  src/core/elapsed_clock.c -o /tmp/pt-frame-window-probe
/tmp/pt-frame-window-probe
```

This numerical diagnostic is not a latency benchmark, song-gate execution,
interrupt-safety test or hardware activation measurement. The linked feasibility
assessment separates inspected source behavior from executed numeric checks:
[`NATIVE_SCHEDULING_FEASIBILITY.md`](../../../docs/NATIVE_SCHEDULING_FEASIBILITY.md).
