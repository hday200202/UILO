#!/usr/bin/env python3
"""Compiles every C++ block in a markdown file, so the docs cannot drift away
from the API without someone noticing.

    tools/check_readme.py [file.md ...]

Needs a built UILO: the include flags come out of the compile database at
build/*/compile_commands.json, so run ./build.sh first.

Blocks that are a whole program are compiled as they stand. A block that is a
function definition is compiled at namespace scope. Anything else is treated as
a fragment and wrapped in a function body, with each blank-line-separated group
terminated, since documentation snippets are usually bare expressions. Blocks
that are plainly signature listings rather than code are skipped.
"""

import glob
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def include_flags():
    dbs = glob.glob(os.path.join(ROOT, 'build', '**', 'compile_commands.json'),
                    recursive=True)
    if not dbs:
        print('no compile_commands.json under build/ -- run ./build.sh first')
        raise SystemExit(2)
    db = json.load(open(dbs[0]))
    ent = next((e for e in db if 'Element.cpp' in e['file']), db[0])
    cmd = ent.get('command') or ' '.join(ent['arguments'])
    found = re.findall(r'-I\s*(\S+)|-isystem\s*(\S+)', cmd)
    return [f'-I{a or b}' for a, b in found]


def blocks(md):
    for m in re.finditer(r'```cpp\n(.*?)```', md, re.S):
        yield md[:m.start()].count('\n') + 1, m.group(1)


def is_listing(code):
    """A signature listing: declarations ending in ';' with no body, or a run of
    bare `.setX(Type)` lines. These document shapes, not compilable code."""
    s = code.strip()
    return (re.match(r'^[\w:<>,\s*&]+\s+\w+\s*\([\w:<>,\s*&{}\.]*\)\s*;', s)
            or s.startswith('.set')
            or s.startswith('UILO(')
            or re.match(r'^\w+\*?\s+\w+\(Modifier', s))


def wrap(code):
    if 'int main' in code:
        return code
    if re.match(r'^\s*\w+\s*\*?\s*\w+\([^)]*\)\s*\{', code):
        return '#include <UILO.hpp>\nusing namespace uilo;\n' + code

    groups = [g.rstrip() for g in re.split(r'\n\s*\n', code.strip()) if g.strip()]
    # The terminator goes on its own line: a snippet's last line often ends in a
    # trailing // comment, which would swallow a semicolon appended to it.
    body = '\n'.join(g if g.endswith((';', '}')) else g + '\n;' for g in groups)
    return ('#include <UILO.hpp>\nusing namespace uilo;\n'
            'void uilo_readme_check() {\n' + body + '\n}\n')


def main() -> int:
    paths = sys.argv[1:] or [os.path.join(ROOT, 'README.md')]
    incs = include_flags()
    scratch = os.path.join(ROOT, 'build', 'readme-check')
    os.makedirs(scratch, exist_ok=True)

    fails = 0
    for path in paths:
        print(f'{os.path.relpath(path, ROOT)}:')
        for i, (line, code) in enumerate(blocks(open(path).read())):
            if is_listing(code):
                print(f'  line {line:4d}: skipped (signature listing)')
                continue
            src = os.path.join(scratch, f'block_{i}.cpp')
            open(src, 'w').write(wrap(code))
            r = subprocess.run(['clang++', '-std=c++20', '-fsyntax-only', *incs, src],
                               capture_output=True, text=True)
            if r.returncode == 0:
                print(f'  line {line:4d}: OK')
            else:
                fails += 1
                errs = [l for l in r.stderr.splitlines() if ' error: ' in l]
                print(f'  line {line:4d}: FAIL  {errs[0] if errs else r.stderr[:160]}')

    print()
    print('all examples compile' if not fails else f'{fails} block(s) failed')
    return 1 if fails else 0


if __name__ == '__main__':
    raise SystemExit(main())
