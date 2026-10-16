"""Root validation of an observed emulator upload before external adoption.

This pure helper opens no connection and grants no ownership. The admitted Root
caller supplies the frozen, ordered upload/parent observations and a separately
observed task list from its current window. The result is a local receipt for the
emulator route only; physical MiniFTP requires its own actual GUI observations.
"""
import hashlib
import json
import re
import zlib

import native_file_protocol as wire
from owned_native_input import InputPlan
from owned_native_run import require
from owned_transfer_receipt import verify_receipt


def _encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def _same(a, b):
    return _encoded(a) == _encoded(b)


def make_receipt(plan, transfer, records, parent_records, tasks, *,
                 native_plan_sha256, owner_run_id, download_name, now):
    """Require every write ACK, fresh empty namespace and complete RAM parent.

    Neither receipt freshness nor CRC establishes exclusive ownership. The caller
    must check its live guards around all observations and the admitted request.
    No failed or partial upload is resumable through this function.
    """
    require(type(plan) is InputPlan and plan.external_transfer, 'External input plan required')
    upload = InputPlan(plan.execution, None, None)
    binary = plan.execution.binary
    manifest = {'NativeTest': {'bytes': len(binary), 'sha256': hashlib.sha256(binary).hexdigest()}}
    ready = {'status': 'INPUT_READY_CUSTODY_RETAINED', 'plan': upload.description(), 'files': manifest}
    require(_same(transfer, ready), 'Exact completed upload result required')
    require(type(records) is list and len(records) <= 514, 'Bounded ordered upload ledger required')
    position = 0

    def item(expected):
        nonlocal position
        require(position < len(records) and _same(records[position], expected),
                'Upload ledger missing, changed or reordered')
        position += 1

    number = 0
    def exchange(command, reply=None, *, checksum=False, directory=False):
        nonlocal number, position
        number += 1
        item({'event': 'input_dispatch', 'number': number, 'command': command})
        require(position < len(records), 'Upload acknowledgement missing')
        observed = records[position]
        require(type(observed) is dict and set(observed) == {'event', 'number', 'raw'}
                and observed['event'] == 'input_reply' and type(observed['number']) is int
                and observed['number'] == number, 'Upload reply identity differs')
        raw = wire.line(observed['raw'])
        if checksum:
            match = re.fullmatch(re.escape('CHECKSUM|' + plan.path('NativeTest') + '|')
                                 + r'([0-9a-fA-F]{8})\|(0|[1-9][0-9]*)', raw)
            require(match and int(match[1], 16) == zlib.crc32(binary)
                    and int(match[2]) == len(binary), 'Observed upload checksum differs')
        elif directory:
            count, entries = wire.directory_page(raw, plan.directory, 0)
            require(count == 1 and entries == (wire.Entry('NativeTest', len(binary), 'F'),),
                    'Observed upload inventory differs')
        else:
            require(raw == reply, 'Upload acknowledgement differs')
        position += 1

    item({'event': 'input_plan', 'plan': upload.description()})
    exchange({'type': 'MAKEDIR', 'path': plan.directory}, 'OK|MAKEDIR|' + plan.directory)
    exchange({'type': 'LISTDIR', 'path': plan.directory, 'offset': 0}, 'DIR|' + plan.directory + '|0|')
    for command in wire.write_commands(plan.path('NativeTest'), binary):
        chunk = bytes.fromhex(command.pop('hexData'))
        command.update(bytes=len(chunk), sha256=hashlib.sha256(chunk).hexdigest())
        exchange(command, 'OK|WRITEFILE|' + str(len(chunk)))
    exchange({'type': 'CHECKSUM', 'path': plan.path('NativeTest')}, checksum=True)
    exchange({'type': 'LISTDIR', 'path': plan.directory, 'offset': 0}, directory=True)
    exchange({'type': 'CHECKSUM', 'path': plan.path('NativeTest')}, checksum=True)
    item({'event': 'input_ready', **ready})
    require(position == len(records), 'Unexpected upload followup in frozen ledger')

    require(type(parent_records) is list and 0 < len(parent_records) <= wire.MAX_PAGES,
            'Complete bounded RAM parent observation required')
    entries, names, total = [], set(), None
    for observation in parent_records:
        require(type(observation) is dict and set(observation) == {'command', 'raw'}
                and _same(observation['command'], {'type': 'LISTDIR', 'path': 'RAM:', 'offset': len(entries)}),
                'RAM parent observation order differs')
        require(total is None or len(entries) < total, 'Unexpected parent observation followup')
        count, page = wire.directory_page(observation['raw'], 'RAM:', len(entries))
        require(total is None or count == total, 'RAM parent changed during pagination')
        total = count
        for entry in page:
            require(entry.name.lower() not in names, 'Repeated RAM parent entry')
            names.add(entry.name.lower())
            entries.append(entry)
    require(len(entries) == total, 'Incomplete RAM parent observation')
    wire.absent(tuple(entries), (plan.execution.directory.split(':')[1], download_name))
    matches = [e for e in entries if e.name.lower() == plan.directory.split(':')[1].lower()]
    require(len(matches) == 1 and matches[0].kind == 'D', 'Transferred input parent absent or not a directory')
    require(type(tasks) is str and len(tasks) <= 65536 and tasks.startswith('Tasks (')
            and all(name.lower() not in tasks.lower() for name in
                    ('NativeTest', plan.execution.directory.split(':')[1], download_name)),
            'Independent transfer reader absence unconfirmed')
    common = {'runID': plan.execution.run_id, 'ownerRunID': owner_run_id, 'target': 'amiberry-030'}
    witnesses = {
        'namespace': {**common, 'state': 'FRESH_INPUT_NAMESPACE_CREATED_EMPTY', 'directory': plan.directory},
        'complete': {**common, 'state': 'EXACT_TRANSFER_COMPLETION_OBSERVED',
                     'method': 'OWNED_EMULATOR_TRANSFER', 'downloadName': download_name, 'bytes': len(binary)},
        'placement': {**common, 'state': 'SINGLE_NATIVE_FILE_PLACED_CONTROL_RELEASED',
                      'path': plan.path('NativeTest'), 'bytes': len(binary), 'members': ['NativeTest'],
                      'downloadPathAbsent': True, 'nativeTaskAbsent': True, 'uiControlReleased': True}}
    encoded = {name: _encoded(value) for name, value in witnesses.items()}
    receipt = _encoded({'state': 'ROOT_OBSERVED_FRESH_TRANSFER_COMPLETE', 'description': plan.description(),
                       'nativePlanSHA256': native_plan_sha256, 'ownerRunID': owner_run_id,
                       'target': 'amiberry-030', 'downloadName': download_name, 'observedUnixSeconds': now,
                       'witnessSHA256': {name: hashlib.sha256(body).hexdigest() for name, body in encoded.items()}})
    digest = hashlib.sha256(receipt).hexdigest()
    verify_receipt(receipt, encoded, expected_sha256=digest, plan=plan,
                   native_plan_sha256=native_plan_sha256, owner_run_id=owner_run_id,
                   target='amiberry-030', download_name=download_name, now=now)
    return {'root-transfer.json': receipt,
            **{'root-transfer-' + name + '.json': body for name, body in encoded.items()}}
