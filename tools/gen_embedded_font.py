#!/usr/bin/env python3
"""Turns a TTF into a header holding its bytes, for fonts that ship inside the
binary rather than beside it.

    tools/gen_embedded_font.py <font.ttf> <SYMBOL> <out.hpp>

SYMBOL names the resulting array, e.g. EMBEDDED_DROIDSANSMONO_FONT. The output
matches include/assets/EmbeddedFont.hpp, which was produced the same way before
this script existed.
"""

import os
import sys

PER_LINE = 12


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__.strip())
        return 2

    src, symbol, dst = sys.argv[1], sys.argv[2], sys.argv[3]
    data = open(src, 'rb').read()
    if data[:4] not in (b'\x00\x01\x00\x00', b'true', b'ttcf', b'OTTO'):
        print(f'{src}: does not look like a TrueType/OpenType file')
        return 1

    out = [
        '#pragma once',
        '',
        '#include <cstdint>',
        '#include <vector>',
        '',
        'namespace uilo {',
        '',
        f'// Embedded font: {os.path.basename(src)}',
        f'// Size: {len(data)} bytes',
        f'inline const std::vector<uint8_t> {symbol} = {{',
    ]
    for i in range(0, len(data), PER_LINE):
        chunk = data[i:i + PER_LINE]
        out.append('    ' + ', '.join(f'0x{b:02X}' for b in chunk) + ',')
    out += ['};', '', '} // namespace uilo', '']

    with open(dst, 'w', encoding='utf-8') as f:
        f.write('\n'.join(out))

    print(f'wrote {dst}  ({len(data)} bytes -> {symbol})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
