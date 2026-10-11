# Finite sampler TRIGGER → CONTROL → STOP

The optional factory now prepares one original TRIGGER, one CONTROL and one
later STOP with two live command slots and the existing 32-reader bound.
CONTROL and STOP take typed original reader handles and derive genuine keys;
neither acquires another reader, sample pin, master, cache, conversion or upload.
The third command requires actual CONTROL completion and genuine first-command
queue service plus consumed handle closure. Retired identities are opaque
comparison values, never dereferenced disposed holders.

Preparation/allocation/enqueue attempts consume their lifetime stage. A failed
or uncertain attempt cannot silently retry or admit a fourth command. Successful
lower transfer retains ownership even if an outer callback faults. First and
later exact frame windows and timestamps remain unchanged. STOP publication
closes admission; exact commit marks selected readers DRAINING. Separate command,
reader and source quiet proofs control eventual master/cache release.

## Verification

The exact genuine-holder host fixture passed 74 own-main cases using C99 strict
warnings, assertions, AddressSanitizer and UndefinedBehaviorSanitizer. Its 24
positive lifetimes cover 8/16/24-bit masters, 8/16-bit AmiGUS cache representations
and mixed four-Paula/twelve-card or sixteen-card routes. Cases verify original
960/1920/2880 frames, actual first-command service and NULL handle consumption,
slot reuse, publication faults, mutations/reentry/aliases, independent reader
retirement and full master/save/cache beforeimages. Later stages allocate zero
new readers, PCM conversions or playback caches.

Actual collector evidence records all three job waits/RC0/both EOFs, paired
671 child and 23 outer inputs, original anchor-only/final-empty observations,
independent closes and restoration. See [host results](host-results.json) and
the actual [fixture output](fixture.stdout). The matching ordinary repository
sampler74 driver and combined controller STOP52 driver passed. Existing factory
default24 and STOP55 regressions also passed once against this same product.
Each retained its original full oracle; default24 describes its lifetime subset
rather than an invented total counter. Current controller43 and producer62
regressions also passed against the combined adopted sources.

Historical first compiler and second fixture failures are retained separately
in [preserved attempts](preserved-attempts.json). The second correction changes
only the fixture's PENDING unchanged-output expectation; it relaxes no product
classification or ownership rule.

## Remaining scope

Default two-TRIGGER and separate STOP-only bindings remain separate. The typed
controller third-stage wrapper and producer/native frontend integration remain
unadopted. The original74-case run did not exercise full32-reader occupancy or
quantized-first composite runtime. A separate [39-case quantized-first host
regression](quantized-first/README.md) now covers the latter; full32 occupancy
remains unqualified. A separate strict m68000/soft-float compiler check passed for this current
sampler C unit, including actual dependency admission, object, assembly and
individual stack rows. See [compiler results](compiler-results.json). No linked
native executable, aggregate stack/IRQ/WCET, exact live scheduling, Amiberry,
A1200/AmiGUS, device completion/order/stop/quiet, sample-RAM capacity or listening
acceptance follows.
The private host cache callbacks operated on ordinary model memory.
