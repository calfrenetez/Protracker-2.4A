# dev47: glissando reference rendering

E3x enabled for zero-finetune tone portamento. Stored periods remain continuous
while output periods follow native table quantization. Any nonzero control value
enables it; E30 disables without changing speed/target. Existing undefined-zero
period refusal remains. Accepted screen unchanged.

58 regression checks passed in99.758 seconds, including independent16-channel
state checks. Seven fixtures cover both directions, control toggle/nonzero value,
5xx volume slide, delayed passes, arrival and zero target. Native run
gliss1789937078446641000:31 checks in206.013 seconds; 180 fixture
ticks per pass, each trace repeated exactly, native C state and PCM oracle passed.
Host comparison against the retained native evidence is logged separately.
Build/source hashes verified. Guarded release at20:48:35 UTC verified no process,
private-HDF holder or socket and exact launcher restoration; AmiConnect notified.
No physical tests performed.
