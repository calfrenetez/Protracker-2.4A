# Donor memory ownership on shared030

2026-09-24 run1790245901323383000: production master allocator, 65 explicit
Fast/not-Chip allocations verified by TypeOfMem, zero owned bytes after source
replacement, failed staging/import, independent selected-sample copy, undo/redo,
source close and document disposal. Existing source_test.c supplies the failure
matrix and content/ownership assertions. Pool-budget refusal does not spill to
Chip. Host source sanitizer regression and full native build pass.

This covers explicit fixture allocator ownership, not runtime-library internal
allocations or physical memory/performance acceptance. Shared run completed,
DMA off rechecked, exact files removed and reservation explicitly released.
Unrelated display work is not qualified by this non-UI test.
