from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from literal_source_list import literal_source_list


class LiteralSourceList(unittest.TestCase):
    def test_committed_editor_includes_appended_dependencies(self):
        source = subprocess.check_output(['git', 'show', 'HEAD:tests/test_editor.py'], cwd=ROOT)
        sources = literal_source_list(source, 'SOURCES')
        self.assertEqual(sources[-7:], [
            'src/editor/workflow.c', 'src/editor/sample_range.c',
            'src/editor/wave_summary.c', 'src/core/sample_usage.c',
            'src/core/event_resource.c', 'src/core/flow.c', 'src/core/pitch.c'])

    def test_order_and_reassignment(self):
        self.assertEqual(literal_source_list("S=['old']\nS=['a']\nS+=['b','b']\nS+=('c',)", 'S'), ['a', 'b', 'b', 'c'])

    def test_wrapper_is_not_executed(self):
        self.assertEqual(literal_source_list("raise RuntimeError('must not run')\nS=['a']", 'S'), ['a'])

    def test_dynamic_and_non_string_values_are_refused(self):
        for source in ["S=__import__('os').getcwd()", "S=['a', 1]", "S='a'", "S=['a']\nS+=make_sources()"]:
            with self.subTest(source=source), self.assertRaises(ValueError):
                literal_source_list(source, 'S')

    def test_unsupported_mutations_are_refused(self):
        for source in ["S+=['a']", "S=['a']\nS*=2", "S=['a']\nS.append('b')", "S=['a']\nS[0]='b'", "if True:\n S=['a']", "S=T=['a']", "S=['a']\ndel S"]:
            with self.subTest(source=source), self.assertRaises(ValueError):
                literal_source_list(source, 'S')

    def test_missing_list_is_refused(self):
        with self.assertRaises(ValueError):
            literal_source_list("OTHER=['a']", 'S')
