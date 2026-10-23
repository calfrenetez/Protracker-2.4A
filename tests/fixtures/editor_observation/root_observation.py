"""Pure exact-image observation protocol; no target API or automatic approval."""
import math
from pathlib import Path
from capture_support import need

STAGES = ('before-move','after-move','after-click')
EXTRA = {
    'before-move': {'editorBlankAndUnmodal','screenRectangle'},
    'after-move': {'pointerTipIdentifiable','observedPointerTip'},
    'after-click': {'row4Channel1CursorVisible','noUnexpectedModalOrContentChange'},
}


def validate_observation(value, request, request_ref, now):
    stage = request['stage']
    need(stage in STAGES, 'Unknown finite mouse observation stage')
    expected = {
        'state':'ROOT_ACTUALLY_VIEWED_CURRENT_EDITOR_MOUSE_IMAGE',
        'acceptedByRoot':True,'actuallyViewedExactPNG':True,
        'stage':stage,'requestSHA256':request_ref['sha256'],
        'ownerRunID':request['ownerRunID'],
        'operationNonceSHA256':request['operationNonceSHA256'],
        'captureID':request['capture']['captureID'],
        'controllerID':request['capture']['controllerID'],
        'captureNonce':request['capture']['captureNonce'],
        'imageSHA256':request['capture']['image']['sha256'],
        'metadata':request['capture']['metadata'],
        'geometry':request['capture']['geometry'],
    }
    need(type(value) is dict and set(value) == set(expected)|EXTRA[stage]|{'observedMonotonic'}
         and all(value[k] == v for k,v in expected.items())
         and value['acceptedByRoot'] is True and value['actuallyViewedExactPNG'] is True,
         'Observation must bind the exact actual request/image/owner/frame/nonce')
    observed=value['observedMonotonic']
    need(type(observed) in (int,float) and math.isfinite(observed)
         and request['requestedMonotonic'] <= observed <= now < request['deadlineMonotonic'],
         'Root must actually view and publish inside this finite observation')
    if stage == 'before-move':
        rect=value['screenRectangle']
        need(value['editorBlankAndUnmodal'] is True
             and type(rect) is list and len(rect)==4 and all(type(v) is int for v in rect),
             'Root must view the actual blank editor and its full unscaled screen rectangle')
    elif stage == 'after-move':
        known=value['pointerTipIdentifiable'];tip=value['observedPointerTip']
        m=request['capture']['metadata']
        need(type(known) is bool and
             ((known is False and tip is None) or
              (known is True and type(tip) is list and len(tip)==2
               and all(type(v) is int for v in tip)
               and 0<=tip[0]<m['image_width'] and 0<=tip[1]<m['image_height'])),
             'Disclose actual integer pointer tip or explicitly report unidentifiable')
    else:
        need(value['row4Channel1CursorVisible'] is True
             and value['noUnexpectedModalOrContentChange'] is True,
             'Final cursor and no unexpected change must actually be observed')
    return value


def validate_request(request, request_ref):
    stage=request['stage'];need(stage in STAGES,'Unknown finite stage')
    case=Path(request['caseDirectory']);image=Path(request['capture']['image']['path'])
    observation=Path(request['observationPath']);path=Path(request_ref['path'])
    need(case.is_absolute() and '..' not in case.parts
         and observation==case/'mouse-phase'/(stage+'-observation.json')
         and path==case/'mouse-phase'/(stage+'-request.json')
         and image.parent in (case,case/'mouse-phase')
         and request['state']=='ROOT_EDITOR_MOUSE_IMAGE_OBSERVATION_REQUIRED'
         and request['deadlineMonotonic']>request['requestedMonotonic']
         and request['deadlineMonotonic']-request['requestedMonotonic']<=60,
         'Observation request case/image/deadline association differs')
    return request
