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
