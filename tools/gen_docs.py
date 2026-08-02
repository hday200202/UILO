#!/usr/bin/env python3
"""
Generates docs/ from the block comments in include/.

UILO's sources carry one block comment above every type and every function, in a
fixed shape:

    /*
        Name:
        - Desc: what it is
        - any number of extra notes
    */

    /*
        name(params):
        - Params:   a, b
        - Returns:  T
        - Desc:     what it does
    */

This walks the tree, pulls those out, and writes one Markdown page per source
file under docs/, mirroring the directory layout. Every type name that has a
page of its own becomes a link wherever it is mentioned, so the docs are
navigable rather than a flat dump.

Usage:
    python3 tools/gen_docs.py            # write docs/
    python3 tools/gen_docs.py --check    # report problems, write nothing
"""

import os, re, sys, html
from collections import OrderedDict

ROOT     = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC      = os.path.join(ROOT, 'include')
OUT      = os.path.join(ROOT, 'docs')
SKIP     = ('assets/EmbeddedFont', 'assets/EmbeddedIcons', 'assets/EmbeddedAssets',
            'wt/shim/')
KEYS     = ('Params', 'Returns', 'Desc')

BLOCK    = re.compile(r'/\*\n(.*?)\n\s*\*/', re.S)
HEADER   = re.compile(r'^\s{4}([A-Za-z_~][\w:]*)\s*(\(.*?\))?\s*:\s*$')
FIELD    = re.compile(r'^\s{4}- (\w+):\s*(.*)$')
NOTE     = re.compile(r'^\s{4}- (.*)$')
CONT     = re.compile(r'^(\s{6,})(\S.*)$')


class Entry:
    def __init__(self, name, sig, line):
        self.name, self.sig, self.line = name, sig, line
        self.fields = OrderedDict()
        self.notes  = []

    @property
    def is_function(self):
        return self.sig is not None

    @property
    def anchor(self):
        return re.sub(r'[^a-z0-9]+', '-', self.name.lower()).strip('-')


def parse_block(text, start_line):
    """Turn one block comment's interior into an Entry, or None if it is not one."""
    lines = text.split('\n')
    m = HEADER.match(lines[0])
    if not m:
        return None
    e = Entry(m.group(1), m.group(2), start_line)

    cur_key = None
    for ln in lines[1:]:
        fm = FIELD.match(ln)
        if fm and fm.group(1) in KEYS:
            cur_key = fm.group(1)
            e.fields[cur_key] = [(0, fm.group(2).strip())]
            continue
        nm = NOTE.match(ln)
        if nm:
            cur_key = '__note__'
            e.notes.append([(0, nm.group(1).strip())])
            continue
        cm = CONT.match(ln)
        if cm and cur_key:
            indent, txt = len(cm.group(1)), cm.group(2).rstrip()
            # Record the indent alongside the text. A sample indented deeper
            # than the field's own continuation is code, and losing that here
            # is what would let a code block be reflowed into prose -- and then
            # have its type names turned into links.
            if cur_key == '__note__':
                e.notes[-1].append((indent, txt))
            else:
                e.fields[cur_key].append((indent, txt))
    return e


def scan(path):
    src = open(path, encoding='utf-8').read()
    out = []
    for m in BLOCK.finditer(src):
        e = parse_block(m.group(1), src[:m.start()].count('\n') + 1)
        if e:
            out.append(e)
    return out


def collect():
    files = OrderedDict()
    for dirpath, _, names in os.walk(SRC):
        for n in sorted(names):
            if not n.endswith(('.hpp', '.cpp')):
                continue
            full = os.path.join(dirpath, n)
            rel  = os.path.relpath(full, ROOT).replace(os.sep, '/')
            if any(s in rel for s in SKIP):
                continue
            entries = scan(full)
            if entries:
                files[rel] = entries
    return files


def flatten(parts):
    """Join a parsed field back into one line, for Params and Returns."""
    return ' '.join(t for _, t in parts) if parts else ''


def md_escape(s):
    return s.replace('<', '&lt;').replace('>', '&gt;').replace('|', '\\|')


def build_link_table(files):
    """type name -> (page, anchor). Types only; function names are too ambiguous."""
    table = {}
    for rel, entries in files.items():
        page = doc_path(rel)
        for e in entries:
            if not e.is_function and e.name not in table:
                table[e.name] = (page, e.anchor)
    return table


def doc_path(rel):
    # include/elements/containers/Row.hpp -> elements/containers/Row.hpp.md
    return re.sub(r'^include/', '', rel) + '.md'


def linkify(text, table, page, skip=None):
    """Turn known type names into links, leaving code fences alone."""
    def repl(m):
        name = m.group(0)
        if name == skip or name not in table:
            return name
        tgt, anch = table[name]
        if tgt == page:
            return f'[{name}](#{anch})'
        rel = os.path.relpath(tgt, os.path.dirname(page)).replace(os.sep, '/')
        return f'[{name}]({rel}#{anch})'

    parts = re.split(r'(```.*?```|`[^`]*`)', text, flags=re.S)
    for i, part in enumerate(parts):
        if part.startswith('`'):
            continue
        parts[i] = re.sub(r'\b[A-Z][A-Za-z0-9_]{2,}\b', repl, part)
    return ''.join(parts)


