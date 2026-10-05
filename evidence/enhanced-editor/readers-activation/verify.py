"""Pure saved-byte reader: no extraction, helper import, producer or product run."""
import ast, hashlib, json, re, struct, tarfile
from pathlib import Path

def require(ok, why):
    if not ok: raise ValueError(why)
def sha(b): return hashlib.sha256(b).hexdigest()
def safe(n): return bool(n) and not n.startswith('/') and '\\' not in n and all(p not in ('', '.', '..') for p in n.split('/'))
def mode(x): return int(x, 8) if isinstance(x, str) else x
def records(x):
    if isinstance(x, dict):
        if {'path', 'bytes', 'sha256'} <= x.keys(): yield x
        else:
            for v in x.values(): yield from records(v)
    elif isinstance(x, list):
        for v in x: yield from records(v)

r = Path(__file__).resolve().parent; m = json.loads((r / 'manifest.json').read_bytes())
p = r / 'payload.tar.gz'; require(p.stat().st_size == m['payload']['bytes'] and sha(p.read_bytes()) == m['payload']['sha256'], 'payload hash')
d = {}
with tarfile.open(p, 'r:gz') as tar:
    for t in tar.getmembers():
        require(t.isfile() and safe(t.name) and t.name not in d and t.name in m['members'], 'unsafe/duplicate/unexpected member')
        require(t.uid == t.gid == t.mtime == 0 and not t.uname and not t.gname, 'noncanonical tar metadata')
        b = tar.extractfile(t).read(); q = m['members'][t.name]
        require(len(b) == q['bytes'] and sha(b) == q['sha256'] and t.mode == q['mode'], 'member changed'); d[t.name] = b
require(set(d) == set(m['members']) and all(n in d for n in m['path_members'].values()), 'membership/alias map')
def load(n): return json.loads(d[n])
def mapped(q):
    n = m['path_members'][q['path']]; b = d[n]
    require(len(b) == q['bytes'] and sha(b) == q['sha256'], 'record changed: ' + q['path'])
    if 'mode' in q: require(m['members'][n]['mode'] == mode(q['mode']), 'record mode')
    return b
def jrec(q): return json.loads(mapped(q))
def check_records(x):
    for q in records(x): mapped(q)
def source_map(f): return f.get('source', f.get('files'))
def lane(x, native=False):
    report, execution = jrec(x['report']), jrec(x['execution'])
    require(execution['state'] == 'completed' and execution['driver_rc'] == x['driver_rc'] and len(execution['durable_copies']) == x['pairs'], 'lane outcome/pairs')
    for pair in execution['durable_copies']: require(mapped(pair['original']) == mapped(pair['durable']), 'original/durable mismatch')
    check_records(report)
    for key in ['compiler', 'python']:
        q = report[key]; require(sha(d[m['path_members'][q['path']]]) == q['sha256'], 'pinned executable hash')
    for product in report.get('products', []):
        q = dict(product, path=product['archive_path']); require(mapped(q) == d[m['path_members'][product['original_path']]], 'product aliases')
    return report

i = load(m['inputs']); require(i['state'] == 'READY_ACTUAL_REVIEWED_INPUTS', 'inputs not approved')
require(m['base_commit'] == i['base_commit'] and m['base_tree'] == i['base_tree'] and m['acceptance'] == i['acceptance'], 'packet scope')
for role, q in i['reviews'].items():
    review = jrec(q['record']); require(review['status'] == q['status'], 'exact review status: ' + role)
    if q.get('require_passed'): require(review.get('passed') is True, 'review failed')
require(set(i['reviews']) == {'root_host', 'independent_host', 'root_native', 'independent_native', 'documentation'}, 'review roles')
for q in i['controls']: mapped(q)
h, n = lane(i['host']), lane(i['native'], True); c = i['counts']; t = n['targets'][i['target']]
f = jrec(i['frozen_source']); fm = source_map(f)
bm = source_map(jrec(i['baseline_manifest']))
require(len(fm) == c['source_files'] and f['base_commit'] == i['base_commit'] and f['base_tree'] == i['base_tree'], 'full source map/base')
require(set(fm) - set(bm) == set(i['owned_paths']) and all(fm[rel] == digest for rel, digest in bm.items()), 'exact additive five-path delta')
fingerprint = sha(json.dumps(fm, sort_keys=True, separators=(',', ':')).encode())
require(h['export_tree_sha256'] == n['source_fingerprint'] == fingerprint, 'full source inventory fingerprint')
for report in [h, n]:
    require(report['passed'] is True and report['source_files_before'] == report['source_files_after'] == c['source_files'], 'stable source/PASS')
    require(report['base_commit'] == i['base_commit'] and report['base_tree'] == i['base_tree'] and source_map(jrec(report['source_manifest'])) == fm, 'proof source binding')
