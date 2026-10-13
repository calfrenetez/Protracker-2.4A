# Typed document replacement

The opt-in `editor_document_replacement` API replaces a document that has a genuine checked mixed owner. It prepares a separate, complete candidate before retiring the old owner, then constructs the new owner against the final `&document->project` address.

Enable `PT_DOCUMENT_TYPED_REPLACEMENT=1` only in a build that includes the typed adapter and its checked-owner dependencies. It supplies a negative document mutation/output veto after genuine factory and publisher identity checks. A caller Boolean, copied control or an inventory status cannot grant release authority. The legacy enrollment prototype is a separate feature and cannot be enabled alongside this option.

## Lifecycle

`begin` captures the old binding, candidate, engines, allocator context and complete storage extents without invoking callbacks. The candidate must be detached and genuinely own fresh, disjoint allocations.

`step` stops the checked owner and drains its voices and caches before disposing the old editor. Disposal is irreversible. `WAITING` retains allocations while close work remains; it does not promise that the old editor is still playable. `RETIRING` holds both documents until real retained pins and sampler storage obligations retire. A retired expanded sample table must not be used during this phase.

After storage retirement, checked release frees the old document's known storage and moves the candidate exactly once. The new editor and owner use the final project address. `COMMITTED` reports the new genuine owner. `COMMITTED_NO_OWNER` reports a published document with confirmed quiet after owner construction failed. It preserves the new document and does not pretend to roll back the old one.

`HOLD` preserves uncertain controls and allocations. It permits no forced cleanup or second release. Keep the control, documents, engines, contexts and allocators alive until confirmed completion. `finish` also requires confirmed owner/cache close, editor disposal and final sampler storage retirement. Inventory retirement is necessary here; it is not factory membership or document release authority.

## Core document guards

The document core adds checked release and bounded staging ownership tracking. It revalidates the captured header, allocator and known storage after allocation, read and finish callbacks before decoder writes or publication. Callback reentry and changed ownership are refused; uncertain storage remains retained instead of being released twice.

These checks retain the existing ordinary document API signatures and result conventions. With the typed option absent, the core has no Editor or enrollment link dependency. The core-only compatibility matrix must pass on this exact composition before promotion; that macro-off check is separate from the typed HOST result.

## HOST checks

The 83-unit portable HOST fixture passed with ASan and UBSan. It retains the original 15 mixed regressions, 39 checked lifecycle cases and 18 allocator-alias cases, and adds nine replacement groups. They cover detached preparation failure, copied-control refusals, real expanded sampler storage with retained pins through disposal and final unpin, final-address owner construction, post-publication establishment/factory refusal, and release callback reentry.

The final uncertain group deliberately retains reachable state for the process lifetime. Its success means the refusal and retention contract held; it is not a quiet or complete-release result.

Run the repository test recipe with:

```sh
python3 -m unittest discover -s tests -p test_editor_document_replacement.py
```

It uses the existing prepared source copy plus only this feature's overlays, leaving unrelated working-tree display edits outside the build. The original C99, warning and sanitizer flags remain; the typed option is scoped to this test.

Native compilation of this typed composition, full editor/UI wiring, Amiberry and physical Amiga behavior remain unverified. The opt-in API does not constitute a complete application release.
