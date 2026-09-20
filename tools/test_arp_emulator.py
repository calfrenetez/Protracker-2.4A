#!/usr/bin/env python3
"""Reserve Amiberry before running the shared native pitch/PCM harness."""
from make_arp_fixtures import fixtures
from test_porta_emulator import main
if __name__=='__main__':
    main(fixtures,'arp','Pinned native arpeggio stored/output periods captured twice; host/m68k pitch and ramp PCM parity. Includes zero-sentinel refusal, adjacent tuning-table reads, inactive voice, slide interaction and delayed passes. Reference rate/timing only; no physical sound claim.')
