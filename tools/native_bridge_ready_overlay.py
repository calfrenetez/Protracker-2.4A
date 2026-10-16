"""Pure, pinned private overlay; never modify or execute the installed wrapper.

For 030 bridge startup, move the existing owned-listener wait before the first
IPC ping. Keep the same waits, arguments, guards, process checks and requests.
Stock/no-bridge startup retains its original order. An inverse AST comparison
refuses any other change. This module performs no process, socket or file I/O.
"""
import ast
import copy
import hashlib

SOURCE_SHA256 = '9f79c268a1539598ed235217f1d0bc7ce707cab58b2018c8cd7f5d2d08b31a25'
CONDITION = "a.target=='amiberry-030' and not a.no_bridge"


def dump(node):
    return ast.dump(node, include_attributes=False)


def expression(text):
    return ast.parse(text, mode='eval').body


def is_bridge_wait(node):
    return dump(node) == dump(ast.parse('await wait_owned_listener(owned)').body[0])


def corrected_tree(source):
    if not isinstance(source, bytes) or hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError('Installed emulator wrapper differs from pinned source')
    original = ast.parse(source)
    changed = copy.deepcopy(original)
    functions = [n for n in changed.body if isinstance(n, ast.AsyncFunctionDef) and n.name == '_main']
    if len(functions) != 1:
        raise ValueError('Exactly one original lifecycle body required')
    body = functions[0].body
    bridge = [i for i, n in enumerate(body) if is_bridge_wait(n)]
    ping = [i for i, n in enumerate(body) if isinstance(n, ast.If)
            and dump(n.test) == dump(expression('not await c.ping()'))]
    if len(bridge) != 1 or len(ping) != 1 or bridge[0] != ping[0] + 2:
        raise ValueError('Original startup order differs')
    branch = body[ping[0] + 1]
    if not isinstance(branch, ast.If) or dump(branch.test) != dump(expression("a.target=='amiberry-stock' or a.no_bridge")):
        raise ValueError('Original stock/no-bridge branch differs')
    wait = body.pop(bridge[0])
    gate = ast.copy_location(ast.If(test=expression(CONDITION), body=[wait], orelse=[]), wait)
    body.insert(ping[0], gate)

    # Independently reconstruct every original statement, including guards,
    # timeout arguments, waits, request counts and stock/no-bridge behavior.
    inverse = copy.deepcopy(changed)
    restored = next(n for n in inverse.body if isinstance(n, ast.AsyncFunctionDef) and n.name == '_main').body
    recovered = restored.pop(ping[0])
    if dump(recovered.test) != dump(expression(CONDITION)) or recovered.orelse or len(recovered.body) != 1:
        raise ValueError('Unexpected readiness gate')
    restored.insert(bridge[0], recovered.body[0])
    if dump(inverse) != dump(original):
        raise ValueError('Private overlay differs beyond one owned-listener wait move')
    return ast.fix_missing_locations(changed)
