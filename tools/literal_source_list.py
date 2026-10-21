"""Read a literal build-input list without importing or executing its wrapper."""
import ast


def literal_source_list(source, key):
    """Honor top-level literal assignments and +=, refusing dynamic mutations."""
    tree = ast.parse(source)
    value = None
    handled = set()

    def strings(node):
        try:
            result = ast.literal_eval(node)
        except (ValueError, TypeError, SyntaxError) as error:
            raise ValueError(f'{key}: expected a literal source list') from error
        if not isinstance(result, (list, tuple)) or not all(isinstance(x, str) for x in result):
            raise ValueError(f'{key}: expected a list or tuple of strings')
        return list(result)

    for node in tree.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == key for t in node.targets):
            if len(node.targets) != 1:
                raise ValueError(f'{key}: aliased assignment is unsupported')
            value = strings(node.value)
            handled.add(id(node.targets[0]))
        elif isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Name) and node.target.id == key:
            if value is None or not isinstance(node.op, ast.Add):
                raise ValueError(f'{key}: expected assignment before +=')
            value += strings(node.value)
            handled.add(id(node.target))

    for node in ast.walk(tree):
        if isinstance(node, ast.Name) and node.id == key and isinstance(node.ctx, (ast.Store, ast.Del)) and id(node) not in handled:
            raise ValueError(f'{key}: non-top-level assignment or mutation is unsupported')
        if isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name) and node.value.id == key:
            raise ValueError(f'{key}: attribute mutation is unsupported')
        if isinstance(node, ast.Subscript) and isinstance(node.value, ast.Name) and node.value.id == key and isinstance(node.ctx, (ast.Store, ast.Del)):
            raise ValueError(f'{key}: item mutation is unsupported')
    if value is None:
        raise ValueError(f'{key}: source list is missing')
    return value
