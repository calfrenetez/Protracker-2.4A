# Literal-only test encoder: this is a synthetic stream generator, not a production compressor.
from pathlib import Path

def literal(data,skip=0):
 bits=[0]*skip
 def put(value,n):bits.extend((value>>shift)&1 for shift in range(n-1,-1,-1))
 put(0,1);count=len(data)-1
 while count>=3:put(3,2);count-=3
 put(count,2)
 for value in data[::-1]:put(value,8)
 # Consumer walks bytes backwards, low bit first in each byte.
 bits.extend([0]*(-len(bits)%32))
 packed=bytes(sum(bits[i+j]<<j for j in range(8)) for i in range(0,len(bits),8))[::-1]
 return b'PP20'+bytes([9,10,12,13])+packed+len(data).to_bytes(3,'big')+bytes([skip])
def match_fixture(length, long_offset=False, offset=0, output_size=None):
 # One literal A followed by an overlapping match. Independent test bit writer.
 bits=[]
 def put(value,n):bits.extend((value>>shift)&1 for shift in range(n-1,-1,-1))
 put(0,1);put(0,2);put(65,8)
 code=min(length-2,3);put(code,2)
 if code==3:
  put(int(long_offset),1);put(offset,13 if long_offset else 7);remaining=length-5
  while remaining>=7:put(7,3);remaining-=7
  put(remaining,3)
 else:put(offset,[9,10,12][code])
 bits.extend([0]*(-len(bits)%32))
 packed=bytes(sum(bits[i+j]<<j for j in range(8)) for i in range(0,len(bits),8))[::-1]
 return b'PP20'+bytes([9,10,12,13])+packed+(length+1 if output_size is None else output_size).to_bytes(3,'big')+b'\0'

if __name__=='__main__':
 import sys
 Path(sys.argv[2]).write_bytes(literal(Path(sys.argv[1]).read_bytes()))
