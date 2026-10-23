# Private Amiberry coordinate diagnostic

`amiberry-coordinate-trace.patch` observes the unresolved one-pixel difference
between the screenshot pointer and the editor's native mouse coordinate. Apply
it only to a private copy of Amiberry source commit
`8dec73501b5d2c2b3a6859b1d30a25291febb491` with the previously qualified
`mousehack-readiness.patch` already applied. The source archive SHA-256 is
`44b19a0470d0ab05eb4c96e82efaead652bda6b0fb8d361768538c93351bfd70`.
The [HOST receipt](../../docs/test-evidence/amiberry_coordinate_trace_host_20261023.json)
binds the exact production baselines and diagnostic bodies.

From that private source directory:

```sh
patch --batch --fuzz=0 --dry-run -p1 -i /absolute/path/to/amiberry-coordinate-trace.patch
patch --batch --fuzz=0 -p1 -i /absolute/path/to/amiberry-coordinate-trace.patch
```

The patch adds a recorder header and marked observation blocks to three source
files. Removing the marked blocks restores every original byte of those files.
The header here is identical to the header added by the patch. No coordinate
formula, input guard, target command or guest-memory read is added or changed.
The existing synchronous absolute-mouse route alone opens a recording scope;
ordinary host motion cannot start one. Its existing log sink receives at most
eight scopes per process and 32 records per scope. Nested calls are suppressed,
other threads cannot borrow a scope, and scope exit restores prior state.
Host logging adds diagnostic timing overhead.

Each `PT24G_COORD_V1` line has a sequence number and a stage: `IPC_ABS`, `HOST`,
`DRAWABLE`, `GFX`, `DRAW_X`, `DRAW_Y`, `NATIVE`, `BEFORE_COMPENSATION`,
`SCREEN_ORIGIN`, or `DELIVER`. These expose the existing scaling, drawing-origin
and hotspot-compensation values. They contain scalar values, with no addresses
or credentials. Data may include the intermediate first-axis update as well as
the final second-axis update; line order and sequence must be retained.
A future observation needs the complete trace from the same current guarded
request and independent native probe. Existing mismatch vetoes remain required.

Eight compiled HOST tests and syntax checks for all three production translation
units pass, including format checking. The patch also applies with zero fuzz and
round-trips to the reviewed source bodies. No diagnostic emulator has been linked,
installed or run. Private offline configuration returned failure without a useful
error message; an initial thin-header check omitted a readiness helper, and its
corrected check passed with both readiness headers verified against production
hashes. All failed evidence is retained. Available local storage is insufficient
for proceeding responsibly with the larger private build; a build location or
additional free space is pending. The installed shared emulator is unchanged.

HOST checks:

```sh
python3 -B -m unittest discover -s tests -p 'test_amiberry_coordinate_trace.py' -v
```

HOST checks do not establish the native mismatch cause, default system-cursor
hotspot, selection/input acceptance, full editor ABI, hardware or listening
acceptance. A future private binary still needs its own build qualification,
reviewed plan and current shared admission before any target operation.
