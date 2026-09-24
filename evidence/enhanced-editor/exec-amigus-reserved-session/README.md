# Reserved PCM session validation

2026-09-24: eight host sanitizer checks pass; pinned Amiga build and shared030
run pass. Run `render-files-1790226885613878000` acquired the shared lock after
AmiConnect clearance and fresh guest guards. Return code 0; exact owned files
cleaned; window explicitly released.

Production reservation/session/queue/FIFO code uses injected fake library and
port callbacks. Each port call asserts a live reservation/access lease. Failed
initial reset, Stop holding a queue lease, and natural odd-frame tail completion
retain the library/card through failed and pending reset, then detach before
ending access and releasing ownership. Three tracked Fast/not-Chip allocations,
zero owned bytes, budget refusal. No real library calls or MMIO. Native public
library adapter is linked but only its callback table is constructed.

Reproduce: pinned AMIGA_CC with `python3 tools/build_amigus_reservation.py`, then
coordinated `tools/shared_infra_render_files.py --studio-memory reserved-session`.
The latter uses the shared infrastructure Python/runtime and requires a fresh
window clearance. No physical or audio acceptance is implied.
