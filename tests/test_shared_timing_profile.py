import importlib.util,json,tempfile,unittest,hashlib
from pathlib import Path
spec=importlib.util.spec_from_file_location('timing',Path(__file__).resolve().parents[1]/'tools/shared_infra_timing.py')
timing=importlib.util.module_from_spec(spec);spec.loader.exec_module(timing)
class TimingProfile(unittest.TestCase):
    def test_no_arguments_and_exact_dedicated_candidate_required(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);build=root/'build/dev';build.mkdir(parents=True)
            candidate=build/timing.BINARY;candidate.write_bytes(b'qualified')
            manifest={'flags':['-DPT_NATIVE_TIMING_ONLY'],'binary_bytes':9,'binary_sha256':hashlib.sha256(b'qualified').hexdigest()}
            path=build/'wavetable-timing-build.json';path.write_text(json.dumps(manifest))
            cfg,_=timing.profile(root)
            self.assertEqual(cfg['test_cases'],[{'artifact':'build/dev/'+timing.BINARY,'expected':timing.EXPECTED}])
            candidate.write_bytes(b'different')
            with self.assertRaises(RuntimeError):timing.profile(root)
            candidate.write_bytes(b'qualified');manifest['flags']=[];path.write_text(json.dumps(manifest))
            with self.assertRaises(RuntimeError):timing.profile(root)
    def test_incomplete_or_extra_files_are_preserved(self):
        with tempfile.TemporaryDirectory() as td:
            run=Path(td);candidate=run/timing.BINARY;candidate.write_bytes(b'qualified')
            log=run/(timing.BINARY+'.log');log.write_text(timing.EXPECTED)
            sha=hashlib.sha256(b'qualified').hexdigest()
            with self.assertRaisesRegex(RuntimeError,'Missing completion'):timing.completed_files(run,sha)
            self.assertTrue(candidate.exists());self.assertTrue(log.exists())
            extra=run/'user-file';extra.write_bytes(b'preserve')
            with self.assertRaisesRegex(RuntimeError,'Unexpected'):timing.completed_files(run,sha)
            self.assertEqual(extra.read_bytes(),b'preserve');self.assertEqual(candidate.read_bytes(),b'qualified')
if __name__=='__main__':unittest.main()