require(len(h['commands']) == c['host_commands'] and all(q['product_returncode'] == 0 for q in h['commands']), 'host RC0')
require(len(h['groups']) == c['host_groups'] and all(q['passed'] for q in h['groups']) and not h['generated_inputs'] and len(h['products']) == c['host_products'], 'host groups/products')
require(len(n['commands']) == c['native_commands'] and all(q['returncode'] == 0 for q in n['commands']) and n['native_NOT_RUN'] is True, 'native compile only RC0')
require(len(h['canonical_dependencies']) == c['host_canonical'] and len(h['external_sdk_dependencies']) == c['host_sdk'] and len(t['dependencies']) == c['native_canonical'] and len(t['external_sdk_dependencies']) == c['native_sdk'] and len(n['runtime_inputs']) == c['native_runtimes'], 'closure counts')
protected = {q['path'] for q in jrec(i['protected_snapshot'])['preserved_files']}
require(not protected & (set(h['canonical_dependencies']) | set(t['dependencies'])), 'protected compiled dependencies')
expected_sources = set(h['canonical_dependencies']) | set(t['dependencies']) | set(i['clean_source_references']) | {q for q in i['owned_paths'] if not q.startswith('docs/')}
require(set(m['source_members']) == expected_sources and not any(q.startswith('docs/') for q in expected_sources), 'source closure/overlay separation')
for rel, name in m['source_members'].items(): require(sha(d[name]) == fm[rel], 'frozen source bytes')
for depmap in [h['canonical_dependencies'], t['dependencies']]:
    for rel, digest in depmap.items(): require(sha(d[m['source_members'][rel]]) == digest, 'compiled source hash')
for depmap in [h['external_sdk_dependencies'], h['sanitizer_runtime_inputs'], h['pinned_sdk_inputs'], t['external_sdk_dependencies']]:
    for path, digest in depmap.items(): require(sha(d[m['path_members'][path]]) == digest, 'SDK/sanitizer closure')
for name, digest in n['runtime_inputs'].items(): require(sha(d[m['path_members'][n['runtime_paths'][name]]]) == digest, 'runtime closure')
hp, np = jrec(h['pins']), jrec(n['pins'])
require(h['compiler'] == hp['host']['compiler'] and h['python'] == hp['host']['python'] and n['compiler'] == np['native']['compiler'] and n['python'] == np['host']['python'], 'actual compiler/Python pins')
require(h['sanitizer_runtime_inputs'] == hp['host']['runtime_inputs'] and h['pinned_sdk_inputs'] == hp['host']['sdk_inputs'] and n['runtime_inputs'] == np['native']['runtime_inputs'], 'actual runtime/SDK pins')
require(sha(d[m['source_members'][np['canonical_recipe']['path']]]) == np['canonical_recipe']['sha256'], 'clean canonical recipe pin')
require(n['flags'] == t['flags'] == n['recipe_contract']['flags'] == i['native_flags'], 'actual native flags')

