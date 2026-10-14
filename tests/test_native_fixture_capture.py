"""Capture checks cannot infer native/Shell interleaving from shared stdout."""
import importlib.util
from pathlib import Path
import unittest
import zlib

spec = importlib.util.spec_from_file_location("capture", Path(__file__).resolve().parents[1] / "tools/native_fixture_capture.py")
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)


def dump(data, offset=0):
    return "\n".join(f"{offset+i:08x}  " + " ".join(f"{x:02x}" for x in data[i:i+16]).ljust(48)
                     + "  " + "".join(chr(x) if 32 <= x < 127 else "." for x in data[i:i+16])
                     for i in range(0, len(data), 16))


class Capture(unittest.TestCase):
    prefix = "RUN-"
    control = "[OK]\nRUN-BEFORE\nRUN-STACK-QUERY-BEGIN\nCurrent stack size is 262144 bytes\nRUN-STACK-QUERY-END\nRUN-NATIVE-BEGIN\nRUN-NATIVE-RC 0\nRUN-NATIVE-END"
    markers = ["FIRST PASS", "SECOND PASS"]

    def test_same_shell_redirect_and_existing_file_refusal(self):
        script = c.run_script("RAM:run/test", "RAM:run/native.out", 262144, self.prefix)
        self.assertEqual(script.count("RAM:run/test >RAM:run/native.out\n"), 1)
        self.assertTrue(script.startswith("If EXISTS RAM:run/native.out\n Echo RUN-CAPTURE-EXISTS\n Quit 20\nEndIf\n"))
        self.assertLess(script.index("\nStack\nIf WARN"), script.index("RAM:run/test >"))
        self.assertIn("Stack 262144\nIf WARN\n", script)
        self.assertIn("RAM:run/test >RAM:run/native.out\nEcho RUN-NATIVE-RC $RC\n", script)

    def test_unsafe_paths_and_sizes(self):
        for remote, output, size, prefix in [
            ("RAM:run/test\nDelete SYS:", "RAM:run/out", 1, "RUN-"),
            ("RAM:run/test", "RAM:elsewhere/out", 1, "RUN-"),
            ("RAM:run/test", "RAM:run/TEST", 1, "RUN-"),
            ("RAM:run/test", "RAM:run/out", True, "RUN-"),
            ("RAM:run/test", "RAM:run/out", 0, "RUN-"),
            ("RAM:run/test", "RAM:run/out", 1, "X;Echo "),
        ]:
            with self.subTest(remote=remote, output=output, size=size, prefix=prefix), self.assertRaises(ValueError):
                c.run_script(remote, output, size, prefix)

    def test_shell_markers_stack_and_rc_remain_strict(self):
        self.assertEqual(c.parse_control(self.control, 262144, self.prefix)["returnCode"], 0)
        for bad in [self.control.replace("NATIVE-RC 0", "NATIVE-RC 5"),
                    self.control.replace("262144", "65536"),
                    self.control.replace("Current stack", "123 Current stack"),
                    self.control.replace("RUN-NATIVE-BEGIN\n", ""),
                    self.control + "\nRUN-NATIVE-END", "[STILL RUNNING]\n" + self.control,
                    "FIRST PASS\n" + self.control, self.control + "\nFIRST PASS",
                    self.control.replace("RUN-NATIVE-BEGIN", "RUN-STACK-SET-WARN")]:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                c.parse_control(bad, 262144, self.prefix)

    def test_bounded_checksum_path_and_bytes(self):
        value = c.parse_checksum("File: RAM:run/out\nCRC32: 1234ABCD\nSize: 8192 bytes", "RAM:run/out")
        self.assertEqual(value, {"crc32": 0x1234abcd, "size": 8192})
        for bad in ["File: RAM:other/out\nCRC32: 1234ABCD\nSize: 2 bytes",
                    "File: RAM:run/out\nCRC32: 1234ABCD\nSize: 0 bytes",
                    "File: RAM:run/out\nCRC32: 1234ABCD\nSize: 8193 bytes",
                    "File: RAM:run/out\nCRC32: 1234ABCD\nSize: 2 bytes\nSize: 2 bytes"]:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                c.parse_checksum(bad, "RAM:run/out")

    def test_readfile_chunks_and_corruption(self):
        data = bytes(range(256)) * 17
        for offset in (0, 4096):
            chunk = data[offset:offset+4096]
            self.assertEqual(c.decode_read(dump(chunk, offset), offset, len(chunk)), chunk)
        raw = dump(b"FIRST PASS\n")
        for bad in [raw.replace("00000000", "00000001"), raw[:-1], raw+"\n"+raw,
                    raw.replace("46 49", "46 zz"), raw[:-1]+"X"]:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                c.decode_read(bad, 0, 11)

    def test_native_markers_independent_of_shell_buffer(self):
        data = b"FIRST PASS\nSECOND PASS\n"
        check = {"size": len(data), "crc32": zlib.crc32(data)}
        self.assertEqual(c.validate_native(data, check, self.markers)["nativeMarkers"], self.markers)
        for bad in [b"SECOND PASS\nFIRST PASS\n", b"FIRST PASS\n", data+data,
                    data+b"unexpected\n", data+b"\xff", data+b" "]:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                c.validate_native(bad, {"size": len(bad), "crc32": zlib.crc32(bad)}, self.markers)
        with self.assertRaises(ValueError):
            c.validate_native(data, dict(check, crc32=0), self.markers)


if __name__ == "__main__":
    unittest.main()
