# Injected recording lifecycle

Host: two ASan/UBSan cases pass (6.489s), covering collector and lifecycle.
Shared030: run1790567724145553000 PASS/RC0, 30 Fast allocations, zero owned bytes.
The fixture covers exact 8/16/24 mono/stereo, 256-frame polling bounds, delayed and
failed starts/stops, invalid acknowledgments/counts/values, device overrun,
cancellation, partial finish and undoable publication with allocation-failure retry.
A stop fault cannot release staging or publish a prefix; cleanup remains pollable
until the adapter explicitly confirms quiescence. No allocation occurs in polls.

Indexed tree7d51cfd9fccee3e890253e2957202498c6967651; all112source hashes and three
native binary hashes verified. Tested executable112000bytes SHA256
3b450ebc5e9e61faa1511601176769b2f7ec2a3d1b4c97fbb2ae85727067905f.
Normal cleanup plus separately locked independent running-guest/all4DMAoff and
owned-run/launcher absence checks completed. Explicit RELEASE sent to AmiConnect.

Input callbacks are synthetic. This does not prove native input, device formats,
AmiGUS MMIO/interrupt ownership, sound quality or physical hardware. Actual Amiga
remains switched off and was not accessed. Native editor input controls are not
wired by this increment. The earlier incomplete export f382030... was never
built or run; a host script assertion during runner editing was corrected before
the complete indexed export above, with no product-test failure.
