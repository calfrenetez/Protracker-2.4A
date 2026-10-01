# AmiGUSTest0.2 final-release confirmation prepared

1 October2026. Physical production-adapter discovery already PASS in the separate
amigus-physical-discovery evidence. Card reservation/transfer/IRQ/audio remain open.

The old diagnostic issued void FreeCard calls and could close its library while a
broken driver retained diagnostic owner addresses. New result separately reports
release_code/confirmed/retained. After both exact diagnostic identities are freed,
a NULL-owner reserve probes free state without acquiring a new identity. On busy
or error the native Task parks cooperatively, retaining library/card/stack/owner
addresses; it never closes, retries, exits or authorizes runner cleanup/reset.
Normal final Free alone cannot produce PASS. Busy foreign initial reservation
SKIPs before any release/probe. No MMIO, interrupts or playback.

This probe is specific to the verified RC6 public implementation:
https://github.com/necronomfive/AmiGUS-pub/blob/f17648d63346bab932bca5b48da922a78289f7dc/Software/Drivers/Base/src/amigus_base.c
It refuses any non-NULL current owner for a NULL check identity and only writes
NULL when already free. Release and SDK-snapshot implementations are byte-identical;
source hash recorded. Public source reviewed only, not copied into production.
Official RC6 distribution archive SHA matches published asset digest. Physical
adapter requires installed library checksum matching one of12 immutable official
variants BEFORE reservations; diagnostic runtime library1.1 required. Different
or unsupported driver refuses. This is a conditional driver contract, not a new
universal reservation API or active-IRQ quiescence proof.

ASan/UBSan20 mock scenarios PASS, including a failure confined to the last release
and probe failure after apparently successful releases. Two physical guard and
five runner guard tests PASS. Pinned GCC6.5/-fbbb=- native builds PASS:
AmiGUSTest16800 bytes/SHA6964732027281f46bc803a03b4f7e4ef19c65f4fe9a599da98aeac87e6e36fa3;
PTDiagOwnershipTest14544 bytes/SHA75a7924a10d6d0cee68572fedf1509110a799005d3349ab3bee164766629591d.
Private-vector native ABI fixture additionally checks literal NULL in published
d1 owner register for both bases and PCM/wavetable flags. Native tests NOT RUN in
this preparation record. Shared --amigus-diagnostic runs mock fixture/private ABI
and exact real diagnostic discover/ownership modes expecting missing-library5;
only those expected refusals can qualify the same real executable for physical
ownership. Exact new coordinated windows/cleanup required for each target.
