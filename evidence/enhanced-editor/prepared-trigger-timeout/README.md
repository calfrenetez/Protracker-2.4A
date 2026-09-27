# Preserved 90-second timeout and late completion

The first bounded-trigger preparation candidate (407776 bytes, SHA256
319fe845c737ad1f4545d4c02b5686f15d494b619f3665e0e70863026c0665a3) exceeded the
90-second outer full-suite window. The original result remains passed:false.
No reset or process retry was performed. The exact run subsequently produced
done, RC0, all fixture assertions and1241Fast allocations/zeroowned/noChipfallback.

Guarded recovery took the shared nonblocking lock and revalidated live030/DevBench
identity, candidate checksum, exact completed file set, completion/RC/final markers
and all4DMAoff. Full logs, original result and launcher were preserved before
cleaning only that completed run/launcher. Independent absence was checked and
AmiConnect was explicitly notified of release/cleared recovery hold. This is late
functional completion and cleanup evidence, NOT a90second pass.

This first candidate added a redundant source_current check inside the command
copy after mandatory source_location had just checked it, with no intervening
external callback. Its16-voice cost83831ticks exceeded the107ms baseline. Host
review removed only this newly introduced duplicate check; all original guards
remain. This retained evidence does not qualify that subsequent candidate.
