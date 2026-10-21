import struct
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from native_hunk_routes import parse,symbol,references,pointer

def words(*v):return struct.pack('>'+'I'*len(v),*v)
def name(n):
    b=n.encode('ascii');b+=b'\0'*(-len(b)%4)
    return words(len(b)//4)+b
def fixture(reloc=(1,1,0,0),addend=4,sym='alpha'):
    # The two CODE sections deliberately have equal offsets and independent symbols.
    return (words(1011,0,2,0,1,2,2,1001,2,addend,0x4e714e75,1004,*reloc,1008)+
            name(sym)+words(4,0,1010,1001,2,0,0x4e714e75,1008)+
            name('beta')+words(4,0,1010))

class Routes(unittest.TestCase):
    def test_equal_vma_routes_to_correct_section(self):
        s=parse(fixture())
        self.assertEqual(symbol(s,'alpha'),(0,4))
        self.assertEqual(symbol(s,'beta'),(1,4))
        self.assertEqual(references(s,'alpha'),[])
        self.assertEqual(references(s,'beta'),[{'section':0,'offset':0,'targetSection':1,'targetOffset':4}])
    def test_pointer_symbol_requires_its_own_relocation(self):
        s=parse(fixture());s[0]['symbols']['slot']=0
        self.assertEqual(pointer(s,'slot'),(1,4))
        with self.assertRaises(ValueError):pointer(s,'alpha')
    def test_ambiguous_cross_section_symbol_refuses(self):
        s=parse(fixture(sym='beta'))
        with self.assertRaises(ValueError):symbol(s,'beta')
    def test_bss_and_zero_length_end_symbol(self):
        s=parse(words(1011,0,1,0,0,1,1003,1,1008)+name('end')+words(4,0,1010))
        self.assertEqual(s[0]['body'],b'');self.assertEqual(symbol(s,'end'),(0,4))
    def test_debug_preserved_as_bounded_digest(self):
        s=parse(words(1011,0,1,0,0,1,1001,1,0x4e714e75,1009,1,42,1010))
        self.assertEqual(s[0]['debug'][0]['bytes'],4)
    def test_chip_allocation_is_not_confused_with_record_kind(self):
        s=parse(words(1011,0,1,0,0,0x40000001,1001,1,0x4e714e75,1010))
        self.assertEqual(s[0]['allocation'],0x40000001)
    def test_malformed_records_refuse(self):
        valid=fixture()
        cases={
            'short':valid[:-4], 'unaligned':valid+b'x', 'trailing':valid+words(0),
            'wrong_header':words(1)+valid[4:], 'named_header':words(1011,1)+valid[8:],
            'table_first':valid[:12]+words(1)+valid[16:],
            'table_last':valid[:16]+words(7)+valid[20:],
            'table_count':valid[:8]+words(5000)+valid[12:],
            'extended_allocation':valid[:20]+words(0xc0000002)+valid[24:],
            'unknown_kind':valid[:28]+words(1234)+valid[32:],
            'exceeds_allocation':valid[:32]+words(3)+valid[36:],
            'unknown_record':valid[:44]+words(1234)+valid[48:],
            'bad_target':fixture(reloc=(1,2,0,0)),
            'odd_offset':fixture(reloc=(1,1,1,0)),
            'out_of_range_offset':fixture(reloc=(1,1,6,0)),
            'duplicate_offset':fixture(reloc=(2,1,0,0,0)),
            'bad_target_addend':fixture(addend=12),
            'huge_relocation_group':fixture(reloc=(0xffffffff,1,0,0)),
        }
        for n,d in cases.items():
            with self.subTest(case=n):
                with self.assertRaises((ValueError,UnicodeDecodeError)):parse(d)
    def test_symbol_errors_refuse(self):
        prefix=words(1011,0,1,0,0,1,1001,1,0,1008)
        cases=[name('x')+words(0)+name('x')+words(0,0,1010),
               name('x')+words(5,0,1010),words(1)+b'x\0y\0'+words(0,0,1010),
               words(1)+b'\xff\0\0\0'+words(0,0,1010),words(99999)]
        for d in cases:
            with self.subTest(data=d[:12].hex()):
                with self.assertRaises((ValueError,UnicodeDecodeError)):parse(prefix+d)

if __name__=='__main__':unittest.main()
