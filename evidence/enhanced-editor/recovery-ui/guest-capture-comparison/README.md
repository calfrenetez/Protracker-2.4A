# Guest bitmap versus emulator capture

Run recovery-ui-1790556536015879000 passed with candidate1224ac2fa6ccd0674cf0a77caf7de4f02ccd041d92eeecffe7d47267793fa5fc.
The second Amiberry IPC capture was black while a fresh AmigaBridge bitmap
showed the actual recovery requester and underlying editor. Both requester
choices were visually confirmed using guest captures before input. Recovery,
real30-second idle autosave, exact full-precision save, original preservation,
normal exits, five exact ENV restorations and independent cleanup passed.

The bridge screen/window list queries reported no entries even though bitmap
capture returned the correct complete640x512 four-plane screen. Raw responses
and unmodified PPM captures are retained alongside PNG format conversions.
These results expose limitations in the capture/list evidence; a black IPC frame
alone does not establish absent guest UI. Native hardware display is untested.
The startup presentation change supplies editor context behind the prompt; it
is not claimed to fix the independent IPC black-frame behavior.

The recovered status still wrapped because it was32 characters; the final
candidate shortens every recovery status to at most30 characters, matching the
existing classic status-field limit. No layout changes are introduced.
