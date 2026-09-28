# Repeated-launch startup display failure

The status-shortened candidate1266e009a41efe75939af5bb0ef7d3ffe49aa07dd2369625bec12752a8a78c50
built successfully but this UI run failed its second-prompt visual gate. The first
prompt was visible and Keep current passed. The second remained black in repeated
captures; no Recover input was sent. Bounded normal Escape cleanup exitedRC0,
restored all five temporary ENV settings exactly and passed independent running,
DMA-off and exact-path cleanup checks. The window was released without a reset.

The next candidate presents the initialized editor and synchronizes display output
before opening the modal startup recovery requester, then invalidates the view
cache after any document replacement. This preserves the accepted classic layout.
This record is a failure, not qualification of that subsequent change.
