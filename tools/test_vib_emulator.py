#!/usr/bin/env python3
"""Reserve Amiberry before running the shared native pitch/PCM harness."""
from make_vib_fixtures import fixtures
from test_porta_emulator import main
if __name__=='__main__':
    main(fixtures,'vib','Pinned native vibrato period writes captured twice, with host/m68k stored/output word and PCM parity. Covers waveform selection, independent speed/depth memory, reset/no-reset, tone interaction, volume slides, delayed passes and wrapped periods. Reference rate/timing only; no physical sound claim.')