def render_body(parts):
    """
    Reflow a field into prose, keeping any deeper-indented run as a code block.

    `parts` is the (indent, text) list the parser produced. The field's own
    continuation indent is the minimum of the non-zero ones; anything further in
    than that is a sample and is emitted verbatim in a fence.
    """
    if not parts:
        return ''
    body = [p for p in parts if p[0] > 0]
    base = min((p[0] for p in body), default=0)

    out, prose, code = [], [], []

    def flush_prose():
        if prose:
            out.append(' '.join(prose)); prose.clear()

    def flush_code():
        if code:
            pad = min(c[0] for c in code)
            out.append('\n```cpp\n' +
                       '\n'.join(' ' * (c[0] - pad) + c[1] for c in code).rstrip() +
                       '\n```\n')
            code.clear()

    for indent, txt in parts:
        if indent > base + 2:
            flush_prose(); code.append((indent, txt))
        else:
            flush_code(); prose.append(txt)
    flush_code(); flush_prose()
    return '\n'.join(out)


def page_for(rel, entries, table):
    page  = doc_path(rel)
    depth = page.count('/')
    up    = '../' * depth
    title = os.path.basename(rel)

    types = [e for e in entries if not e.is_function]
    funcs = [e for e in entries if e.is_function]

    L = [f'# {title}', '',
         f'`{rel}`', '',
         f'[← index]({up}README.md)', '']

    if types:
        L += ['## Types', '']
        for e in types:
            L += [f'- [{e.name}](#{e.anchor})']
        L += ['']
    if funcs:
        L += ['## Functions', '']
        for e in funcs:
            L += [f'- [`{md_escape(e.name + (e.sig or ""))}`](#{e.anchor})']
        L += ['']

    for e in types:
        L += ['---', '', f'### {e.name}', '']
        if 'Desc' in e.fields:
            L += [linkify(render_body(e.fields['Desc']), table, page, skip=e.name), '']
        for n in e.notes:
            L += [f'> {linkify(render_body(n), table, page, skip=e.name)}', '']

    for e in funcs:
        L += ['---', '', f'### {e.name}', '',
              '```cpp', f'{e.name}{e.sig}', '```', '']
        params = flatten(e.fields.get('Params')).strip()
        if params and params != 'none':
            L += ['**Parameters**', '']
            for p in [x.strip() for x in params.replace('\n', ' ').split(',')]:
                if p:
                    L += [f'- `{p}`']
            L += ['']
        ret = flatten(e.fields.get('Returns')).strip()
        if ret and ret != 'none':
            L += [f'**Returns** — {linkify(md_escape(ret), table, page)}', '']
        if 'Desc' in e.fields:
            L += [linkify(render_body(e.fields['Desc']), table, page), '']
        for n in e.notes:
            L += [f'> {linkify(render_body(n), table, page)}', '']

    return page, '\n'.join(L).rstrip() + '\n'


def index_page(files, table):
    L = ['# UILO API reference', '',
         'Generated from the block comments in `include/` by `tools/gen_docs.py`.',
         'Edit the comments, not these files.', '']

    total_t = sum(1 for es in files.values() for e in es if not e.is_function)
    total_f = sum(1 for es in files.values() for e in es if e.is_function)
    L += [f'{len(files)} files, {total_t} types, {total_f} functions.', '']

    groups = OrderedDict()
    for rel in files:
        d = os.path.dirname(re.sub(r'^include/?', '', rel)) or '.'
        groups.setdefault(d, []).append(rel)

    for d in sorted(groups, key=lambda x: (x == '.', x)):
        L += [f'## {"(root)" if d == "." else d}', '']
        for rel in sorted(groups[d]):
            page = doc_path(rel)
            ts = [e.name for e in files[rel] if not e.is_function]
            note = ' — ' + ', '.join(f'`{t}`' for t in ts[:4]) if ts else ''
            if len(ts) > 4:
                note += ', …'
            L += [f'- [{os.path.basename(rel)}]({page}){note}']
        L += ['']

    L += ['## All types', '']
    for name in sorted(table):
        tgt, anch = table[name]
        L += [f'- [{name}]({tgt}#{anch})']
    return '\n'.join(L).rstrip() + '\n'


def main():
    check = '--check' in sys.argv
    files = collect()
    table = build_link_table(files)

    problems = []
    for rel, entries in files.items():
        for e in entries:
            if e.is_function:
                for k in KEYS:
                    if k not in e.fields:
                        problems.append(f'{rel}:{e.line}  {e.name} is missing "- {k}:"')
                if flatten(e.fields.get('Desc')).strip() in ('', 'TODO'):
                    problems.append(f'{rel}:{e.line}  {e.name} has an empty Desc')
            else:
                if 'Desc' not in e.fields:
                    problems.append(f'{rel}:{e.line}  type {e.name} is missing "- Desc:"')

    if problems:
        print(f'{len(problems)} problem(s):')
        for p in problems[:40]:
            print('  ' + p)
    else:
        print('no problems found')

    if check:
        return 1 if problems else 0

    written = 0
    for rel, entries in files.items():
        page, text = page_for(rel, entries, table)
        dst = os.path.join(OUT, page)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'w', encoding='utf-8').write(text)
        written += 1
    open(os.path.join(OUT, 'README.md'), 'w', encoding='utf-8').write(index_page(files, table))

    t = sum(1 for es in files.values() for e in es if not e.is_function)
    f = sum(1 for es in files.values() for e in es if e.is_function)
    print(f'wrote {written} pages + index to docs/  ({t} types, {f} functions)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
