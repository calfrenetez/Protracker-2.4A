#!/usr/bin/env python3
"""Read the exact checked-memory packet bytes only. No extraction or execution.
Uncopied compiler/Python/runtime/host-SDK bytes remain provenance, not offline
qualification. Archived test executables and objects are never invoked.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import shlex
import tarfile

MARKER = 'CHECKED MEMORY HOST MODEL PASS: 14 bounded ownership/policy cases; no native placement or source-quiet proof'
FLAGS = ['-std=c99','-m68000','-msoft-float','-mcrt=nix20','-Os','-Wall','-Wextra','-Werror','-Isrc/core','-Ibuild/dev','-fbbb=-']
RUNTIMES = ['ncrt0.o','libnix20.a','libnixmain.a','libnix.a','libstubs.a','libamiga.a','libgcc.a']


def require(ok, message):
    if not ok: raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    p=PurePosixPath(name)
    return bool(name) and not p.is_absolute() and '..' not in p.parts and p.as_posix()==name


def dependencies(text):
    text=re.sub(r'\\\r?\n',' ',text)
    require(':' in text,'Full-M rule missing')
    return shlex.split(text.split(':',1)[1])


def inspect(packet):
    manifest=json.loads((packet/'manifest.json').read_bytes())
    payload=(packet/'payload.tar.gz').read_bytes()
    require(manifest['schema']=='PT_CHECKED_MEMORY_PACKET_V2','Packet schema')
    require(len(payload)==manifest['payload']['bytes'] and sha(payload)==manifest['payload']['sha256'],'Payload binding')
    inputs=manifest['input'];require(inputs['status']=='READY_ALL_ACTUAL_REVIEWS','All actual proof gates')
    require(set(inputs['lanes'])=={'original_host','native_object_original','integrated_host','native_object_current'},'Four separately scoped lanes')
    files=manifest['files'];data={}
    with tarfile.open(fileobj=io.BytesIO(payload),mode='r:gz') as archive:
        for member in archive.getmembers():
            require(member.isfile() and safe(member.name) and member.name not in data and member.name in files,'Traversal/link/duplicate/extra member')
            row=files[member.name];body=archive.extractfile(member).read()
            require(member.mode==row['mode'] and member.size==len(body)==row['bytes'] and sha(body)==row['sha256'],'Member bytes/mode')
            data[member.name]=body
    require(set(data)==set(files),'Exact member closure')
    aliases=manifest['aliases'];expected_aliases={}
    controls=manifest['packet_controls']
    require(set(controls)=={'inputs','approval','generator','verifier','README'},'Exact packet controls')
    selected_rows=inputs['artifacts']+[{'record':row,'roles':['packet_control:'+name]} for name,row in controls.items()]
    selected={row['record']['path']:row for row in selected_rows}
    require(len(selected)==len(selected_rows),'Selected origins unique')
    excluded=set(inputs['unbundled_binary_paths'])
    require(excluded and all(PurePosixPath(path).is_absolute() for path in excluded),'Uncopied binary provenance path set')
    require(not (set(selected)&excluded),'No selected tool/runtime binary bytes')
    for name,row in files.items():
        require(bool(row['origins']) and len({o['record']['path'] for o in row['origins']})==len(row['origins']),'Distinct origins required')
        derived=[]
        for origin in row['origins']:
            r=origin['record'];path=r['path'];require(path in selected and origin==selected[path],'Exact selected origin metadata')
            require(all(r[k]==row[k] for k in ('bytes','sha256','mode')),'Origin byte/mode attribution')
            require(path not in expected_aliases,'Duplicate origin alias');expected_aliases[path]=name;derived+=origin['roles']
        require(row['roles']==sorted(set(derived)),'Derived unique roles')
    require(aliases==expected_aliases and set(aliases)==set(selected),'Every member/origin alias binding')
    ended={}
    for lane in inputs['lanes'].values():
        for r in lane.get('ended_products',[]):
            name=aliases[r['archive_path']]
            require(files[name]['bytes']==r['bytes'] and files[name]['sha256']==r['sha256'],'Ended product archive binding')
            require(r['path'] not in ended or ended[r['path']]==name,'Conflicting ended product alias');ended[r['path']]=name
    require(manifest['ended_aliases']==ended,'Exact ended product aliases; no live-path/mode claim')

    def blob(r):
        path=r['path'];name=aliases[path] if path in aliases else ended[path];row=files[name]
        require(all(row[k]==r[k] for k in ('bytes','sha256') if k in r),'Record byte binding')
        require('mode' not in r or row['mode']==r['mode'],'Record mode binding')
        return data[name]

    def parsed(r): return json.loads(blob(r))

    require(parsed(controls['inputs'])==inputs and controls['inputs']==manifest['inputs'],'Original READY input byte/schema binding')
    approval=parsed(controls['approval'])
    require(approval.get('status')=='AUTHORIZED_EVIDENCE_PACKET_ONLY_ONCE' and all(approval.get(name)==controls[name] for name in ('inputs','generator','verifier','README')),'Actual root packet approval binding')

    def pairs(execution,count):
        require(len(execution['durable_copies'])==count,'Exact saved pair count')
        for pair in execution['durable_copies']:
            a,b=pair['original'],pair['durable'];require(blob(a)==blob(b) and all(a[k]==b[k] for k in ('bytes','sha256','mode')),'Original/durable equality')

    def actual_reviews(lane):
        for key in ('root_review','independent_review'):
            review=parsed(lane[key]);require(str(review.get('status','')).startswith(('CLEAR','PASS')),'Actual root and independent reviews required')
            binding=review.get('bindings',review);reported=binding.get('host_report',binding.get('manifest',binding.get('report')))
            require(reported==lane['report'] and binding.get('execution')==lane['execution'],'Actual review binds this report/execution')

    def source_bindings(frozen,key):
        source=frozen[key]
        for row in source.values():blob(row)
        return source

    def host(lane):
        actual_reviews(lane);report=parsed(lane['report']);execution=parsed(lane['execution']);frozen=parsed(lane['source_manifest'])
        source=source_bindings(frozen,'source')
        require(len(source)==lane['source_count']==report['source_files_before']==report['source_files_after'],'Host source count')
        fingerprint=sha(json.dumps({k:r['sha256'] for k,r in source.items()},sort_keys=True,separators=(',',':')).encode())
        require(report['source_manifest']['sha256']==lane['source_manifest']['sha256'] and report['export_tree_sha256']==fingerprint and report['base_commit']==frozen['base_commit'] and report['base_tree']==frozen['base_tree'],'Host full source identity')
        require(report['passed'] is True and report['modules']==['test_native_checked_memory'] and len(report['groups'])==1 and report['groups'][0]['passed'] is True and report['groups'][0]['tests']==1,'Host group PASS')
        commands=report['commands'];require(len(commands)==2 and all(c['product_returncode']==0 and c['stage']=='complete' for c in commands),'Host compile/runtime RC0')
        compile_command,run_command=commands;argv=compile_command['executed_argv']
        require('-fsanitize=address,undefined' in argv and '-DPT_CHECKED_MEMORY_HOST_ASAN=1' in argv and not any(a.startswith('-DNDEBUG') for a in argv),'Host sanitizer/assert flags')
        require(all(report[k]['path'] in excluded for k in ('compiler','python')) and set(report['sanitizer_runtime_inputs'])<=excluded,'Recorded host compiler/Python/runtime bytes remain uncopied')
        product=compile_command['produced_binary'];require(product==run_command['input_binary']==report['products'][0] and len(report['products'])==1,'Compile to runtime actual binary edge')
        require(argv[argv.index('-o')+1] in ended and run_command['executed_argv'][0] in ended and ended[argv[argv.index('-o')+1]]==ended[run_command['executed_argv'][0]],'Executed binary alias edge')
        executable=blob({'path':product['archive_path'],'bytes':product['bytes'],'sha256':product['sha256']})
        require(MARKER.encode() in executable and b'__assert_rtn' in executable and b'__asan' in executable and b'__ubsan' in executable,'Saved executable enabled assertions/sanitizers/footer')
        output=blob(run_command['output_log']).decode();require(output.splitlines().count(MARKER)==1,'Complete runtime footer once')
        dep=compile_command['dependencies'];require(not dep['generated'] and not report['generated_inputs'],'No generated inputs')
        require(len(report['canonical_dependencies'])==lane['canonical_count'] and len(report['external_sdk_dependencies'])==lane['SDK_count'],'Host canonical/SDK recorded closure counts')
        require(dep['canonical']==report['canonical_dependencies'] and dep['external_sdk']==report['external_sdk_dependencies'],'Host dependency unions')
        seen=set();query_commands=compile_command['dependency_commands'];require(len(query_commands)==lane['full_M_count']==3 and not run_command['dependency_commands'],'Ordered host full-M count')
        for query in query_commands:
            qa=query['argv'];require(query['returncode']==0 and '-M' in qa and qa[-1]==query['unit'],'Actual host full-M argv')
            for raw in dependencies(blob(query['output']).decode()):
                absolute=posixpath.normpath(posixpath.join(query['directory'],raw));require(absolute in lane['dependency_aliases'],'Explicit captured dependency alias required')
                resolved=lane['dependency_aliases'][absolute]
                if resolved.startswith(report['source_export']+'/'):
                    name=resolved[len(report['source_export'])+1:];require(name in dep['canonical'] and source[name]['sha256']==dep['canonical'][name],'Host canonical full-M edge');blob(source[name])
                else:require(resolved in dep['external_sdk'],'Recorded host SDK full-M edge; bytes uncopied')
                seen.add(absolute)
        require(seen==set(lane['dependency_aliases']),'No unused dependency alias')
        require(execution['state']=='completed' and execution['driver_rc']==0 and execution.get('native_emulator_physical')=='NOT_RUN' and execution.get('actual_IRQ')=='NOT_RUN','Host-only execution classification')
        pairs(execution,lane['pair_count'])
        return source

    original=host(inputs['lanes']['original_host']);integrated=host(inputs['lanes']['integrated_host'])
    def native_object(native):
        actual_reviews(native);report=parsed(native['report']);execution=parsed(native['execution']);frozen=parsed(native['frozen_inputs']);source=source_bindings(frozen,'files')
        require(len(source)==report['source_files_before']==report['source_files_after']==5 and report['source_after_matches_before'] is True,'Native exact finite source')
        require(report['status']=='PASS_ORDINARY_OBJECT_LOCAL_ANNOTATIONS_ONLY' and execution['result']==report['status'] and execution['state']=='completed_first_invocation' and execution['helper_invocations']==1,'Actual ordinary-object outcome')
        commands=report['commands'];require(len(commands)==execution['compiler_tool_commands']==native['commands']==18 and all(c['returncode']==0 for c in commands),'18 RC0 native commands')
        command={c['label']:c for c in commands};require(len(command)==18,'Distinct command labels')
        require(frozen['functional_flags']==FLAGS,'Frozen exact functional flags')
        cc=report['tools']['compiler']['path'];unit='src/native/native_checked_memory.c'
        require(command['full-M']['argv']==[cc,*FLAGS,'-M',unit],'Full-M exact functional flags')
        require(command['object']['argv']==[cc,*FLAGS,'-fstack-usage','-c',unit,'-o',report['object']['path']],'Actual annotated object edge')
        require(command['objdump']['argv']==[report['tools']['objdump']['path'],'-dr',report['object']['path']] and command['nm']['argv']==[report['tools']['nm']['path'],'-an',report['object']['path']],'Object inspection edge')
        require(len(report['full_M'])==1,'Native full-M count');dep=report['full_M'][0]
        require(dep['unit']==unit and len(dep['canonical'])==3 and len(dep['SDK'])==39 and dep['SDK']==report['external_sdk_dependencies'],'Native actual canonical/NDK closure')
        seen=set()
        for raw in dependencies(blob(command['full-M']['stdout']).decode()):
            resolved=posixpath.normpath(posixpath.join(command['full-M']['directory'],raw))
            prefix=command['full-M']['directory']+'/'
            if resolved.startswith(prefix):
                rel=resolved[len(prefix):];require(rel in dep['canonical'] and source[rel]['sha256']==dep['canonical'][rel],'Native canonical edge');blob(source[rel])
            else:require(resolved in dep['SDK'],'Exact native real-NDK edge');blob(dep['SDK'][resolved])
            seen.add(resolved)
        require(seen==set(dep['SDK'])|{command['full-M']['directory']+'/'+k for k in dep['canonical']},'Exact native full-M union')
        pins=parsed(frozen['reused_tools']);require(set(report['runtime_paths'])==set(RUNTIMES),'Seven runtime pin records')
        require(set(report['tools'])==set(pins['tools']) and len(report['tools'])==7,'Seven separately pinned tool metadata records')
        for name,expected in pins['tools'].items():
            require(all(report['tools'][name].get(k)==value for k,value in expected.items()),'Recorded tool metadata matches actual saved pins; executable bytes uncopied')
        require(set(pins['runtime_inputs'])==set(RUNTIMES),'Exact recorded runtime-name hashes')
        require(set(report['runtime_paths'].values())<=excluded and all(tool[k] in excluded for tool in report['tools'].values() for k in ('path','resolved_path') if k in tool),'Recorded native tools/runtime files remain uncopied')
        require(all(any(path.startswith(root+'/') for root in pins['allowed_sdk_roots']) for path in dep['SDK']),'Real NDK closure lies inside recorded roots')
        for name in RUNTIMES:
            query=command['runtime-'+name.replace('.','-')]
            require(query['argv']==[cc,'-m68000','-msoft-float','-mcrt=nix20','-print-file-name='+name],'Runtime query argv')
            raw=blob(query['stdout']).decode().strip();require(raw.startswith('/') and posixpath.normpath(raw)==report['runtime_paths'][name] and name in pins['runtime_inputs'],'Actual runtime raw-query lexical normalization; libraries uncopied')
        local=[]
        for line in blob(report['object_records']['stack_usage']).decode().splitlines():
            cells=line.split('\t');require(len(cells)==3 and cells[0] and re.fullmatch('[0-9]+',cells[1]),'Local .su row')
            q=cells[2].split(',');require(len(set(q))==len(q) and set(q) in ({'static'},{'dynamic'},{'dynamic','bounded'}),'Local .su qualifier')
            local.append({'compiler_location_function':cells[0],'reported_bytes':int(cells[1]),'qualifiers':q,'local_numeric_bound':'static' in q or 'bounded' in q})
        require(local==report['local_stack_rows'] and len(local)==15 and max(r['reported_bytes'] for r in local)==512,'15 exact local annotations, largest local only')
        require('file format' in blob(report['object_records']['disassembly']).decode() and blob(report['object_records']['symbols']).strip(),'Saved object decode/symbol bytes')
        require(blob(report['object'])==blob(report['object_records']['object']),'Actual object records')
        require(report['link'] is False and report['HUNK'] is False and report['product_execution']=='NOT_RUN' and report['native_entry_IRQ_emulator_physical']=='NOT_RUN','Native never-executed/no-link scope')
        require(report['task_stack_total']==report['system_IRQ_stack_total']=='UNKNOWN' and report['launcher65536']=='NOT_CLEARED' and execution['native_product_IRQ_target']=='NOT_RUN','No aggregate stack or runtime acceptance')
        pairs(execution,39)
        return source

    original_object=native_object(inputs['lanes']['native_object_original'])
    current_object=native_object(inputs['lanes']['native_object_current'])
    require(len(inputs['source_map'])==9 and len(inputs['new_repo_sources'])==8 and set(inputs['new_repo_sources'])==set(inputs['source_map'])-{'src/native/master_memory.h'},'Eight additive sources plus policy')
    for name,row in inputs['source_map'].items():require(row==integrated[name],'Published source exact integrated layout');blob(row)
    for name in ('native_checked_memory.c','native_checked_memory.h'):
        require(original_object['src/native/'+name]['sha256']==original['tests/production/'+name]['sha256'],'Historical host and object bind exact old C/H')
        require(current_object['src/native/'+name]['sha256']==integrated['src/native/'+name]['sha256'],'Current host and object bind exact current C/H')
    require(original_object['src/native/native_checked_memory.c']['sha256']==current_object['src/native/native_checked_memory.c']['sha256'],'Production C unchanged across separate lanes')
    revision=inputs['header_revision'];require(set(revision)=={'source_review','original_header','current_header','unchanged_C'},'Explicit comment-only header relation')
    clarification=parsed(revision['source_review']);require(str(clarification.get('status','')).startswith(('CLEAR','PASS')),'Pinned source clarification review')
    require(revision['original_header']==original_object['src/native/native_checked_memory.h'] and revision['current_header']==current_object['src/native/native_checked_memory.h'] and revision['unchanged_C']==current_object['src/native/native_checked_memory.c'],'Clarification exact object-source binding')
    def c_tokens(body):
        # Preserve literals and every non-whitespace byte; comments alone disappear.
        pattern=r""""(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|/\*.*?\*/|//[^\n]*|[^\s]"""
        return tuple(t for t in re.findall(pattern,body.decode(),re.S) if not t.startswith(('/*','//')))
    old_header,new_header=blob(revision['original_header']),blob(revision['current_header'])
    require(old_header!=new_header and c_tokens(old_header)==c_tokens(new_header),'Only comment/whitespace header change, no API/layout token change')
    require(original_object['src/native/master_memory.h']['sha256']==current_object['src/native/master_memory.h']['sha256']==integrated['src/native/master_memory.h']['sha256'],'Original master policy unchanged')
    for row in inputs['final_docs'].values():blob(row)
    require(set(inputs['final_docs'])=={'docs/NATIVE_CHECKED_MEMORY.md','docs/SAMPLE_MEMORY.md'},'Final documentation pins')
    require(inputs['scope_flags']['native_entry_IRQ_emulator_physical']=='NOT_RUN' and inputs['scope_flags']['launcher65536']=='NOT_CLEARED','Final packet scope')
    require(set(p.name for p in packet.iterdir())=={'README.md','manifest.json','payload.tar.gz','verify.py','SHA256SUMS'},'Exact five packet controls')
    expected_sums=''.join(sha((packet/name).read_bytes())+'  '+name+'\n' for name in ['README.md','manifest.json','payload.tar.gz','verify.py'])
    require((packet/'SHA256SUMS').read_text()==expected_sums,'Exact external SHA list')
    require((packet/'verify.py').read_bytes()==blob(manifest['verifier']) and (packet/'README.md').read_bytes()==blob(manifest['README']),'External verifier/README bindings')
    return {'status':'PASS_SAVED_PACKET_BYTES_ONLY','members':len(data),'origins':len(aliases),'sources':9,'native_commands_per_separate_lane':18,'native_pairs_per_separate_lane':39,'native_lanes':2,'native_SDK_headers':39,'local_SU_rows':15,'native_entry_IRQ_emulator_physical':'NOT_RUN','task_system_stack':'UNKNOWN;65536_NOT_CLEARED','uncopied_tools_runtime_host_SDK':'PROVENANCE_ONLY_NOT_OFFLINE_QUALIFIED'}


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packet',type=Path,required=True);args=parser.parse_args()
    print(json.dumps(inspect(args.packet),sort_keys=True))


if __name__=='__main__': main()
