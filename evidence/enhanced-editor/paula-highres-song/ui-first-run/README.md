# First UI run: failed Stop observation

Run paula-highres-ui-1790559614924884000 reached initial song playback, sample
reverse (low24-bit-only) and DMA stop, exact undo, pattern restart, and byte-exact
save. It failed an immediate DMA query after sending F10 without waiting for the
application to process Stop. Reopen was not reached; this is not a full UI pass.
Normal exit RC0, exact restoration of all five temporary ENV settings, runner
cleanup and subsequent independent DMA-off/run/launcher-absence checks passed.
The window was explicitly released. No automatic reset/retry was performed.
A later separately coordinated run uses a fresh STOPPED application frame before
checking DMA. Product candidate bytes remain unchanged.
