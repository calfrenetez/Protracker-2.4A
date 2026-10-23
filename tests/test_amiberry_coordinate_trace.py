"""Compile/run the private diagnostic recorder with a fake original log sink.

No emulator, shared harness, guest memory, GUI input or installed source access.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

HEADER = Path(__file__).resolve().parents[1] / 'tools/diagnostics/amiberry_coordinate_trace.h'

class CoordinateTraceHostTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='pt24g-coordinate-trace-host-')
        cls.root = Path(cls.tmp.name)
        cls.compiler = shutil.which('clang++') or shutil.which('g++')
        if cls.compiler is None:
            cls.tmp.cleanup()
            raise unittest.SkipTest('C++17 host compiler unavailable')
        (cls.root / 'shim.h').write_text('#pragma once\nvoid write_log(const char*, ...);\n#define _T(x) x\n#include "amiberry_coordinate_trace.h"\n')
        shutil.copyfile(HEADER, cls.root / HEADER.name)
        (cls.root / 'other.cpp').write_text('#include "shim.h"\nvoid other() { PT24G_COORD_TRACE("OTHER", "x=%d", 4); }\n')
        (cls.root / 'main.cpp').write_text(r'''
#include "shim.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
std::mutex mutex;
std::vector<std::string> lines;
void other();
void write_log(const char* format, ...) {
    char text[1024]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args);
    std::lock_guard<std::mutex> guard(mutex); lines.emplace_back(text);
}
void emit() { PT24G_COORD_TRACE("TEST", "x=%d y=%d", 218, 424); }
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::string mode(argv[1]);
    if (mode == "inactive") { emit(); other(); }
    else if (mode == "scope") {
        { pt24g_coordinate_trace::Scope scope; emit(); other(); }
        emit(); other();
        if (lines.size() != 2 || lines[0].find("seq=1 stage=TEST x=218 y=424") == std::string::npos || lines[1].find("seq=1 stage=OTHER x=4") == std::string::npos) return 3;
    } else if (mode == "rows") {
        pt24g_coordinate_trace::Scope scope;
        for (int i=0; i<1000; ++i) emit();
    } else if (mode == "budget") {
        for (int i=0; i<1000; ++i) { pt24g_coordinate_trace::Scope scope; emit(); }
        if (pt24g_coordinate_trace::requests != 8) return 4;
    } else if (mode == "nested") {
        pt24g_coordinate_trace::Scope scope; emit();
        { pt24g_coordinate_trace::Scope nested; emit(); }
        emit(); if (pt24g_coordinate_trace::requests != 1) return 5;
    } else if (mode == "unwind") {
        try { pt24g_coordinate_trace::Scope scope; emit(); throw std::runtime_error("fixture"); } catch (const std::runtime_error&) {}
        emit(); if (pt24g_coordinate_trace::active) return 6;
    } else if (mode == "thread-isolation") {
        pt24g_coordinate_trace::Scope scope;
        std::thread worker([] { emit(); other(); }); worker.join(); emit();
    } else if (mode == "concurrent-budget") {
        std::vector<std::thread> workers;
        for (int i=0; i<64; ++i) workers.emplace_back([] { pt24g_coordinate_trace::Scope scope; for (int j=0; j<100; ++j) emit(); });
        for (auto& worker:workers) worker.join();
        if (pt24g_coordinate_trace::requests != 8) return 7;
    } else return 8;
    printf("%zu\n", lines.size());
}
''')
        cls.binary = cls.root / 'fixture'
        cls.compile = subprocess.run([cls.compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pthread', str(cls.root/'main.cpp'), str(cls.root/'other.cpp'), '-o', str(cls.binary)], capture_output=True, text=True, timeout=60)
        if cls.compile.returncode:
            raise AssertionError(cls.compile.stdout + cls.compile.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def check_mode(self, mode, expected):
        result = subprocess.run([str(self.binary), mode], capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, '')
        self.assertEqual(result.stdout.strip(), str(expected))

    def test_inactive_suppresses_every_log(self): self.check_mode('inactive', 0)
    def test_scope_and_cross_translation_unit_share_sequence(self): self.check_mode('scope', 2)
    def test_per_request_rows_are_bounded(self): self.check_mode('rows', 32)
    def test_total_request_budget_saturates(self): self.check_mode('budget', 8)
    def test_nested_request_suppresses_without_losing_outer_scope(self): self.check_mode('nested', 2)
    def test_exception_restores_inactive_state(self): self.check_mode('unwind', 1)
    def test_other_threads_cannot_borrow_active_scope(self): self.check_mode('thread-isolation', 1)
    def test_global_budget_holds_under_concurrent_callers(self): self.check_mode('concurrent-budget', 256)

if __name__ == '__main__': unittest.main()
