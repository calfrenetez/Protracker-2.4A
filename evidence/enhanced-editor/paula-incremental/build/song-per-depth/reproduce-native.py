#!/usr/bin/env python3
"""Optional reproduction to a new output directory; never overwrites candidates."""
from pathlib import Path
import argparse,hashlib,json,subprocess
here=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--target',required=True);p.add_argument('--output-dir',type=Path,required=True)
a=p.parse_args();commands=json.loads((here/'compile-commands.json').read_text());m=json.loads((here/'manifest.json').read_text())
command=next((c for c in commands if c['target']==a.target),None)
if command is None:p.error('unknown target; select one of '+', '.join(c['target'] for c in commands))
argv=command['arguments'].copy();folder=Path(command['directory']);binary=a.output_dir/a.target
if binary.resolve()==Path(command['output']).resolve() or binary.exists():p.error('output must be a new file outside the original candidates')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
if digest(Path(argv[0]))!=m['compiler_sha256']:p.error('compiler SHA-256 differs from the qualified build')
for name,sha in m['targets'][a.target]['dependencies'].items():
 if digest(folder/name)!=sha:p.error('source dependency differs: '+name)
a.output_dir.mkdir(parents=True,exist_ok=True);argv[-1]=str(binary.resolve())
subprocess.run(argv,cwd=folder,check=True)
sha=digest(binary);print(binary,sha,'MATCH' if sha==command['binary_sha256'] else 'DIFFERENT')
