# Causal publication and cleanup qualification

The bounded two-trigger scheduler now has a separate 56-case HOST fixture for
publication failures and actual allocation/release behavior. The original
26-case causal fixture remains a separate acceptance gate. Production code
was unchanged by this qualification.

The fixture uses genuine 24-bit masters, selective Paula caches, 16-bit AmiGUS
cache holders, and the actual causal owner and mixed queue. Forty-eight cases
exercise first and successor publication in both command-first and reader-first
quiet-proof orders. They cover clean refusal, unknown and malformed replies,
identity/packet mutation, reentry, explicit fault, and late post-clock observation.
Faulting raw-zero replies retain ownership; they cannot establish absence. A
clean-zero retry is a separate explicit fixture action while its original deadline
is future. No automatic retry or schedule rebasing is introduced.

Six constructor cases exercise real allocation failures, construction reentry,
explicit fault, and queue-before-owner cleanup. Two release cases exercise a
close returning zero after actually consuming a queue or owner slot. Sample
master beforeimages and exact release accounting are checked throughout.

The one isolated HOST run compiled 27 actual source units with strict C99,
assertions, AddressSanitizer and UndefinedBehaviorSanitizer. Compile and run
returned zero in 7.716 and 0.426 seconds. The complete 404-byte oracle matched;
compiler and error streams were empty; both HOST process groups were absent
after reaping. Source, donor and unrelated protected paths were unchanged.
The saved evidence is in
`evidence/enhanced-editor/mixed-readers-causal-publication-host/`.

The standalone recipe is encoded in
`tests/test_mixed_readers_causal_publication.py`; Root executed its exact compile
and fixture recipe directly through the isolated collector, without importing
that driver. No additional run is implied.

This is software memory-ownership evidence. The fixture has not run in Amiberry
or on the A1200. It does not qualify timer/CIA/IRQ timing, actual device transfers,
voice stop, paired hardware atomicity, audio or human listening. The defensive
constructor branch that retains a newborn queue after unconsumed refused cleanup
remains unqualified: no conforming public callback route to it was demonstrated.

The causal owner still supports one bounded pure-trigger pair. Whole-song
rolling refill and future CONTROL/STOP integration remain separate development
work; the application default activation and classic PLAY are unchanged.
