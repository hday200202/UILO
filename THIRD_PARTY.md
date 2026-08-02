# Third-party notices

UILO itself is under the licence in `License.txt`. These are the components
redistributed with it, including the two typefaces compiled into every binary.

## Fonts compiled into the library

Both faces are embedded as byte arrays so a UILO binary renders text with no
files beside it. Regenerate either with `tools/gen_embedded_font.py`.

### DejaVu Sans — `include/assets/EmbeddedFont.hpp`

Named `Resources::fonts::default_` / `Resources::fonts::regular`, and used for
any text that never had a font set.

Released under the DejaVu Fonts Licence, which derives from the Bitstream Vera
Fonts Licence. Permission is granted, free of charge, to use, copy, modify and
distribute the fonts, provided the copyright notices are kept and the fonts are
not sold on their own. The full text ships with the upstream DejaVu release:

    https://dejavu-fonts.github.io/License.html

Bitstream Vera is a trademark of Bitstream, Inc.

### Droid Sans Mono — `include/assets/EmbeddedMonoFont.hpp`

Named `Resources::fonts::mono`, and what anything laid out as a grid should use
— `Terminal`, or a `Textbox` acting as a code editor.

Copyright (C) Google, Inc. Licensed under the Apache Licence, Version 2.0:

    http://www.apache.org/licenses/LICENSE-2.0

The font's own name table carries the same statement. The copy embedded here
came from `ext/bgfx/examples/runtime/font/droidsansmono.ttf`.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied.

## Icons compiled into the library

`include/assets/EmbeddedIcons.hpp` holds the Feather icon set (MIT), reachable
by name through `Resources::icons`.

    https://github.com/feathericons/feather

## Vendored dependencies

These live under `ext/` and keep their own licences, which are included in each
subdirectory:

| Component     | Licence      | Used for                          |
|---------------|--------------|-----------------------------------|
| bgfx / bimg / bx | BSD 2-Clause | rendering backend              |
| SDL3          | Zlib         | windowing and input               |
| stb_truetype  | MIT / public domain | glyph rasterisation        |
| NanoSVG       | Zlib         | SVG icon parsing                  |
| Wt            | GPL / commercial | the optional web backend only |

The Wt backend is opt-in (`-DUILO_WT=ON`) and is not part of a default build. If
you enable it, its licence applies to what you ship.
