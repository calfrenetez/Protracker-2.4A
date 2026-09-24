# Native library-availability probe

2026-09-24: eight host sanitizer checks pass; pinned native build passes.
Shared030 run render-files-1790227400915491000 returned zero on two bounded
probe attempts. Each reported library=0, cards=0, pcm_cards=0, closed=1.
This proves graceful unavailable-library handling in the real native adapter,
not a positive library/card or playback test. Probe never calls reserve/release;
no MMIO or interrupts. No card pointers escape discovery.

Fresh AmiConnect clearance, shared lock and live guest guards preceded the run.
Exact owned cleanup completed and window explicitly released. No reset/relaunch
or physical operations. Build with `tools/build_amigus_reservation.py --discovery`;
run with the coordinated shared harness `--amigus-discovery`.
