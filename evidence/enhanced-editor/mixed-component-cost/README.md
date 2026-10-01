# Mixed boundary public-call component observations

1 October 2026. Test-only separate native6manual scenarios2/16voices at8/16/24.
Production unchanged; hostfull195ASan/UBSan PASS8.070s, not native195.
162indexed source hashes match candidate/treefe2ed6253284b59506166b62d99b105ac080d08d;
zero generated/3verifiedbinaries. PTExecMixedOwnerTest233700bytesSHA256
e3fc198aaa5c785ce9da6160db85db2a3560efbcb10251a228b5d4fda47784ba.
ONErun1790835336778641000 PASS,90fixture/140runner/200overall inclcleanup within
<=240s reservation. Measures existing current,next,prefetch,complete separately;
no profiling hooks added to product. One private unsubmitted EClock counter percase.

Each prep<=1Chipallocation OR256byteupload/128fakewrites, noFastalloc/output;
manual completion and next interval allocate/upload nothing. Callbackcount and
Paula4slot/AmiGUS12channel global startorder retained. All outputs use fake voices;
this manual interval fixture proves no native eventloop/deadline feasibility.

At709379Hz, all observations/outliers retained:2voice current12samples0.467..0.481ms,
next_start3samples0.919..0.921ms,prefetch78samples0.710..31.628ms,
complete3samples3.777..3.806ms,next_live3samples0.895..0.904ms.
16voice current12samples0.467..0.813ms,next_start3samples1.463..1.477ms,
prefetch195samples0.761..31.585ms,complete3samples6.558..6.960ms,
next_live3samples1.225..1.242ms. Complete includes public source validation,
lookahead commit/cancel, ready lease/state validation, callbacks and batch cleanup;
these are public-call buckets, not isolated private-helper costs. Startup versus
manual paths have different call structures; no causal subtraction claim.

66actualFastallocations/zeroownedbytes, actualChiprelease/readers/pins/leases/private
counter/device/requestclosed. Normaldone/rc0+exactrun/launchercleanup and later
independentlocked bridge/68030/running/all4DMAoff/pathabsence PASS. ExplicitRELEASE
acknowledged AmiConnect/forwardedpeers. No retry/lifecycle/alarms/MMIO/actualDMA/
audio/physical; physicalOFF/unprobed/untouched. Earlier bulk failure remainsFAILED.

Next: measure idle pt_mixed_stage_cancel and standalone lookahead-cancel bulk
clearing before an optimization. Code inspection finds whole temporary batch and
lookahead memset at commit; cost significance remains a hypothesis until measured.
Any reduced clearing must preserve resource retirement, fresh reuse and stale/
uncertain-reader guards. Native PLAY/IRQ/device/audio and physical gates remain open.
