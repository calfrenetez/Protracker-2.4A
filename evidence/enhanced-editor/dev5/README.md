# Enhanced editor dev5 evidence

Current binary: `9147adbe89539ced30ddef563c105025275511cf3412eaf62f138c8d0ca3329b`.

The requester, native OS mouse and keyboard editor runs all use this executable.
Screenshots show actual completed native frames. Host tests compare full and
incremental pixels, including copies restricted to the reported dirty rectangles.
Native cursor benchmark: 311 full-render ticks versus 14 incremental ticks for
six moves, identical final pixels. This is drawing time in the private emulated
68030 profile, not physical ACA1234 responsiveness or a 50 Hz claim.

Audio ownership/effect tests remain under dev4 with their original binary identity.
These current runs additionally check changed-note playback and mouse transport.
Physical AmiGUS and the full AmiConnect browser transport are not tested here.
