import asyncio,datetime,fcntl,hashlib,json,sys,shutil
from pathlib import Path
from types import SimpleNamespace
ROOT=Path("/Users/james1/Documents/Codex/2026-09-18/rev/work/Protracker-2.4A")
INFRA=Path("/Users/james1/Documents/Codex/shared-tools/amiga-dev-infra")
REPO=ROOT/"build/dev/qualification-1ac4856"
sys.path.insert(0,str(ROOT/"tools"));sys.path.insert(0,str(INFRA/"scripts"))
import amiga
from shared_guest import Guest
from shared_infra_render_files import require_running_guest
cfg=json.loads((ROOT/"amiga-test.json").read_text())
# Only the existing registered no-argument core cases; no installed profile edits.
cfg={"repo":str(REPO),"artifact":cfg["artifact"],"test_cases":cfg["test_cases"]}
build=json.loads((REPO/"build/dev/core-build.json").read_text())
expected={}
for case in cfg["test_cases"]:
    assert set(case)=={"artifact","expected"}
    p=REPO/case["artifact"];h=hashlib.sha256(p.read_bytes()).hexdigest()
    assert h==build["binaries"][p.name]["sha256"]
    expected[p.name]=h
out=ROOT/"build/dev"/("core-qualification-"+datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ"));out.mkdir()
try:
    with (INFRA/"runtime/test.lock").open("a") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        guest=Guest(INFRA,out);require_running_guest(guest,out,"before-core-test")
        amiga.PROJECTS["protracker"]=cfg
        asyncio.run(asyncio.wait_for(amiga.test(SimpleNamespace(project="protracker",target="amiberry-030",suite="core"),out),300))
        record=json.loads((out/"result.json").read_text())
        assert record.get("passed") is True and record.get("tested_artifacts")==expected
        assert record.get("completed_cases")==list(expected)
        audio=guest.command("GET_AUDIO_STATE")
        assert all("ch%d_dma=0"%i in audio.split("\t") for i in range(4))
        run=guest.share/out.name
        names=sorted(list(expected)+[n+".log" for n in expected])
        assert not run.is_symlink() and sorted(p.name for p in run.iterdir())==names
        for name,h in expected.items():
            p=run/name;assert not p.is_symlink() and hashlib.sha256(p.read_bytes()).hexdigest()==h
        for name in names:
            assert not (run/name).is_symlink()
        for name in names:(run/name).unlink()
        run.rmdir();assert not run.exists() and not guest.launch.exists()
        record.update(source_export_commit="1ac4856",cleanup_audio=audio,run_files_cleaned=True,scope="14 noninteractive core fixtures; no audio or AmiGUS operations")
        (out/"result.json").write_text(json.dumps(record,indent=2)+"\n")
except BaseException as error:
    p=out/"result.json";record=json.loads(p.read_text()) if p.exists() else {}
    record.update(passed=False,error=str(error) or type(error).__name__)
    p.write_text(json.dumps(record,indent=2)+"\n")
    raise
finally:print(out,flush=True)
