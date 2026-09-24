# Donor shared setup diagnosis

Guard-only run1790244036961180000 never launched an editor: the shell environment
string condition failed. Prior ENV restored, done/DMA-off checked, owned files
cleaned and window released. No donor acceptance.

Corrected run1790244200365716000 verified exact isolated recent-prefix bytes
BEFORE editor launch; native log confirms that prefix. Donor requester interaction
did not complete. Screenshot shows corrupted lower display/requester area in the
binary built with preserved unrelated display edits. Normal cancellation/exit
completed; ENV restored byte-exact, all DMA off, owned run cleaned and released.
No reset/relaunch/physical action. No donor/UI acceptance from this run.

A separate clean committed-source build is being used to distinguish display
work from donor workflow. Existing checkout edits remain untouched.

Clean b184a93 source snapshot built with pinned compiler/assembler; no working
checkout edits altered. Run1790244483007640000 shows the full requester correctly,
but typed absolute path plus Return still did not complete it. This remains an
interaction diagnostic, not successful donor UI acceptance or a proven loader
bug. Normal exit, exact ENV restoration, done/DMA-off and owned cleanup passed;
window explicitly released. Next inspect requester Load-button handling; do not
repeat unchanged input blindly. Host source ownership/undo sanitizer test passed.
