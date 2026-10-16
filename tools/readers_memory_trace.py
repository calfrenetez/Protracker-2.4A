"""Exact finite stdout and final closed-sidecar contract for one reader fixture.

No target access or acceptance from a partial phase report. The backend remains
a serialized software model even when allocations use real Exec memory classes.
"""
POOL = 'EXEC MEMORY POOL PASS: Fast-only pool, reserve and zero-budget refusal'
READERS = 'SAMPLER PAULA READERS PASS: genuine master and selective Chip cache retained through 20 controls with command capacity2'
LIFECYCLE = 'SAMPLER PAULA READERS LIFECYCLE PASS: six8/16/24 mono/stereo cases,20controls/capacity2, explicit STOP and retirement; negative matrix excluded'
FAULTS = 'SAMPLER PAULA READERS FAULT MATRIX PASS: retirement races, cancellation, allocation pressure, alias/reentry/stale-key refusal; lifecycle sequence excluded'
CHIP = 'EXEC READERS CHIP PASS: Chip-only cache extents, zero owned bytes; modeled backend, no DMA or audio'
FINISH = 'EXEC MEMORY PASS: Fast allocations, zero owned bytes, budget refusal without Chip fallback'


def phase(stage, x=0, y=0, n=0):
    return f'READERS MEMORY TRACE:{stage}:{x}:{y}:{n}'


def lifecycle_phases():
    lines = []
    for bits in (8, 16, 24):
        for channels in (1, 2):
            lines += [phase(s, bits, channels) for s in
                      ('case-begin', 'case-initialized', 'trigger-prepared', 'trigger-active')]
            lines += [phase('control-complete', bits, channels, n) for n in (1, 5, 10, 15, 20)]
            lines += [phase(s, bits, channels, 20) for s in
                      ('stop-active', 'retirement-observed', 'case-finished')]
    return lines


def fault_phases():
    lines = [phase('retire-race', a, b) for a in (0, 1) for b in (0, 1)]
    lines += [phase(s) for s in ('replacement_race', 'version_race', 'control_cancels_and_pressure',
        'all_phase_cancel', 'allocation_failures', 'alias_guards', 'deferred_retirement',
        'address_reuse', 'domain_failures', 'reentry_cases', 'forged_keys_and_metadata', 'extras-finished')]
    return lines


def completion(component):
    return {'full': READERS, 'lifecycle': LIFECYCLE, 'faults': FAULTS}[component]


def markers(component='full'):
    # Invalid components refuse here, never silently omit either group.
    passed = completion(component)
    lines = [phase('entered'), POOL, phase('memory-started')]
    if component != 'faults':
        lines += lifecycle_phases()
    if component != 'lifecycle':
        lines += fault_phases()
    return tuple(lines + [passed, phase('chip-closed'), CHIP, FINISH, phase('finished')])


FINAL_SIDECAR = (phase('finished') + '\n').encode('ascii')
