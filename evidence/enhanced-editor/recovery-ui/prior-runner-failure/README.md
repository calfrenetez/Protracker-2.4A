# Requester automation failure

Run recovery-ui-1790555231055219000 remains failed: the runner sent Return to an
Intuition EasyRequest, so Restore was not selected. Seed helper passed and
Keep current was verified. Normal bounded Escape cleanup declined the pending
requester, exited the editor withRC0, restored the presence and exact bytes of
all five temporary app ENV settings, and removed only the owned staging.
Independent running/identity/DMAoff/absence check passed; window released.
No restart, reset, clock or physical operation occurred.

The corrected runner uses Left-Amiga+V, as documented in the official
[Intuition requester documentation](https://wiki.amigaos.net/wiki/Intuition_Requesters),
and waits for the requester to draw before capture/input. No product binary
change was needed for this automation correction. Full UI acceptance remains
separate from this failed window.
