# Genuine paired sampler reader factory

`sampler_mixed_readers` creates the genuine holders consumed by the opt-in
`mixed_scheduled_readers` queue. It establishes no master and attaches no editor.
The caller first establishes authoritative sampler-owned 8/16/24-bit masters,
then supplies the actual sampler/project, an initially empty dedicated attached
card-cache backend, a fresh empty borrowed typed queue, and complete callback
context extents. The factory owns its private validation and incremental jobs;
a public record cannot substitute for their completion or pin provenance.

One pool registers at most two command controls, thirty-two persistent reader
controls, sixteen actions per command and sixteen context extents. Pool and
separate controls share an explicit aggregate byte budget. Actual Chip allocations
use a separate bounded exact pointer/size ledger. Each trigger retains an
independent master pin and Chip or card-cache lease. Temporary upload/preparation
pins expire independently. Shared immutable sample/cache data does not merge
reader lifetimes. CONTROL and STOP borrow no predecessor holder: they use the
complete original positive adopted/active key from the genuine queue getter.

Open starts a cancellable validator. Each preparation advance performs one
transition/action or at most 256 derived cache bytes. Preparation never reads a
playback clock, publishes, activates or probes voices. Paula caches contain signed
8-bit selected-channel data with even padding and whole-sample ONCE geometry.
Card caches use selected-channel 8/16-bit data and explicit endian/voice-plan
bounds. Cache lease slots and sample-RAM arena indices are separate: the factory
resolves the genuine resource pointer against the arena, then protects its full
reserved capacity. Numeric card ranges never become CPU protection spans.

All complete outputs are checked against the pool, registered LIVE and RETIRED
controls, full captured sources, current sampler versions, declared contexts and
Chip allocations before writes or writable reentry faults. Fixed identity checks
precede former-table walks. Known allocation aliases are never freed as fresh
ownership. Admitted callback reentry or staleness fails closed. Actual successful
enqueue still transfers ownership and preserves its ticket when an independent
fault is latched; that fault cannot invent a local cancellation authority.

The caller keeps source values immutable during calls and changes the supplied
revision or sampler generation for every edit between calls. Publish/key/service
APIs use the captured revision rather than obtaining an editor revision: there
is no attached editor or revision supplier. An edit that changes only an external
editor counter is therefore not observable here. A future editor adapter must
cancel and drain behind its edit barrier before changing such borrowed source
values. The queue remains factory-exclusive for admission/services, and its owner,
sampler fixed control and callbacks remain alive through all drains and closes.

Command detach and reader retirement are independent exact proofs. Both proofs
and zero command references are needed before reader pins expire. Issued but
unadopted or uncertain readers provide no CONTROL/STOP authority. Retired contexts
remain registered until explicit handle close. Stale-source NULL-output services
can drain independent proofs without former-table traversal. Cancel/stop affects
only untransferred resources and adds no musical STOP, clock, polling or retry.
Close every command/reader handle and the pool before its borrowed queue owner.
If the caller retains handles for cancelled, untransferred readers, close those
reader handles before the command: command close consumes its remaining local
readers, so their former handles cannot subsequently be used.
A close returning zero after consuming its slot to NULL is terminal; never retry
freed storage. Factory close clears only its dedicated unpinned derived-cache
identity; it does not detach the backend/reservation or close the queue.

Exact isolated host attempt `g2k6au4m` passes six ASan/UBSan compile/run calls with
empty stderr, using committed c4cbc04 plus four factory overlays/font: 927 saved
inputs and 27 factory units. Independent frozen source/evidence review reports a
bounded pass. Tests include 24 sixteen-reader master/cache lifetimes, both proof
orders, literal conversion/endian/channel/padding and exact save oracles, released
caller and sampler-current master ownership, 32-reader replacement pressure,
fragmented cache slot 2 / arena block 0, aliases, expired tables, malformed proof
domains, callbacks and consumed close-zero cases. Earlier actual failures remain
preserved. The separate combined fixture `ln2dk4t_` also passes its two sanitizer
calls and independent review. See the [host evidence](../evidence/enhanced-editor/paired-sampler-activation-host/README.md).
These results qualify callable HOST software only.

The complete combined fixture cross-compiles and links to a 266,204-byte HUNK
candidate. Its [compiler evidence](../evidence/enhanced-editor/paired-sampler-activation-portable/README.md)
records 41 successful calls and independent offline saved-byte verification.
That candidate has never been executed. Native entry, emulator and physical
execution, placement/device timing/audio and listening remain separate.

Paired song/editor lowering, native PLAY and a qualified real atomic activation
port remain unfinished. Neither this factory nor an injected RAM model qualifies
physical card capacity, upload ordering/completion or actual voice stop.
