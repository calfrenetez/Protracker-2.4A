# Failed prepared Paula fixture

Run 1790846045339020000 remains FAILED, native return 20 at preparation check line 38. Candidate tree 5e717af6b35ed20ed03c0b389f2b69bb67493934; executable SHA256 6691943057e508e98326d1b804955f1932d931c67888a5b2c0d0c43d6cb1331e. No prepared reader started. Original log did not record the preparation result.

Normal return 20 occurs only after successful bounded song stop, detach, editor disposal, DMA-off/context checks, document release and native allocation release. Incomplete cleanup returns 21..25 and retains storage. The global zero-owned-byte assertion was not reached on this failed path. Separate locked read-only cleanup verifies a running guest, all four DMA channels off and exact run/launcher absence. This is failure cleanup evidence, not playback acceptance.

Later host reproduction of the same configuration reports capability OPERATION, channel 4, SEGMENT: zero-leading classic8 follows the renderer's supported classic repeat-segment semantics, refused by the one-shot-only prepared adapter. This host diagnosis does not retroactively add diagnostic output to the original native run. A distinct corrected fixture tests this refusal and actual silent16/24 output separately. No same-byte retry.
