# Stopped editor capture diagnostic

The runner can now request captures only after a fresh STOPPED - AUDIO RELEASED
frame and allfour DMA channels are off. Each capture is capped at45seconds and
by the remaining180-second workflow budget; complete640x512/four-plane/2048-row
protocol results are still required. Capture phase and completion are explicit.
No application or shared bridge changes are included.

Odd two-master candidate265184, SHA badc183fbecf3111a8f08fc9801a02e923dca7f74dd4e08929b3bf1cc51c9969:
run1790564131396006000 PASS functional workflow and both complete stopped captures,
5.88/6.02seconds. Both bitmaps inspected: classic layout, three active columns,
empty fourth column, correct255-frame sample length and STOPPED status. This is
stopped-state visual evidence, not active animation/audio/performance acceptance.
Both exitsRC0; fiveENV exactrestore, DMAoff, exactprivatecleanup, subsequent
independent locked running/absence check and explicit RELEASE complete.

The earlier mapped255-master playing capture failures remain FAILED/unresolved.
The two-master stopped result alone does not isolate the cause: fixture and
playback phase differ. Matched earlier candidate264512 SHA1324c3f6a4415cd77d3f65a8d1ae0d057d4abecb2060bedf1e20c42aad2b320b
and original255-master fixturee2a82a1837eb1bfab476bf5e8dd8fd9df252fe457e6e785e7be0776c67a34cd6:
run1790564298706633000 PASS the fullfunctionalworkflow and both complete stopped
captures in6.01/6.04seconds. Both bitmaps reviewed for the classic layout,
three-track display, empty fourth column and STOPPED status. Captured rows are
mostly empty later rows; high source instrument identities are proven by the
fixture/native/functional tests, not these particular stopped images.
Bothnormal exitsRC0, fiveENV exactrestore, allDMAoff, exactprivatecleanup,
subsequent independentlocked running/absence check and explicitrelease passed.

This matched comparison supports a stopped-capture workaround. It does not prove
the root cause or clear either active-playback timeout. No shared infrastructure
or product changes were made. Use the explicit --stopped-capture option for
layout evidence; report capture phase separately from functional acceptance.
Physical Amiga stays OFF and untouched.
