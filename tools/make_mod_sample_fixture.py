"""Create our synthetic two-instrument MOD for import tests."""
from pathlib import Path

def make(baseline):
    data=bytearray(baseline);second=50
    data[second:second+22]=b'SOURCE TWO'+bytes(12)
    data[second+22:second+30]=bytes([0,16,13,48,0,4,0,8])
    data.extend(bytes((i*7+128)%256 for i in range(32)))
    return bytes(data)

if __name__=='__main__':
    import sys
    Path(sys.argv[2]).write_bytes(make(Path(sys.argv[1]).read_bytes()))