# Parse only the literal source declaration; never import a fixture or recipe.
recipe = ast.parse(d[m['source_members']['tests/test_readers_activation.py']])
ordered = [ast.literal_eval(q.value) for q in recipe.body if isinstance(q, ast.Assign) and any(isinstance(v, ast.Name) and v.id == 'ACTIVATION_SOURCES' for v in q.targets)]
require(len(ordered) == 1 and len(ordered[0]) == c['translation_units'] and t['source_inputs'] == ordered[0], 'literal TU order')
queries = [q for cmd in h['commands'] for q in cmd['dependency_commands']]
require(len(queries) == c['translation_units'] and [q['unit'] for q in queries] == ordered[0] and all(q['returncode'] == 0 and '-M' in q['argv'] for q in queries), 'host full-M order')
require(all(q['argv'] == [h['compiler']['path'], *i['host_compile_options'], '-M', q['unit']] for q in queries), 'host full-M flags')
native_queries = [q for q in n['commands'] if '-M' in q['argv']]
require(len(native_queries) == c['translation_units'] and [q['argv'][-1] for q in native_queries] == ordered[0], 'native full-M order')
require(all(q['argv'] == [n['compiler']['path'], *i['native_flags'], '-M', unit] for q, unit in zip(native_queries, ordered[0])), 'native full-M flags')
host_argv = h['commands'][0]['executed_argv']
require(host_argv[:-2] == [h['compiler']['path'], *i['host_compile_options'], *ordered[0]] and host_argv[-2] == '-o' and h['commands'][1]['executed_argv'] == [host_argv[-1]], 'host compile/runtime argv edges')
native_argv = [n['compiler']['path'], *i['native_flags'], *ordered[0], '-o', t['binary']['path']]
links = jrec(n['compile_commands'])['commands']
require(len(links) == 1 and links[0]['argv'] == native_argv and links[0]['ordered_translation_units'] == ordered[0] and sum(q['argv'] == native_argv for q in n['commands']) == 1, 'actual native link edge')
require(h['commands'][0]['produced_binary']['sha256'] == h['commands'][1]['input_binary']['sha256'] == h['products'][0]['sha256'], 'compile/runtime binary edge')
runtime = mapped(h['commands'][1]['output_log'])
for marker in i['host_markers']: require(runtime.splitlines().count(marker.encode()) == 1, 'full host marker')
hb = mapped(dict(h['products'][0], path=h['products'][0]['archive_path']))
require(b'___assert_rtn' in hb and b'___asan' in hb and b'___ubsan' in hb, 'host assertion/sanitizer symbol bytes')
binary, cpp = mapped(t['binary']), mapped(t['preprocessed_fixture'])
require(struct.unpack('>I', binary[:4])[0] == 0x3f3 and '__assert_func' in t['last_assert_macro'], 'HUNK/active assert')
assert_counts = [len(re.findall(rb'\b__assert_func\s*\(', cpp)), len(re.findall(rb'^#define assert\([^\n]*', cpp, re.M)), len(re.findall(rb'void __assert_func\s*\(', cpp)), len(re.findall(rb'__assert_func\s*\(\s*"[^"]+"\s*,', re.sub(rb'^#.*\n', b'', cpp, flags=re.M)))]
require(assert_counts == [c['assert_tokens'], c['assert_macros'], c['assert_declarations'], c['assert_expansions']], 'actual enabled assertion counts')
require(binary.count(i['footer'].encode()) == cpp.count(i['footer'].encode()) == 1, 'full CPP/HUNK footer')
for x in i['failed_host_lanes']:
    old = lane(x); sf = source_map(jrec(x['frozen_source'])); jrec(x['classification'])
    require(old['passed'] is False and [q['product_returncode'] for q in old['commands']] == x['returncodes'] and len(old['products']) == x['products'], 'retained failure outcome')
    require(old['source_files_before'] == old['source_files_after'] == len(sf) == c['source_files'] and old['export_tree_sha256'] == sha(json.dumps(sf, sort_keys=True, separators=(',', ':')).encode()), 'retained failed source inventory')
    oq = [q for cmd in old['commands'] for q in cmd['dependency_commands']]
    require(len(oq) == c['translation_units'] and [q['unit'] for q in oq] == ordered[0] and all(q['returncode'] == 0 for q in oq), 'retained failure full-M order')
    if x['products']: require(old['commands'][0]['produced_binary']['sha256'] == old['commands'][1]['input_binary']['sha256'] == old['products'][0]['sha256'], 'retained failed runtime binary edge')
    for rel, digest in old['canonical_dependencies'].items(): require(sha(d[m['history_source_members'][x['name']][rel]]) == digest == sf[rel], 'retained original failed source')
    for cmd in old['commands']:
        if 'output_log' in cmd: require(i['footer'].encode() not in mapped(cmd['output_log']), 'failed lane claims success')
for x in i['source_history']:
    seal = jrec(x['seal']); owned = seal.get('owned_files', seal.get('members')); require(set(owned) == set(i['owned_paths']), 'authored history paths')
    for rel, q in owned.items(): mapped(dict(q, path=str(Path(x['directory']) / rel)))
    for q in x['records']: mapped(q)
    for rel, q in seal['original_dependency_files'].items(): require(sha(mapped(dict(q, path=str(Path(i['source_export']) / rel)))) == fm[rel], 'original dependency binding')
    for rel, q in seal['preserved_origin_files'].items(): mapped(dict(q, path=str(Path(i['origin_directory']) / rel)))
final_owned = jrec(i['source_history'][-1]['seal'])['owned_files']
for rel, q in final_owned.items(): require(q['sha256'] == fm[rel], 'final authored source binding')
refusal = jrec(i['native_pre_dispatch_refusal']); mapped(refusal['driver'])
require(refusal['status'] == 'PRE_DISPATCH_PREPARATION_REFUSAL' and refusal['producer_invocations'] == 0 and refusal['output_exists'] is False, 'pre-dispatch history')
require(set(m['post_build_overlays']) == set(i['post_build_overlays']) == {'docs/READERS_ACTIVATION.md', 'docs/SAMPLE_MEMORY.md'}, 'final prose paths')
for rel, q in i['post_build_overlays'].items(): require(mapped(q) == d[m['post_build_overlays'][rel]], 'final prose bytes')
for key in ['native_execution', 'emulator', 'physical']: require(i['acceptance'][key] == 'NOT_RUN', 'execution scope')
require(i['acceptance']['native_activation_port'] == 'NOT_IMPLEMENTED_OR_QUALIFIED' and i['acceptance']['stack_total'] == 'UNKNOWN' and i['acceptance']['launcher_65536'] == 'NOT_CLEARED', 'hardware/stack scope')
print(json.dumps({'status': 'PASS_SAVED_BYTES_ONLY', 'members': len(d), 'sources': len(m['source_members']), 'host': 'PASS', 'native_compile_link': 'PASS', 'native_execution': 'NOT_RUN', 'stack_total': 'UNKNOWN'}))
