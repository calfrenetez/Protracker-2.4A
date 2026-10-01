# Idle mixed batch and lookahead clearing costs

1 October 2026. Production unchanged; native6manual component scenarios at8/16/24
and2/16voices each add4idle mixed-stage cancels AFTER completed/retired batch plus
4standalone initially-zeroed lookahead cancels. LivePaula/AmiGUS voice structs
remain byte-identical after idle cancel; currentowner validates. All48cancelcalls
allocate/upload/emit nothing. Separate Fastworkspace fully zero after every
lookahead cancel, released before ownerclose. Hostfull195ASan/UBSan PASS9.821s.
Native cases and native-only cancellation assertions are distinct from host195.

162indexedsourcehashes verified candidate/tree8a54ba20100532bb3b265c96b7b2dc45ed7994e4,
zero generated/3verifiedbinaries. Native235072bytesSHA256
62854df68b24930e37a184daf1ab70b9da38fcea38157241ad709f025cfff718.
ONErun1790835644217211000 PASS;90fixture/140runner/200overall+cleanup within<=240s.
72actualFastallocations/zeroownedbytes,Chiprelease/readers/pins/leases/workspace/
privateunsubmittedEClockcounter/device/requestclosed. Normaldone/rc0,exactowned
run+launchercleanup,independentlaterlocked bridge/68030/running/all4DMAoff/path
absencePASS; RELEASE acknowledgedAmiConnect/forwardedpeers. No hold/lifecycle/
retry/alarms/MMIO/actualDMA/audio/physical;physicalOFF/unprobed/untouched.

At709379Hz(alloutliersretained):24idle batch cancels1.489..1.837ms;
24lookahead5612byte cancels0.323..0.331ms. Original6component calls also repeated:
current0.467..0.825ms,next_start0.932..1.511ms,prefetch0.718..31.745ms,
complete3.774..6.668ms,next_live0.904..1.245ms. Isolated idle cancellation does not
by itself measure active job/lease release cost; no real-time/output/hardware proof.
Code review: Paula cancellation already retires job/unstarted leases selectively;
mixedstage then memsets its wholeprivate batch. Next investigate selective private
batch reset after releasing every resource, invalidating phase/index/count and
plan counts while preserving held=0 and complete overwrite on reuse. Lookahead's
public fullzero contract remains unchanged. Guards/fault/strictdeadlines stay intact.
