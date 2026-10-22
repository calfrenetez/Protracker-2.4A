"""Publish only Root's actual fresh image observation; no target calls."""
from clock_runtime import require_harness_clock
require_harness_clock()
import argparse,hashlib,json,time
from pathlib import Path
from capture_support import read,pinned,save,need
from root_observation import validate_request,validate_observation

D=Path(__file__).resolve().parent

def main():
    a=argparse.ArgumentParser()
    a.add_argument('--freeze-ref',required=True);a.add_argument('--request-ref',required=True)
    a.add_argument('--actually-viewed-exact-png',action='store_true')
    a.add_argument('--fields',required=True)
    args=a.parse_args();need(args.actually_viewed_exact_png,'Root must first view the actual exact current PNG')
    fr=json.loads(args.freeze_ref);need(fr['path']==str(D/'SOURCE-FREEZE.json'),'Exact current package freeze required')
    freeze=json.loads(pinned(fr))
    need(freeze['state']=='ROOT_FROZEN_EDITOR_READONLY_PASSIVE_V2_SESSION_SOURCE_NOT_EXECUTED','Current exact source freeze differs')
    for source in freeze['sources'].values():pinned(source)
    request_ref=json.loads(args.request_ref);request=json.loads(pinned(request_ref));validate_request(request,request_ref)
    need(request['sourceFreezeRef']==fr,'Request belongs to another source freeze')
    image=request['capture']['image'];body=pinned(image);need(body.startswith(b'\x89PNG\r\n\x1a\n'),'Actual exact PNG required')
    value={'state':'ROOT_ACTUALLY_VIEWED_CURRENT_EDITOR_MOUSE_IMAGE','acceptedByRoot':True,
           'actuallyViewedExactPNG':True,'stage':request['stage'],'requestSHA256':request_ref['sha256'],
           'ownerRunID':request['ownerRunID'],'operationNonceSHA256':request['operationNonceSHA256'],
           'captureID':request['capture']['captureID'],'controllerID':request['capture']['controllerID'],
           'captureNonce':request['capture']['captureNonce'],'imageSHA256':image['sha256'],
           'metadata':request['capture']['metadata'],'geometry':request['capture']['geometry'],
           'observedMonotonic':time.monotonic()}
    fields=json.loads(args.fields);need(not (set(value)&set(fields)),'No substitution of bound observation fields')
    value.update(fields);validate_observation(value,request,request_ref,time.monotonic())
    pinned(request_ref);pinned(image)
    out=save(request['observationPath'],value)
    print(json.dumps({'state':'ROOT_ACTUAL_CURRENT_MOUSE_IMAGE_OBSERVATION_PUBLISHED_NO_INPUT','observation':out}))

if __name__=='__main__':main()
