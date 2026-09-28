# Host regression results

The isolated staged-source export ran202 tests in430.482 seconds:200 passed;
two Git-dependent helpers could not archive from the nested non-repository export.
Both failed before their C test ran. Their original errors are retained.

Those two checks were rerun from the real checkout using their existing committed/
staged-source isolation: editor EFx reference passed in7.808 seconds and editor
wavetable/studio ownership passed in27.976 seconds. Thus all202 host checks have
passing results across the broad run and the two corrected-environment reruns;
there is no claim that the original broad invocation passed wholesale.

The later native-only startup presentation ordering is compiled and emulator-tested
separately. Host tests are not card, real interrupt, performance or physical proof.
