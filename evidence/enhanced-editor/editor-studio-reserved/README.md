# Studio output reservation ownership

An optional already-open PCM reservation now transfers a single access lease to
integrated Studio output. It is acquired before port callbacks and retained through
initial failure, natural drain or Stop, confirmed reset/session detach and a
separate adapter-quiescence acknowledgement. Pending/failed quiescence forbids
restart/detach/card close. Quiescence must mean no remaining adapter references or
installed interrupts; it cannot be inferred from an empty host queue. On success
only the access lease ends: the caller retains and later closes its reservation.

Host ASan/UBSan staged lifecycle suite passed29.488s. Tests use fake library/port
callbacks to verify clean natural completion, allocation/initial-reset failure,
leased Stop, pending/failed quiescence, resource selection, refusal of early close
or restart, and exactly-once library/card release. No actual library/device access.
Native/main syntax build passed;154 inputs matched index. Candidate254040bytes SHA
b5bde80f692fb06c7c105d23ed8e81cd9cdd99ebbdafe376f85ea0c2e634c99a.
Shared030 run render-files-1790547068707183000 passed within90seconds,RC0,
141Fast allocations/zero owned bytes/noChipfallback. All4DMAoff; exact run/launcher
cleanup and independent absence verified. No physical, audio, MMIO or real-time
acceptance follows. Physical Amiga remains switched off and untouched.
