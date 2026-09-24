from pathlib import Path
import ctypes
import importlib.util
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/pp20_test.c','src/platform/pp20_import.c','src/core/document.c','src/core/pp20.c','src/core/mod_project.c','src/core/mod_inspect.c','src/core/project.c','src/core/channels.c','src/core/pcm.c']
class PP20(unittest.TestCase):
    def test_bounded_decode_and_atomic_document(self):
        spec=importlib.util.spec_from_file_location('ppfixture',ROOT/'tools/make_pp20_fixture.py');encoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(encoder)
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);binary=tmp/'pp20';packed=tmp/'baseline.pp';packed.write_bytes(encoder.literal((ROOT/'evidence/baseline/mod.baseline').read_bytes()))
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/core',*SOURCES,'-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(packed),str(ROOT/'evidence/baseline/mod.baseline')],check=True)
            subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Dread=pt_test_read','-Isrc/core',*SOURCES,'tests/render_read_faults.c','-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary),str(packed),str(ROOT/'evidence/baseline/mod.baseline')],check=True)
            # Independent Python bit writer exercises every skip count and short literal length.
            libpath=tmp/'pp20.dylib'
            subprocess.run(['cc','-std=c99','-O1','-shared','-fPIC','-Isrc/core','src/core/pp20.c','-o',str(libpath)],cwd=ROOT,check=True)
            lib=ctypes.CDLL(str(libpath));lib.pt_pp20_decode.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
            callback=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t)
            lib.pt_pp20_decode_reader.argtypes=[callback,ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
            original_decode=lib.pt_pp20_decode
            def both(src,length,out,capacity,written):
                expected=original_decode(src,length,out,capacity,written)
                alternate=ctypes.create_string_buffer(b'Z'*capacity,capacity);w2=ctypes.c_size_t(99)
                @callback
                def read(context,offset,dest,count):
                    if count>256 or offset+count>length:return 0
                    ctypes.memmove(dest,src[offset:offset+count],count);return 1
                actual=lib.pt_pp20_decode_reader(read,None,length,alternate,capacity,ctypes.byref(w2))
                self.assertEqual(actual,expected)
                if expected==0:
                    self.assertEqual(alternate.raw,out.raw)
                    self.assertEqual(w2.value,ctypes.cast(written,ctypes.POINTER(ctypes.c_size_t))[0])
                else:self.assertEqual(w2.value,99)
                return expected
            lib.pt_pp20_decode=both
            for skip in range(33):
                for count in range(1,65):
                    data=bytes((i*37+3)%256 for i in range(count));src=encoder.literal(data,skip);out=ctypes.create_string_buffer(count);w=ctypes.c_size_t()
                    self.assertEqual(lib.pt_pp20_decode(src,len(src),out,count,ctypes.byref(w)),0)
                    self.assertEqual(out.raw,data);self.assertEqual(w.value,count)

            for length in [2,3,4,5,6,11,12,13,19,100]:
                for long_offset in [False,True]:
                    src=encoder.match_fixture(length,long_offset);out=ctypes.create_string_buffer(length+1);w=ctypes.c_size_t()
                    self.assertEqual(lib.pt_pp20_decode(src,len(src),out,len(out),ctypes.byref(w)),0)
                    self.assertEqual(out.raw,b'A'*(length+1))
                    for bad in [encoder.match_fixture(length,long_offset,offset=1),encoder.match_fixture(length,long_offset,output_size=length)]:
                        out=ctypes.create_string_buffer(b'Z'*(length+1),length+1);w.value=99
                        self.assertNotEqual(lib.pt_pp20_decode(bad,len(bad),out,len(out),ctypes.byref(w)),0)
                        self.assertEqual(out.raw,b'Z'*(length+1));self.assertEqual(w.value,99)
