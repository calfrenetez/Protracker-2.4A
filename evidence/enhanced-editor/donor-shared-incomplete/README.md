# Shared donor editor workflow — incomplete

Run source-shared-1790241314973098000 did NOT pass. Native PTSourceTest returned0,
but the first requested donor.pp load returned SAMPLE EDIT REFUSED with revision0.
The harness timed out waiting for donor preview. No import/save/export acceptance.
Normal bounded Escape closure returned editor0; the launcher's reopen returned20
because no saved project existed. Script completion and all four DMA-off checks
passed. No reset/relaunch. Failed guest files and launch preserved; shared window
explicitly released to AmiConnect, no active process/control remains.

The legacy script was adapted to shared Guest locking/identity but initially
omitted recents isolation. Launch added its run-owned input to the guest's default
recent list. A proposed read-only capture of recent slots for precise repair was
rejected by automatic approval review because they may contain unrelated private
paths. That action did not run; no bypass or history overwrite occurred. Repair
requires user permission. Do not commit any private recent-list contents.

Host source ownership/undo regression PASS. The revised shared_infra_source.py
uses explicit full donor paths and run-owned recent-prefix backup/restore, but
this revised harness is syntax-checked ONLY and has NOT been retried in guest.
Screenshot also shows existing dirty display corruption; no UI acceptance is
claimed and unrelated display work has not been changed.

## User-approved repair completed

The user approved local inspection and precise reversal. Both recent-list slots
validated; generation29 contained only this test's insertion relative to valid
generation28. State was rechecked before restoration. The previous ten-entry list
was restored byte-for-byte and verified. No private paths or recent payloads are
included in repository evidence. `repair.json` contains only non-private results.
After script-completion and all-DMA-off checks, exact failed-run files and launch
were removed, with failure logs retained. Shared window explicitly released.
The original donor UI failure remains unqualified; repair is not a test pass.
