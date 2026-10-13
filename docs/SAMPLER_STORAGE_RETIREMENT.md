# Sampler storage retirement

`pt_sampler_lifetime_storage_retired()` is an internal, read-only inventory
check. It returns true only when the sampler has no lifetime fault, live or
pending records, charged allocation bytes, current versions, or sample-table
obligations. It reads the sampler's own scalar and pointer fields without
following record or version pointers, invoking callbacks, or freeing storage.

This differs from `pt_sampler_lifetime_idle()`, which checks pending execution
and faults. A sampler can be idle while a pin, history resource, current version
or table still retains storage. Calling `pt_sampler_release()` does not retire
pins held elsewhere; the final unpin must release their remaining storage.

The new check is not permission to release a document. Document replacement
still needs a genuine document-to-owner relationship, confirmed execution stop,
complete applicable inventory retirement, and checked replacement ordering.
Native editor composition of that relationship remains unfinished.

## Validation

The exact private sampler fixture compiled with C99, `-Wall -Wextra -Werror`,
`-UNDEBUG`, AddressSanitizer and UndefinedBehaviorSanitizer. Compilation and
execution each returned 0, with empty diagnostics and the full expected output.
The fixture retains the existing sampler tests and adds 301 source-derived
assertion observations covering pins across release/final unpin, pending
records, faults and malformed inventory, all current slots, tables and budget
rollback. Allocation, budget, release and idle implementations are unchanged.

The three integrated code files match the tested private bytes. This is host
validation. Native compilation, Amiberry, physical A1200/AmiGUS and listening
acceptance for this addition remain untested.
