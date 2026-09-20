#!/usr/bin/env python3
"""Generate bounded synthetic MOD flow fixtures; this does not capture native traces."""
import argparse
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
# Events are (pattern, row, channel, effect, parameter). Native behavior must be
# captured, not inferred from the fixture's name or a host implementation.
CASES=[
    ('speed', 'F01/F06/F1F tick counts and F00 stop', [(0,0,0,15,1),(0,1,0,15,6),(0,2,0,15,31),(0,3,0,15,0)], 160),
    ('tempo', 'F20/FFF quantization and stop', [(0,0,0,15,32),(0,1,0,15,255),(0,2,0,15,0)], 80),
    ('two_speed', 'Two speed commands in ascending channel order', [(0,0,0,15,3),(0,0,1,15,9),(0,2,0,15,0)], 100),
    ('stop_then_speed', 'F00 with a later F06 in the same row', [(0,0,0,15,0),(0,0,1,15,6),(0,1,0,15,0)], 80),
    ('jump_then_break', 'B01 then D12 on the same row', [(0,0,0,11,1),(0,0,1,13,0x12),(1,12,0,15,0)], 100),
    ('break_then_jump', 'D12 then B01 on the same row', [(0,0,0,13,0x12),(0,0,1,11,1),(1,0,0,15,0)], 100),
    ('break_range', 'D64 exceeds the 63-row boundary', [(0,0,0,13,0x64),(1,0,0,15,0)], 100),
    ('break_low_nibble', 'D0A exercises native low-nibble decimal arithmetic', [(0,0,0,13,0x0a),(1,10,0,15,0)], 100),
    ('loop', 'E60/E62 loop-counter persistence', [(0,0,0,14,0x60),(0,2,0,14,0x62),(0,3,0,15,0)], 180),
    ('two_loops', 'Different E6 start/count state on two tracks', [(0,0,0,14,0x60),(0,1,1,14,0x60),(0,3,0,14,0x61),(0,3,1,14,0x62),(0,4,0,15,0)], 300),
    ('delay', 'EE2 delayed passes must not refetch/retrigger the row', [(0,0,0,14,0xe2),(0,1,0,15,0)], 100),
    ('delay_break', 'EE2 and D03 shared state interaction', [(0,0,0,14,0xe2),(0,0,1,13,3),(1,3,0,15,0),(1,4,0,15,0)], 160),
    ('last_row_loop', 'E6 at row 63 before natural next-position processing', [(0,0,0,15,1),(0,62,0,14,0x60),(0,63,0,14,0x61),(1,0,0,15,0)], 200),
    ('cross_order_loop', 'E6 start state when entering a different pattern', [(0,0,0,15,1),(0,10,0,14,0x60),(0,11,0,13,0),(1,0,0,14,0x61),(1,11,0,15,0)], 180),
    ('natural_wrap', 'Natural end/wrap is bounded by the harness, not a successful finish', [(0,0,0,15,1)], 180),
    ('backward_jump', 'B00 nontermination is an explicit trace-budget outcome', [(0,0,0,11,0)], 120),
]

def fixtures():
    header=bytearray(1084);header[20:36]=b'FLOW TEST SAMPLE';header[42:44]=(32).to_bytes(2,'big');header[45]=24
    header[48:50]=(32).to_bytes(2,'big');header[950]=2;header[951]=127;header[952:954]=bytes([0,1]);header[1080:1084]=b'M.K.'
    # Deterministic looping triangle. The remaining thirty samples are empty.
    pcm=bytes((v%256 for v in [4*i-64 for i in range(32)]+[64-4*i for i in range(32)]))
    for name,focus,events,budget in CASES:
        h=bytearray(header);h[:20]=name.encode().ljust(20,b'\0');patterns=bytearray(2048)
        patterns[:4]=bytes([1,172,16,0])  # C-2, instrument 1, no effect initially.
        for pattern,row,channel,effect,parameter in events:
            assert 0<=pattern<2 and 0<=row<64 and 0<=channel<4 and 0<=effect<16 and 0<=parameter<256
            offset=(pattern*64*4+row*4+channel)*4
            patterns[offset+2]=(patterns[offset+2]&0xf0)|effect;patterns[offset+3]=parameter
        yield name,bytes(h+patterns+pcm),{'focus':focus,'max_ticks':budget,'events':events,'native_trace':'NOT RUN'}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False);manifest={'schema':1,'status':'INPUT FIXTURES ONLY - NATIVE TRACE NOT RUN','cases':{}}
    for name,data,metadata in fixtures():
        path=args.output/(name+'.mod')
        with path.open('xb') as f:f.write(data)
        manifest['cases'][path.name]={**metadata,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    with (args.output/'manifest.json').open('x') as f:json.dump(manifest,f,indent=2);f.write('\n')
    print(f'Prepared {len(manifest["cases"])} flow fixtures; native traces remain NOT RUN.')
if __name__=='__main__':main()
