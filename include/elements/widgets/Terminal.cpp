#include "Terminal.hpp"

#include "../../UILO.hpp"
#include "../../utils/Resources.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace uilo {

namespace {

/*
    utf8Encode(char32_t cp):
    - Params:   char32_t cp
    - Returns:  std::string
    - Desc:     Encodes one codepoint, for handing a cell's character back to the
                renderer, which takes UTF-8.
*/
std::string utf8Encode(char32_t cp) {
    std::string r;
    if (cp < 0x80u) {
        r += static_cast<char>(cp);
    } else if (cp < 0x800u) {
        r += static_cast<char>(0xC0 | (cp >> 6));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000u) {
        r += static_cast<char>(0xE0 | (cp >> 12));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        r += static_cast<char>(0xF0 | (cp >> 18));
        r += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return r;
}


/*
    csiParams(const std::string& s, std::vector<int>& out):
    - Params:   const std::string& s, std::vector<int>& out
    - Returns:  void
    - Desc:     Splits a CSI parameter string on semicolons into numbers. An
                omitted parameter becomes -1 rather than 0, because the two mean
                different things: most sequences read a missing parameter as 1,
                and conflating it with an explicit 0 breaks them.
*/
void csiParams(const std::string& s, std::vector<int>& out) {
    out.clear();
    int  cur  = -1;
    bool any  = false;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            if (!any) { cur = 0; any = true; }
            cur = cur * 10 + (c - '0');
        } else if (c == ';') {
            out.push_back(any ? cur : -1);
            cur = -1; any = false;
        }
    }
    out.push_back(any ? cur : -1);
}


/*
    param(const std::vector<int>& p, size_t i, int dflt):
    - Params:   const std::vector<int>& p, size_t i, int dflt
    - Returns:  int
    - Desc:     One CSI parameter, substituting the default for a missing or
                omitted one.
*/
int param(const std::vector<int>& p, size_t i, int dflt) {
    if (i >= p.size() || p[i] < 0) return dflt;
    return p[i];
}

} // namespace


/*
    Terminal(Modifier modifier, TerminalOptions options, const std::string& name):
    - Params:   Modifier modifier, TerminalOptions options,
                const std::string& name
    - Returns:  Terminal
    - Desc:     Constructs the widget and its initial grid. The shell is not
                started here: the grid has to be measured against real bounds
                first, so it starts on the first update instead.
*/
Terminal::Terminal(
    Modifier modifier,
    TerminalOptions options,
    const std::string& name
) : m_options(std::move(options)) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Terminal;

    m_pen.fg = m_options.getForegroundColor();
    m_pen.bg = Color{0, 0, 0, 0};
    resizeGrid(m_cols, m_rows);
}


/*
    ~Terminal():
    - Params:   none
    - Returns:  none
    - Desc:     Stops the shell, so closing a page never leaves one running.
*/
Terminal::~Terminal() { m_pty.close(); }


/*
    setOptions(const TerminalOptions& opts):
    - Params:   const TerminalOptions& opts
    - Returns:  void
    - Desc:     Replaces the options and forces the cell metrics to be measured
                again, since the font or character size may have moved.
*/
void Terminal::setOptions(const TerminalOptions& opts) {
    m_options     = opts;
    m_fontLoaded  = false;
    m_lastCharSize = 0;
    m_dirty       = true;
}


/*
    start():
    - Params:   none
    - Returns:  bool -- false when no shell could be started
    - Desc:     Launches the shell on a pseudo-terminal sized to the current
                grid. Starting twice is harmless.
*/
bool Terminal::start() {
    if (m_pty.isOpen()) return true;
    m_exitFired = false;
    const bool ok = m_pty.open(m_options.getShell(), m_cols, m_rows);
    m_started = ok;
    m_dirty   = true;
    return ok;
}


/*
    stop():
    - Params:   none
    - Returns:  void
    - Desc:     Ends the shell. The widget keeps whatever is on screen and can be
                started again.
*/
void Terminal::stop() {
    m_pty.close();
    m_started = false;
    m_dirty   = true;
}


/*
    isRunning():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a shell is attached and has not exited.
*/
bool Terminal::isRunning() const {
    return m_pty.isOpen();
}


/*
    send(const std::string& text):
    - Params:   const std::string& text
    - Returns:  void
    - Desc:     Feeds text to the shell as though it had been typed, which is
                also how a caller runs a command programmatically -- append a
                newline to execute it.
*/
void Terminal::send(const std::string& text) {
    m_pty.write(text);
}


/*
    cell(int col, int row):
    - Params:   int col, int row
    - Returns:  TerminalCell&
    - Desc:     One cell of the active grid, clamped into range so a malformed
                escape sequence cannot address outside it.
*/
TerminalCell& Terminal::cell(int col, int row) {
    col = std::clamp(col, 0, m_cols - 1);
    row = std::clamp(row, 0, m_rows - 1);
    return m_grid[static_cast<size_t>(row) * m_cols + col];
}

const TerminalCell& Terminal::cell(int col, int row) const {
    col = std::clamp(col, 0, m_cols - 1);
    row = std::clamp(row, 0, m_rows - 1);
    return m_grid[static_cast<size_t>(row) * m_cols + col];
}


/*
    resizeGrid(int cols, int rows):
    - Params:   int cols, int rows
    - Returns:  void
    - Desc:     Reshapes the grid, keeping the written content at the top where
                the shell put it. A resize spends the blank rows below the
                content first, so growing and shrinking normally move nothing at
                all; only when the written rows themselves no longer fit does
                the top scroll away into history.
    - Absolute line numbering is kept in step with any such shift, so a
      selection stays on the text it was made on.
    - History is never pulled back onto the screen. Doing that on a grow moves
      text the shell believes it has already drawn, and the shell will overwrite
      the wrong rows the next time it paints.
    - Lines are not re-wrapped. Reflowing needs the original unwrapped text, and
      a cell grid has already thrown that away.
*/
void Terminal::resizeGrid(int cols, int rows) {
    cols = std::max(1, cols);
    rows = std::max(1, rows);
    if (cols == m_cols && rows == m_rows && !m_grid.empty()) return;

    const bool keepHistory = !m_altScreen && m_options.getScrollback() > 0;
    const bool hadGrid     = !m_grid.empty();

    /* How far down the screen is actually written: the cursor's row, plus any
       written row below it. Everything past that is blank padding the shell has
       never touched, and padding is what a resize should be spending. */
    int used = 0;
    if (hadGrid) {
        used = m_curRow + 1;
        for (int r = m_rows - 1; r >= used; --r) {
            const TerminalCell* line = &m_grid[static_cast<size_t>(r) * m_cols];
            bool written = false;
            for (int c = 0; c < m_cols && !written; ++c)
                written = (line[c].ch != 0 && line[c].ch != U' ') || line[c].bg.a > 0;
            if (written) { used = r + 1; break; }
        }
    }

    /* Content sits at the top of the screen and grows downward, so that is
       where it stays. Growing just exposes more blank rows underneath, and
       shrinking eats the blank rows first -- only once the written rows
       themselves no longer fit does the top give way into scrollback. */
    int shifted = 0;                   /* rows moved off the top into history */
    if (hadGrid && used > rows) {
        shifted = used - rows;
        if (keepHistory) {
            for (int r = 0; r < shifted; ++r) {
                std::vector<TerminalCell> row(
                    m_grid.begin() + static_cast<size_t>(r) * m_cols,
                    m_grid.begin() + static_cast<size_t>(r + 1) * m_cols);
                m_scrollback.push_back(std::move(row));
            }
            while (static_cast<int>(m_scrollback.size()) > m_options.getScrollback())
                m_scrollback.pop_front();
        }
    }

    std::vector<TerminalCell> next(static_cast<size_t>(cols) * rows);
    for (auto& c : next) { c.fg = m_options.getForegroundColor(); c.bg = Color{0,0,0,0}; }

    const int copyCols = std::min(cols, m_cols);
    const int copyRows = std::min(rows, used - shifted);
    for (int r = 0; r < copyRows; ++r)
        for (int c = 0; c < copyCols; ++c)
            next[static_cast<size_t>(r) * cols + c] =
                m_grid[static_cast<size_t>(r + shifted) * m_cols + c];

    /* Row 0 of the grid is now `shifted` lines further along the history, which
       is exactly what an absolute line number counts -- so keeping this in step
       is what stops a selection sliding onto different text across a resize.
       Most resizes only move padding, leaving this at zero. */
    m_linesScrolled += shifted;
    m_curRow        -= shifted;

    m_grid = std::move(next);
    m_altGrid.assign(static_cast<size_t>(cols) * rows, TerminalCell{});

    m_cols = cols;
    m_rows = rows;
    m_scrollTop = 0;
    m_scrollBot = rows - 1;
    m_curCol = std::clamp(m_curCol, 0, cols - 1);
    m_curRow = std::clamp(m_curRow, 0, rows - 1);
    m_dirty  = true;
}


/*
    clearRegion(int fromCol, int fromRow, int toCol, int toRow):
    - Params:   int fromCol, int fromRow, int toCol, int toRow
    - Returns:  void
    - Desc:     Blanks an inclusive span, treated as a linear run of cells rather
                than a rectangle, because that is what erase-to-end-of-screen
                means. Cleared cells take the current background so an erase
                inside a coloured region stays coloured.
*/
void Terminal::clearRegion(int fromCol, int fromRow, int toCol, int toRow) {
    const int first = std::clamp(fromRow, 0, m_rows - 1) * m_cols + std::clamp(fromCol, 0, m_cols - 1);
    const int last  = std::clamp(toRow,   0, m_rows - 1) * m_cols + std::clamp(toCol,   0, m_cols - 1);
    for (int i = std::min(first, last); i <= std::max(first, last); ++i) {
        auto& c = m_grid[static_cast<size_t>(i)];
        c = TerminalCell{};
        c.fg      = m_pen.fg;
        c.bg      = m_pen.bg;
        c.inverse = m_pen.inverse;
    }
    m_dirty = true;
}


/*
    scrollUp(int lines):
    - Params:   int lines
    - Returns:  void
    - Desc:     Moves the scrolling region up, which is what a newline at the
                bottom does. Rows leaving the top are pushed to scrollback, but
                only when the region is the whole screen and the alternate screen
                is not in use -- a program scrolling a sub-region, or running
                full-screen, is not producing history.
*/
void Terminal::scrollUp(int lines) {
    if (lines <= 0) return;
    const int top = m_scrollTop, bot = m_scrollBot;
    const int span = bot - top + 1;
    lines = std::min(lines, span);

    const bool keepHistory = !m_altScreen && top == 0 && bot == m_rows - 1
                          && m_options.getScrollback() > 0;

    for (int i = 0; i < lines; ++i) {
        if (keepHistory) {
            std::vector<TerminalCell> row(
                m_grid.begin() + static_cast<size_t>(top) * m_cols,
                m_grid.begin() + static_cast<size_t>(top + 1) * m_cols);
            m_scrollback.push_back(std::move(row));
            while (static_cast<int>(m_scrollback.size()) > m_options.getScrollback())
                m_scrollback.pop_front();
        }
        /* Counted whether or not history is kept, so an absolute line number
           means the same thing on the alternate screen as on the main one. */
        ++m_linesScrolled;

        /* A reader who has scrolled back stays on the text they were reading:
           it is now one line further from the bottom, so the offset grows with
           it. Without this, output would drag the view along under them. */
        if (m_scrollPx > 0.f)
            m_scrollPx = std::min(m_scrollPx + m_cellH,
                                  m_scrollback.size() * m_cellH);

        for (int r = top; r < bot; ++r)
            std::copy(m_grid.begin() + static_cast<size_t>(r + 1) * m_cols,
                      m_grid.begin() + static_cast<size_t>(r + 2) * m_cols,
                      m_grid.begin() + static_cast<size_t>(r) * m_cols);

        for (int c = 0; c < m_cols; ++c) {
            auto& cc = m_grid[static_cast<size_t>(bot) * m_cols + c];
            cc = TerminalCell{};
            cc.fg = m_pen.fg;
            cc.bg = m_pen.bg;
        }
    }
    m_dirty = true;
}


/*
    scrollDown(int lines):
    - Params:   int lines
    - Returns:  void
    - Desc:     Moves the scrolling region down, opening blank rows at the top.
                Nothing is taken from scrollback: this is a program scrolling its
                own content, not the user looking back through history.
*/
void Terminal::scrollDown(int lines) {
    if (lines <= 0) return;
    const int top = m_scrollTop, bot = m_scrollBot;
    lines = std::min(lines, bot - top + 1);

    for (int i = 0; i < lines; ++i) {
        for (int r = bot; r > top; --r)
            std::copy(m_grid.begin() + static_cast<size_t>(r - 1) * m_cols,
                      m_grid.begin() + static_cast<size_t>(r) * m_cols,
                      m_grid.begin() + static_cast<size_t>(r) * m_cols);
        for (int c = 0; c < m_cols; ++c) {
            auto& cc = m_grid[static_cast<size_t>(top) * m_cols + c];
            cc = TerminalCell{};
            cc.fg = m_pen.fg;
            cc.bg = m_pen.bg;
        }
    }
    m_dirty = true;
}


/*
    newline():
    - Params:   none
    - Returns:  void
    - Desc:     Moves the cursor down a row, scrolling the region when it is
                already on the last one.
*/
void Terminal::newline() {
    if (m_curRow >= m_scrollBot) scrollUp(1);
    else                         ++m_curRow;
}


/*
    putChar(char32_t c):
    - Params:   char32_t c
    - Returns:  void
    - Desc:     Writes one character at the cursor and advances it. Wrapping is
                deferred: running past the last column arms a pending wrap rather
                than moving immediately, so a character landing exactly in the
                final column does not push the cursor onto the next line until
                something more is actually written. That is what stops a
                full-width line from leaving a stray blank row.
*/
void Terminal::putChar(char32_t c) {
    if (m_wrapPending) {
        m_curCol = 0;
        newline();
        m_wrapPending = false;
    }

    TerminalCell& cc = cell(m_curCol, m_curRow);
    cc = m_pen;
    cc.ch = c;

    if (m_curCol + 1 >= m_cols) m_wrapPending = true;
    else                        ++m_curCol;
    m_dirty = true;
}


/*
    useAlternateScreen(bool on, bool saveRestore):
    - Params:   bool on, bool saveRestore
    - Returns:  void
    - Desc:     Swaps between the normal screen and the alternate one full-screen
                programs draw on. The two are kept side by side and exchanged, so
                leaving vim puts back exactly what the shell had on screen
                before, and nothing the program drew reaches scrollback.
*/
void Terminal::useAlternateScreen(bool on, bool saveRestore) {
    if (on == m_altScreen) return;

    if (saveRestore) {
        if (on) {
            m_savedCol = m_curCol;
            m_savedRow = m_curRow;
            m_savedPen = m_pen;
        } else {
            /* Restoring the pen is the point: a program that signs off by
               setting its own idea of the default colours -- btop ends with an
               explicit black background and never returns it to default --
               would otherwise leave every later erase filling with that colour
               instead of the terminal's own background. */
            m_curCol = m_savedCol;
            m_curRow = m_savedRow;
            m_pen    = m_savedPen;
        }
    }

    std::swap(m_grid, m_altGrid);
    m_altScreen = on;

    if (on) {
        for (auto& c : m_grid) {
            c = TerminalCell{};
            c.fg = m_pen.fg; c.bg = m_pen.bg; c.inverse = m_pen.inverse;
        }
        m_curCol = m_curRow = 0;
    }
    m_scrollTop = 0;
    m_scrollBot = m_rows - 1;
    m_scrollPx = 0.f;
    m_dirty = true;
}


/*
    indexedColor(int n):
    - Params:   int n
    - Returns:  Color
    - Desc:     Resolves one of the 256 palette indices. 0-15 come from the
                configured sixteen; 16-231 are the 6x6x6 colour cube; 232-255 are
                the greyscale ramp. The cube and ramp are computed rather than
                stored, since they are defined by formula.
*/
Color Terminal::indexedColor(int n) const {
    if (n < 0)   return m_options.getForegroundColor();
    if (n < 16)  return m_options.getAnsiColor(n);

    if (n < 232) {
        const int i = n - 16;
        const int r = (i / 36) % 6, g = (i / 6) % 6, b = i % 6;
        auto lvl = [](int v) -> uint8_t {
            return static_cast<uint8_t>(v == 0 ? 0 : 55 + v * 40);
        };
        return Color{lvl(r), lvl(g), lvl(b), 255};
    }
    const uint8_t v = static_cast<uint8_t>(8 + (n - 232) * 10);
    return Color{v, v, v, 255};
}


/*
    applySgr():
    - Params:   none
    - Returns:  void
    - Desc:     Applies a select-graphic-rendition sequence to the pen: reset,
                the style flags and their individual offs, the eight normal and
                eight bright colours for both foreground and background, and the
                extended forms -- 5 for a 256-colour index and 2 for true colour.
                An unrecognised parameter is skipped rather than aborting the
                sequence, so one unknown attribute does not discard the rest.
*/
void Terminal::applySgr() {
    std::vector<int> p;
    csiParams(m_csi, p);
    if (p.empty()) p.push_back(0);

    for (size_t i = 0; i < p.size(); ++i) {
        const int v = p[i] < 0 ? 0 : p[i];
        switch (v) {
            case 0:
                m_pen = TerminalCell{};
                m_pen.fg = m_options.getForegroundColor();
                m_pen.bg = Color{0, 0, 0, 0};
                break;
            case 1:  m_pen.bold = true;       break;
            case 3:  m_pen.italic = true;     break;
            case 4:  m_pen.underline = true;  break;
            case 22: m_pen.bold = false;      break;
            case 23: m_pen.italic = false;    break;
            case 24: m_pen.underline = false; break;
            case 7:  m_pen.inverse = true;  break;
            case 27: m_pen.inverse = false; break;
            case 39: m_pen.fg = m_options.getForegroundColor(); break;
            case 49: m_pen.bg = Color{0, 0, 0, 0};              break;
            case 38: case 48: {
                const bool fg   = (v == 38);
                const int  mode = param(p, i + 1, -1);
                if (mode == 5) {
                    (fg ? m_pen.fg : m_pen.bg) = indexedColor(param(p, i + 2, 0));
                    i += 2;
                } else if (mode == 2) {
                    const Color c{
                        static_cast<uint8_t>(std::clamp(param(p, i + 2, 0), 0, 255)),
                        static_cast<uint8_t>(std::clamp(param(p, i + 3, 0), 0, 255)),
                        static_cast<uint8_t>(std::clamp(param(p, i + 4, 0), 0, 255)), 255};
                    (fg ? m_pen.fg : m_pen.bg) = c;
                    i += 4;
                }
                break;
            }
            default:
                if (v >= 30 && v <= 37)        m_pen.fg = m_options.getAnsiColor(v - 30);
                else if (v >= 40 && v <= 47)   m_pen.bg = m_options.getAnsiColor(v - 40);
                else if (v >= 90 && v <= 97)   m_pen.fg = m_options.getAnsiColor(v - 90 + 8);
                else if (v >= 100 && v <= 107) m_pen.bg = m_options.getAnsiColor(v - 100 + 8);
                break;
        }
    }
}


/*
    handleCsi(char final):
    - Params:   char final
    - Returns:  void
    - Desc:     Dispatches a completed control sequence: cursor movement and
                addressing, erase in line and display, insert and delete of
                characters and lines, scrolling, the scrolling region, save and
                restore of the cursor, and the private mode toggles for cursor
                visibility, application cursor keys and the alternate screen.
                Mouse-reporting and bracketed-paste modes are recognised and
                ignored, which is deliberate: acknowledging them silently is
                better than letting the program think they are unsupported and
                fall back to something stranger.
*/
void Terminal::handleCsi(char final) {
    const bool priv = !m_csi.empty() && m_csi[0] == '?';
    std::string body = priv ? m_csi.substr(1) : m_csi;

    std::vector<int> p;
    csiParams(body, p);

    switch (final) {
        case 'A': m_curRow = std::max(0, m_curRow - param(p, 0, 1)); m_wrapPending = false; break;
        case 'B': m_curRow = std::min(m_rows - 1, m_curRow + param(p, 0, 1)); m_wrapPending = false; break;
        case 'C': m_curCol = std::min(m_cols - 1, m_curCol + param(p, 0, 1)); m_wrapPending = false; break;
        case 'D': m_curCol = std::max(0, m_curCol - param(p, 0, 1)); m_wrapPending = false; break;
        case 'E': m_curRow = std::min(m_rows - 1, m_curRow + param(p, 0, 1)); m_curCol = 0; break;
        case 'F': m_curRow = std::max(0, m_curRow - param(p, 0, 1)); m_curCol = 0; break;
        case 'G': m_curCol = std::clamp(param(p, 0, 1) - 1, 0, m_cols - 1); m_wrapPending = false; break;
        case 'd': m_curRow = std::clamp(param(p, 0, 1) - 1, 0, m_rows - 1); break;
        case 'H': case 'f':
            m_curRow = std::clamp(param(p, 0, 1) - 1, 0, m_rows - 1);
            m_curCol = std::clamp(param(p, 1, 1) - 1, 0, m_cols - 1);
            m_wrapPending = false;
            break;

        case 'J': {
            const int mode = param(p, 0, 0);
            if (mode == 0)      clearRegion(m_curCol, m_curRow, m_cols - 1, m_rows - 1);
            else if (mode == 1) clearRegion(0, 0, m_curCol, m_curRow);
            else if (mode == 3) {
                /* Erase saved lines. `clear` sends this ahead of erasing the
                   screen, and it is what makes the scrollback go too rather
                   than leaving the old output a scroll away. */
                m_scrollback.clear();
                m_scrollPx = 0.f;
                clearSelection();
            } else {
                clearRegion(0, 0, m_cols - 1, m_rows - 1);
            }
            break;
        }
        case 'K': {
            const int mode = param(p, 0, 0);
            if (mode == 0)      clearRegion(m_curCol, m_curRow, m_cols - 1, m_curRow);
            else if (mode == 1) clearRegion(0, m_curRow, m_curCol, m_curRow);
            else                clearRegion(0, m_curRow, m_cols - 1, m_curRow);
            break;
        }

        case 'L': {   /* insert lines at the cursor, within the region */
            const int n = std::max(1, param(p, 0, 1));
            const int save = m_scrollTop;
            m_scrollTop = m_curRow;
            scrollDown(n);
            m_scrollTop = save;
            break;
        }
        case 'M': {   /* delete lines at the cursor */
            const int n = std::max(1, param(p, 0, 1));
            const int save = m_scrollTop;
            m_scrollTop = m_curRow;
            scrollUp(n);
            m_scrollTop = save;
            break;
        }
        case '@': {   /* insert blanks, pushing the rest of the line right */
            const int n = std::clamp(param(p, 0, 1), 1, m_cols - m_curCol);
            for (int c = m_cols - 1; c >= m_curCol + n; --c)
                cell(c, m_curRow) = cell(c - n, m_curRow);
            for (int c = m_curCol; c < m_curCol + n; ++c) {
                auto& cc = cell(c, m_curRow); cc = TerminalCell{};
                cc.fg = m_pen.fg; cc.bg = m_pen.bg;
            }
            break;
        }
        case 'P': {   /* delete characters, pulling the rest of the line left */
            const int n = std::clamp(param(p, 0, 1), 1, m_cols - m_curCol);
            for (int c = m_curCol; c < m_cols - n; ++c)
                cell(c, m_curRow) = cell(c + n, m_curRow);
            for (int c = m_cols - n; c < m_cols; ++c) {
                auto& cc = cell(c, m_curRow); cc = TerminalCell{};
                cc.fg = m_pen.fg; cc.bg = m_pen.bg;
            }
            break;
        }
        case 'X': {   /* erase characters in place */
            const int n = std::clamp(param(p, 0, 1), 1, m_cols - m_curCol);
            for (int c = m_curCol; c < m_curCol + n; ++c) {
                auto& cc = cell(c, m_curRow); cc = TerminalCell{};
                cc.fg = m_pen.fg; cc.bg = m_pen.bg;
            }
            break;
        }

        case 'S': scrollUp(std::max(1, param(p, 0, 1)));   break;
        case 'T': scrollDown(std::max(1, param(p, 0, 1))); break;

        case 'r': {
            const int top = std::clamp(param(p, 0, 1) - 1, 0, m_rows - 1);
            const int bot = std::clamp(param(p, 1, m_rows) - 1, 0, m_rows - 1);
            if (top < bot) { m_scrollTop = top; m_scrollBot = bot; }
            m_curCol = 0; m_curRow = m_scrollTop;
            break;
        }

        case 's': m_savedCol = m_curCol; m_savedRow = m_curRow; break;
        case 'u': m_curCol = m_savedCol; m_curRow = m_savedRow; break;

        case 'm': m_csi = body; applySgr(); break;

        case 'h': case 'l': {
            const bool on = (final == 'h');
            if (!priv) break;
            for (int v : p) {
                switch (v) {
                    case 25:   m_cursorVisible = on; break;
                    case 1:    m_appCursorKeys = on; break;
                    /* 47 and 1047 only switch screens. 1049 also saves and
                       restores the cursor and the current colours, which is
                       what makes a full-screen program's exit leave no trace. */
                    case 47:
                    case 1047: useAlternateScreen(on, false); break;
                    case 1049: useAlternateScreen(on, true);  break;
                    case 2004: m_bracketedPaste = on; break;
                    default: break;   /* mouse reporting modes: ignored */
                }
            }
            break;
        }
        default: break;
    }
    m_dirty = true;
}


/*
    handleOsc():
    - Params:   none
    - Returns:  void
    - Desc:     Operating-system commands, which are how a shell sets the window
                title and reports the working directory. Nothing here needs to
                affect the grid, so the sequence is consumed and discarded --
                which is the point: without consuming it the payload would be
                printed as literal text.
*/
void Terminal::handleOsc() {
    m_osc.clear();
}


/*
    handleControl(char c):
    - Params:   char c
    - Returns:  void
    - Desc:     The C0 control characters that move the cursor rather than
                printing: carriage return, line feed, backspace, tab to the next
                eight-column stop, and bell. Bell is swallowed rather than
                flashing anything, since a UI toolkit has no business making
                noise on a program's behalf.
*/
void Terminal::handleControl(char c) {
    switch (c) {
        case '\r': m_curCol = 0; m_wrapPending = false; break;
        case '\n': newline(); m_wrapPending = false; break;
        case '\b': if (m_curCol > 0) --m_curCol; m_wrapPending = false; break;
        case '\t': {
            const int next = ((m_curCol / 8) + 1) * 8;
            m_curCol = std::min(next, m_cols - 1);
            break;
        }
        case '\a': default: break;
    }
    m_dirty = true;
}


/*
    feed(const std::string& bytes):
    - Params:   const std::string& bytes
    - Returns:  void
    - Desc:     Runs the parser over a chunk of shell output. The state machine
                has four states -- ground, after an escape, inside a control
                sequence, inside an operating-system command -- which is enough
                for everything a shell and the common full-screen programs emit.
                A multi-byte character split across two reads is held in a buffer
                and completed on the next one, so a chunk boundary in the middle
                of a codepoint never corrupts it.
*/
void Terminal::feed(const std::string& bytes) {
    for (unsigned char b : bytes) {
        switch (m_state) {
            case P::Ground: {
                if (b == 0x1B) { m_state = P::Esc; m_utf8.clear(); break; }
                if (b < 0x20 || b == 0x7F) {
                    if (!m_utf8.empty()) m_utf8.clear();
                    handleControl(static_cast<char>(b));
                    break;
                }
                if (b < 0x80) { putChar(static_cast<char32_t>(b)); break; }

                /* Multi-byte: accumulate until the sequence is complete. */
                m_utf8 += static_cast<char>(b);
                const unsigned char lead = static_cast<unsigned char>(m_utf8[0]);
                size_t need = 1;
                if      ((lead & 0xE0) == 0xC0) need = 2;
                else if ((lead & 0xF0) == 0xE0) need = 3;
                else if ((lead & 0xF8) == 0xF0) need = 4;
                else { m_utf8.clear(); break; }

                if (m_utf8.size() >= need) {
                    char32_t cp = 0;
                    if (need == 2) cp = ((lead & 0x1F) << 6) | (m_utf8[1] & 0x3F);
                    else if (need == 3)
                        cp = ((lead & 0x0F) << 12) | ((m_utf8[1] & 0x3F) << 6) | (m_utf8[2] & 0x3F);
                    else
                        cp = ((lead & 0x07) << 18) | ((m_utf8[1] & 0x3F) << 12)
                           | ((m_utf8[2] & 0x3F) << 6) | (m_utf8[3] & 0x3F);
                    putChar(cp);
                    m_utf8.clear();
                }
                break;
            }

            case P::Esc: {
                if (b == '[')      { m_state = P::Csi; m_csi.clear(); }
                else if (b == ']') { m_state = P::Osc; m_osc.clear(); }
                else if (b == '(' || b == ')' || b == '#' || b == '%') {
                    /* charset selection: one more byte follows, ignore both */
                    m_state = P::Ground;
                    m_csi = "\x01";     /* marker: swallow the next byte */
                } else {
                    if (b == 'M') { if (m_curRow <= m_scrollTop) scrollDown(1); else --m_curRow; }
                    else if (b == '7') { m_savedCol = m_curCol; m_savedRow = m_curRow; }
                    else if (b == '8') { m_curCol = m_savedCol; m_curRow = m_savedRow; }
                    m_state = P::Ground;
                }
                break;
            }

            case P::Csi: {
                if (b >= 0x40 && b <= 0x7E) {
                    handleCsi(static_cast<char>(b));
                    m_state = P::Ground;
                } else if (m_csi.size() < 64) {
                    m_csi += static_cast<char>(b);
                }
                break;
            }

            case P::Osc: {
                if (b == 0x07) { handleOsc(); m_state = P::Ground; }
                else if (b == 0x1B) { /* ST follows as ESC \ */ handleOsc(); m_state = P::Esc; }
                else if (m_osc.size() < 512) m_osc += static_cast<char>(b);
                break;
            }
        }

        /* The charset-selection marker swallows exactly one following byte. */
        if (m_state == P::Ground && m_csi == "\x01") { m_csi.clear(); }
    }
}


/*
    contentArea():
    - Params:   none
    - Returns:  Rectf
    - Desc:     The rectangle the grid is drawn in: the bounds less the padding.
*/
Rectf Terminal::contentArea() const {
    const float s = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float p = m_options.getPadding() * s;
    return Rectf{
        { m_bounds.position.x + p, m_bounds.position.y + p },
        { std::max(0.f, m_bounds.size.x - 2 * p), std::max(0.f, m_bounds.size.y - 2 * p) }
    };
}


/*
    gridTopY():
    - Params:   none
    - Returns:  float
    - Desc:     The y of the grid's first row. A whole number of rows almost
                never divides the height exactly, and that leftover has to go
                somewhere; it goes above the grid, so the last row always ends
                exactly one padding above the bottom edge. Left at the bottom it
                reads as the text being cut off short, because it sits right
                where the next line would have been.
*/
float Terminal::gridTopY() const {
    const Rectf area = contentArea();
    const float used = m_rows * m_cellH;
    return area.position.y + std::max(0.f, area.size.y - used);
}


/*
    remeasure(float dt):
    - Params:   float dt
    - Returns:  void
    - Desc:     Works out the cell size from the font and, from that and the
                current bounds, how many columns and rows fit. The width comes
                from a single glyph's advance, which is why the font has to be
                monospaced. When the grid changes shape the PTY is told, so the
                shell reflows to match.
    - A new size has to hold still for a moment before it is applied. Dragging a
      window edge steps through a new grid size every few pixels, and a
      full-screen program told about each one restarts its layout over and over
      while still working on the previous change -- which is what makes such a
      program crawl to catch up. One drag should cost one reshape.
*/
void Terminal::remeasure(float dt) {
    if (!m_uiloRef) return;
    auto& renderer = m_uiloRef->getRenderer();

    const float scale = m_uiloRef->getScale();
    const unsigned cs = m_options.getCharSize();

    if (!m_fontLoaded || cs != m_lastCharSize || scale != m_lastScale) {
        const std::string_view resolved =
            Resources::get().fontRegistry().resolve(m_options.getFontPath());
        Font f = renderer.loadFont(std::string(resolved));
        if (!f.valid()) return;

        m_fontId       = f.id;
        m_fontLoaded   = true;
        m_lastCharSize = cs;
        m_lastScale    = scale;

        Font probe; probe.id = m_fontId;
        const float pxH = static_cast<float>(cs) * scale;
        const TextMetrics m = renderer.measureText("M", probe, pxH);
        m_cellW = m.size.x > 0.f ? m.size.x : pxH * 0.6f;
        m_cellH = std::max(1.f, m.lineHeight() * m_options.getLineSpacing());
    }

    if (m_cellW <= 0.f || m_cellH <= 0.f) return;

    const Rectf area = contentArea();

    const int cols = std::max(1, static_cast<int>(area.size.x / m_cellW));
    const int rows = std::max(1, static_cast<int>(area.size.y / m_cellH));

    if (cols == m_cols && rows == m_rows) {
        m_pendingCols = cols;
        m_pendingRows = rows;
        return;
    }

    /* First sizing lands at once -- there is nothing on screen to disturb and
       the shell has not started yet. */
    const bool firstSizing = m_grid.empty() || !m_pty.isOpen();

    if (cols != m_pendingCols || rows != m_pendingRows) {
        m_pendingCols = cols;
        m_pendingRows = rows;
        m_sizeSettled = 0.f;
    } else {
        m_sizeSettled += dt;
    }

    /* Long enough that a drag coalesces into one reshape, short enough that a
       single resize still feels immediate. */
    constexpr float kSettleSeconds = 0.12f;
    if (!firstSizing && m_sizeSettled < kSettleSeconds) return;

    m_sizeSettled = 0.f;
    resizeGrid(cols, rows);
    if (m_pty.isOpen()) m_pty.resize(cols, rows);
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves bounds, re-measures the grid, starts the shell on the
                first pass when autoStart is on, drains whatever the shell has
                written and parses it, and advances the cursor blink. Draining
                happens here rather than on a thread so the grid is only ever
                touched from one place, which is what keeps it safe to read from
                render without any locking.
*/
void Terminal::update(Rectf& parentBounds, float dt) {
    resize(parentBounds);
    remeasure(dt);

    if (!m_started && m_options.getAutoStart() && m_uiloRef) start();

    if (m_pty.isOpen()) {
        m_pending.clear();
        if (m_pty.read(m_pending) > 0) {
            if (const auto& cb = m_options.getOnOutput()) cb(m_pending);
            feed(m_pending);
        }
        if (m_pty.childExited()) {
            m_pty.close();
            m_started = false;
            if (!m_exitFired) {
                m_exitFired = true;
                if (const auto& cb = m_options.getOnExit()) cb();
            }
            m_dirty = true;
        }
    }

    /* Drag-select. The button is polled rather than waited on, the way Textbox
       does it: a release can land anywhere, including outside the widget. */
    if (m_mouseDown) {
        float mx, my;
        const uint32_t btns = SDL_GetMouseState(&mx, &my);
        if (!(btns & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) {
            m_mouseDown = false;
        } else if (m_uiloRef) {
            const Vec2f mp   = m_uiloRef->getMousePosition();
            const long  line = hitTestLine(mp);
            const int   col  = hitTestColumn(mp);
            if (line != m_selHeadLine || col != m_selHeadCol) {
                m_selHeadLine = line;
                m_selHeadCol  = col;
                /* Only a drag that actually moved off the anchor is a
                   selection, so a plain click just clears the old one. */
                m_hasSelection = (m_selHeadLine != m_selAnchorLine ||
                                  m_selHeadCol  != m_selAnchorCol);
                m_dirty = true;
            }
        }
    }

    if (m_focused && m_options.getBlinkRate() > 0.f) {
        m_blinkTimer += dt;
        const float half = m_options.getBlinkRate() * 0.5f;
        if (m_blinkTimer >= half) { m_blinkTimer -= half; m_blinkOn = !m_blinkOn; m_dirty = true; }
    } else {
        m_blinkOn = true;
    }
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the surface, then the visible rows, then the cursor,
                clipped to the widget. Background cells are emitted as merged
                runs rather than one rect per cell, and text likewise as runs of
                identical style, so a full screen costs a few dozen draws instead
                of a few thousand. Rows come from scrollback first when the view
                is scrolled back.
*/
void Terminal::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    if (!m_uiloRef) { m_dirty = false; return; }
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) { m_dirty = false; return; }

    auto& r = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    const Color bg = resolveColor(m_options.getBackgroundColorRole(),
                                  m_options.getBackgroundColor());
    const float rounding = m_options.getRounding() * scale;
    if (bg.a > 0) {
        if (rounding <= 0.f) r.draw(Rect{m_bounds.position, m_bounds.size, bg});
        else r.draw(RoundedRect{m_bounds.position, m_bounds.size, rounding, 8u, bg});
    }

    if (!m_fontLoaded || m_cellW <= 0.f) { m_dirty = false; return; }

    const Rectf area = contentArea();
    r.pushRoundClip(m_bounds, rounding);
    /* A second, square clip on the text. The row loop starts one row early so a
       partly scrolled-in line is drawn, and that row would otherwise paint into
       the padding above the text -- a sliver of the previous line hanging over
       the top edge. */
    const Rectf textClip = area;
    r.pushRoundClip(textClip, 0.f);

    /* Rows hang from gridTopY(), which keeps the sub-row leftover above the
       grid so the last row finishes flush against the bottom inset. */
    const float gridTop = gridTopY();

    /* Reverse video is applied here rather than stored swapped. A cell with no
       background of its own takes the widget's, which is the only thing that
       makes "inverse on the default background" come out right. */
    auto effFg = [&](const TerminalCell& c) {
        return c.inverse ? (c.bg.a == 0 ? bg : c.bg) : c.fg;
    };
    auto effBg = [&](const TerminalCell& c) {
        return c.inverse ? c.fg : c.bg;
    };

    Font font; font.id = m_fontId;
    const float pxH = static_cast<float>(m_options.getCharSize()) * scale;

    const int sb = static_cast<int>(m_scrollback.size());

    /* Split the scroll into whole rows and the leftover within a row. The
       remainder shifts every row down by a fraction of its height, which is
       what makes the view glide rather than jump a line at a time. */
    const float scrollPx = std::clamp(m_scrollPx, 0.f, sb * m_cellH);
    const int   off      = static_cast<int>(std::floor(scrollPx / m_cellH));
    const float frac     = scrollPx - off * m_cellH;

    /* Starting a row early draws the one that is only partly scrolled in; the
       round clip takes care of the part hanging above the top edge. */
    for (int row = -1; row < m_rows; ++row) {
        const int src = row - off;   /* < 0 means it comes from scrollback */
        const TerminalCell* line = nullptr;
        int lineLen = 0;
        if (src >= 0) {
            if (src >= m_rows) break;
            line    = &m_grid[static_cast<size_t>(src) * m_cols];
            lineLen = m_cols;
        } else {
            const int idx = sb + src;
            if (idx < 0 || idx >= sb) continue;
            const auto& hist = m_scrollback[static_cast<size_t>(idx)];
            if (hist.empty()) continue;
            line = hist.data();
            /* A row kept from before a widening is narrower than the grid.
               Reading it to the grid's edge walks off the end of it, which is
               where the stray glyphs and colour blocks came from. */
            lineLen = static_cast<int>(hist.size());
        }
        const int nCells = std::min(m_cols, lineLen);

        const float y = gridTop + row * m_cellH + frac;
        if (y > textClip.position.y + textClip.size.y) break;

        /* Selection highlight, under the cell backgrounds so a coloured cell
           still reads as selected rather than being painted over. */
        if (m_hasSelection) {
            long fromLine, toLine;
            int  fromCol,  toCol;
            normalizedSelection(fromLine, fromCol, toLine, toCol);

            const long abs = absoluteTopLine() - off + row;
            if (abs >= fromLine && abs <= toLine) {
                const int lo = (abs == fromLine) ? std::clamp(fromCol, 0, nCells) : 0;
                const int hi = (abs == toLine)   ? std::clamp(toCol,   0, nCells) : nCells;
                if (hi > lo) {
                    r.draw(Rect{{area.position.x + lo * m_cellW, y},
                                {(hi - lo) * m_cellW, m_cellH},
                                m_options.getSelectionColor()});
                }
            }
        }

        /* Background runs. */
        int c = 0;
        while (c < nCells) {
            const Color col = effBg(line[c]);
            int e = c + 1;
            while (e < nCells) {
                const Color n = effBg(line[e]);
                if (n.r != col.r || n.g != col.g || n.b != col.b || n.a != col.a) break;
                ++e;
            }
            if (col.a > 0) {
                r.draw(Rect{{area.position.x + c * m_cellW, y},
                            {(e - c) * m_cellW, m_cellH}, col});
            }
            c = e;
        }

        /* Text runs of identical colour and style. */
        c = 0;
        while (c < nCells) {
            const TerminalCell& first = line[c];
            if (first.ch == U' ' || first.ch == 0) { ++c; continue; }

            const Color firstFg = effFg(first);
            int e = c;
            std::string run;
            while (e < nCells) {
                const TerminalCell& cc = line[e];
                if (cc.ch == U' ' || cc.ch == 0) break;
                const Color f = effFg(cc);
                if (f.r != firstFg.r || f.g != firstFg.g ||
                    f.b != firstFg.b || f.a != firstFg.a ||
                    cc.bold != first.bold || cc.italic != first.italic) break;
                run += utf8Encode(cc.ch);
                ++e;
            }
            if (!run.empty()) {
                r.drawText(run, {area.position.x + c * m_cellW, y}, font, pxH,
                           firstFg, TextStyle{first.bold, first.italic});
            }
            c = (e > c) ? e : c + 1;
        }
    }

    /* Cursor. It rides with the rest of the screen rather than being switched
       off the moment the view moves: scrolling back by less than a whole line
       leaves the screen looking untouched, and hiding the cursor there just
       looks like it vanished. It disappears when it is genuinely scrolled out
       of sight, which is the only reason it should. */
    const float curY = gridTop + (m_curRow + off) * m_cellH + frac;
    const bool  curOnScreen = curY + m_cellH > textClip.position.y
                           && curY < textClip.position.y + textClip.size.y;

    if (m_focused && m_cursorVisible && m_blinkOn && curOnScreen) {
        const Color cc = resolveColor(m_options.getCursorColorRole(),
                                      m_options.getCursorColor());
        r.draw(Rect{{area.position.x + m_curCol * m_cellW, curY},
                    {std::max(1.f, m_cellW), m_cellH}, cc});

        const TerminalCell& under = cell(m_curCol, m_curRow);
        if (under.ch != U' ' && under.ch != 0) {
            r.drawText(utf8Encode(under.ch),
                       {area.position.x + m_curCol * m_cellW, curY},
                       font, pxH, bg, TextStyle{under.bold, under.italic});
        }
    }

    r.popRoundClip();   /* text area */
    r.popRoundClip();   /* widget bounds */
    m_dirty = false;
}


/*
    screenText():
    - Params:   none
    - Returns:  std::string
    - Desc:     Everything on the live screen as text, rows joined by newlines
                with trailing blanks trimmed. Intended for tests and for a
                "copy everything" action.
*/
std::string Terminal::screenText() const {
    std::string out;
    for (int row = 0; row < m_rows; ++row) {
        std::string line;
        for (int col = 0; col < m_cols; ++col) {
            const TerminalCell& c = m_grid[static_cast<size_t>(row) * m_cols + col];
            line += utf8Encode(c.ch == 0 ? U' ' : c.ch);
        }
        while (!line.empty() && line.back() == ' ') line.pop_back();
        out += line;
        if (row + 1 < m_rows) out += '\n';
    }
    while (!out.empty() && out.back() == '\n') out.pop_back();
    return out;
}


/*
    viewText():
    - Params:   none
    - Returns:  std::string
    - Desc:     The rows currently on display, which is the live screen when
                scrolled to the bottom and history when scrolled up. Rows that
                have aged out of the scrollback come back blank rather than
                shifting the rest.
*/
std::string Terminal::viewText() const {
    if (m_cellH <= 0.f) return screenText();

    const long back = static_cast<long>(std::floor(m_scrollPx / m_cellH));
    const long top  = absoluteTopLine() - back;

    std::string out;
    for (int row = 0; row < m_rows; ++row) {
        int len = 0;
        const TerminalCell* line = lineAt(top + row, len);
        std::string text;
        if (line)
            for (int c = 0; c < std::min(m_cols, len); ++c)
                text += utf8Encode(line[c].ch == 0 ? U' ' : line[c].ch);
        while (!text.empty() && text.back() == ' ') text.pop_back();
        out += text;
        if (row + 1 < m_rows) out += '\n';
    }
    while (!out.empty() && out.back() == '\n') out.pop_back();
    return out;
}


/*
    scrollToBottom():
    - Params:   none
    - Returns:  void
    - Desc:     Returns the view to the live screen.
*/
void Terminal::scrollToBottom() {
    if (m_scrollPx == 0.f) return;
    m_scrollPx = 0.f;
    m_dirty    = true;
}


/*
    lineAt(long absolute, int& len):
    - Params:   long absolute, int& len
    - Returns:  const TerminalCell* -- null when that line is no longer held
    - Desc:     One line of text by absolute line number, whether it is still on
                the live grid or has aged into scrollback. Lines that have
                fallen off the end of the scrollback are gone and report null.
    - `len` is the row's own width, which is not always the grid's. A row
      captured before the terminal was widened keeps the width it had, so every
      caller has to stop at `len` rather than at m_cols -- reading to the wider
      grid's edge walks off the end of the row.
*/
const TerminalCell* Terminal::lineAt(long absolute, int& len) const {
    len = 0;
    const long top = absoluteTopLine();

    if (absolute >= top) {
        const long row = absolute - top;
        if (row >= m_rows) return nullptr;
        len = m_cols;
        return &m_grid[static_cast<size_t>(row) * m_cols];
    }

    /* Behind the screen: index back through the history that is still held. */
    const long sb  = static_cast<long>(m_scrollback.size());
    const long idx = sb - (top - absolute);
    if (idx < 0 || idx >= sb) return nullptr;
    const auto& hist = m_scrollback[static_cast<size_t>(idx)];
    if (hist.empty()) return nullptr;
    len = static_cast<int>(hist.size());
    return hist.data();
}


/*
    hitTestLine(const Vec2f& pos):
    - Params:   const Vec2f& pos
    - Returns:  long -- absolute line number under the position
    - Desc:     Which line of text a point falls on, taking the scrolled-back
                view into account.
*/
long Terminal::hitTestLine(const Vec2f& pos) const {
    if (m_cellH <= 0.f) return absoluteTopLine();
    const long  row  = static_cast<long>(std::floor((pos.y - gridTopY()) / m_cellH));
    const long  back = static_cast<long>(std::floor(m_scrollPx / m_cellH));
    return absoluteTopLine() - back + row;
}


/*
    hitTestColumn(const Vec2f& pos):
    - Params:   const Vec2f& pos
    - Returns:  int -- column under the position
    - Desc:     Which column a point falls on. Rounding rather than truncating
                puts the boundary at the middle of a cell, so dragging across a
                character either takes all of it or none of it.
*/
int Terminal::hitTestColumn(const Vec2f& pos) const {
    if (m_cellW <= 0.f) return 0;
    const Rectf area = contentArea();
    const int   col  = static_cast<int>(
        std::lround((pos.x - area.position.x) / m_cellW));
    return std::clamp(col, 0, m_cols);
}


/*
    normalizedSelection(long& fromLine, int& fromCol, long& toLine, int& toCol):
    - Params:   long& fromLine, int& fromCol, long& toLine, int& toCol
    - Returns:  void
    - Desc:     The selection ordered so the start comes before the end, since a
                drag upward or leftward leaves the anchor after the head.
*/
void Terminal::normalizedSelection(
    long& fromLine,
    int&  fromCol,
    long& toLine,
    int&  toCol
) const {
    const bool anchorFirst =
        m_selAnchorLine < m_selHeadLine ||
        (m_selAnchorLine == m_selHeadLine && m_selAnchorCol <= m_selHeadCol);

    fromLine = anchorFirst ? m_selAnchorLine : m_selHeadLine;
    fromCol  = anchorFirst ? m_selAnchorCol  : m_selHeadCol;
    toLine   = anchorFirst ? m_selHeadLine   : m_selAnchorLine;
    toCol    = anchorFirst ? m_selHeadCol    : m_selAnchorCol;
}


/*
    selectedText():
    - Params:   none
    - Returns:  std::string
    - Desc:     The selected text. A selection spanning several lines takes the
                first line from its start column to the end, the last from its
                beginning to the end column, and everything in between whole.
                Trailing blanks go, so the padding a cell grid keeps does not
                come along with the paste.
*/
std::string Terminal::selectedText() const {
    if (!m_hasSelection) return {};

    long fromLine, toLine;
    int  fromCol,  toCol;
    normalizedSelection(fromLine, fromCol, toLine, toCol);

    std::string out;
    for (long ln = fromLine; ln <= toLine; ++ln) {
        int len = 0;
        const TerminalCell* line = lineAt(ln, len);
        if (!line) continue;

        const int width = std::min(m_cols, len);
        const int lo = (ln == fromLine) ? std::clamp(fromCol, 0, width) : 0;
        const int hi = (ln == toLine)   ? std::clamp(toCol,   0, width) : width;

        std::string text;
        for (int c = lo; c < hi; ++c)
            text += utf8Encode(line[c].ch == 0 ? U' ' : line[c].ch);
        while (!text.empty() && text.back() == ' ') text.pop_back();

        if (ln != fromLine) out += '\n';
        out += text;
    }
    return out;
}


/*
    clearSelection():
    - Params:   none
    - Returns:  void
    - Desc:     Drops the selection and repaints without the highlight.
*/
void Terminal::clearSelection() {
    if (!m_hasSelection) return;
    m_hasSelection = false;
    m_dirty        = true;
}


/*
    selectAll():
    - Params:   none
    - Returns:  void
    - Desc:     Selects the scrollback and the live screen together.
*/
void Terminal::selectAll() {
    m_selAnchorLine = absoluteTopLine() - static_cast<long>(m_scrollback.size());
    m_selAnchorCol  = 0;
    m_selHeadLine   = absoluteTopLine() + m_rows - 1;
    m_selHeadCol    = m_cols;
    m_hasSelection  = true;
    m_dirty         = true;
}


/*
    selectRange(long fromLine, int fromCol, long toLine, int toCol):
    - Params:   long fromLine, int fromCol, long toLine, int toCol
    - Returns:  void
    - Desc:     Selects a range of absolute line numbers. Nothing is clamped to
                what is currently held: a range reaching past the end of the
                scrollback simply yields the part that survives.
*/
void Terminal::selectRange(long fromLine, int fromCol, long toLine, int toCol) {
    m_selAnchorLine = fromLine;
    m_selAnchorCol  = fromCol;
    m_selHeadLine   = toLine;
    m_selHeadCol    = toCol;
    m_hasSelection  = true;
    m_dirty         = true;
}


/*
    copySelection():
    - Params:   none
    - Returns:  void
    - Desc:     Puts the selection on the system clipboard, leaving it alone
                when nothing is selected.
*/
void Terminal::copySelection() const {
    const std::string sel = selectedText();
    if (!sel.empty()) SDL_SetClipboardText(sel.c_str());
}


/*
    pasteClipboard():
    - Params:   none
    - Returns:  void
    - Desc:     Sends the clipboard to the shell as though it had been typed.
                Carriage returns are normalised to newlines, since a pasted
                CRLF would otherwise submit a line twice. When the running
                program has asked for bracketed paste the text is fenced, which
                is what lets an editor take a multi-line paste literally instead
                of acting on every newline in it.
*/
void Terminal::pasteClipboard() {
    if (!m_pty.isOpen()) return;

    const char* raw = SDL_GetClipboardText();
    if (!raw) return;
    std::string text(raw);
    SDL_free(const_cast<char*>(raw));
    if (text.empty()) return;

    std::string clean;
    clean.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') continue;
            clean += '\n';
        } else {
            clean += text[i];
        }
    }

    if (m_bracketedPaste) m_pty.write("\x1b[200~");
    m_pty.write(clean);
    if (m_bracketedPaste) m_pty.write("\x1b[201~");

    m_scrollPx = 0.f;
}


/*
    checkLeftClick(const Vec2f& mousePos):
    - Params:   const Vec2f& mousePos
    - Returns:  bool -- true when the click landed on the terminal
    - Desc:     Takes focus, so keystrokes start going to the shell.
*/
bool Terminal::checkLeftClick(const Vec2f& mousePos) {
    if (!m_bounds.contains(mousePos)) return false;
    if (m_uiloRef) m_uiloRef->setCurrInteractible(this);
    m_focused   = true;
    m_blinkOn   = true;
    m_blinkTimer = 0.f;

    /* A press drops the anchor but does not select anything yet: a plain click
       should clear the old selection, and only dragging makes a new one. */
    m_mouseDown     = true;
    m_selAnchorLine = hitTestLine(mousePos);
    m_selAnchorCol  = hitTestColumn(mousePos);
    m_selHeadLine   = m_selAnchorLine;
    m_selHeadCol    = m_selAnchorCol;
    m_hasSelection  = false;

    m_dirty     = true;
    return true;
}


/*
    checkHover(const Vec2f& mousePos):
    - Params:   const Vec2f& mousePos
    - Returns:  bool -- true when the pointer is over the terminal
    - Desc:     Asks for the text cursor while the pointer is inside.
*/
bool Terminal::checkHover(const Vec2f& mousePos) {
    if (!m_bounds.contains(mousePos)) return false;
    if (m_uiloRef) m_uiloRef->requestCursor(CursorType::Text, 1);
    return true;
}


/*
    checkScroll(const Vec2f& mousePos, float delta, bool precise, bool momentum):
    - Params:   const Vec2f& mousePos, float delta, bool precise, bool momentum
    - Returns:  bool -- true when the terminal consumed the event
    - Desc:     Scrolls back through history. The shell is never told: as far as
                it is concerned the screen has not moved, which is why scrolling
                back during a running program does not disturb it. Declines when
                there is no history, so the event bubbles to the page.
*/
bool Terminal::checkScroll(const Vec2f& mousePos, float delta, bool precise, bool /*momentum*/) {
    if (!m_bounds.contains(mousePos)) return false;

    const int sb = static_cast<int>(m_scrollback.size());
    if (sb <= 0) return false;

    /* The same conversion Textbox and a scrollable Column use, so one gesture
       moves all three by the same distance. */
    const float speed = m_options.getScrollSpeed();
    const float step  = precise ? 30.f * (speed / 40.f) : speed;

    const float maxScroll = sb * m_cellH;
    const float next = std::clamp(m_scrollPx + delta * step, 0.f, maxScroll);
    if (next != m_scrollPx) { m_scrollPx = next; m_dirty = true; }
    return true;
}


/*
    onDeactivate():
    - Params:   none
    - Returns:  void
    - Desc:     Releases focus when something else is clicked, so typing stops
                reaching the shell and the cursor stops blinking.
*/
void Terminal::onDeactivate() {
    m_focused = false;
    m_dirty   = true;
}


/*
    handleTextInput(char32_t unicode):
    - Params:   char32_t unicode
    - Returns:  void
    - Desc:     Sends a typed character to the shell. Nothing is echoed locally:
                the shell decides what appears, which is what makes a password
                prompt hide input without the widget knowing anything about it.
*/
void Terminal::handleTextInput(char32_t unicode) {
    if (!m_focused || !m_pty.isOpen()) return;
    if (unicode < 32u && unicode != U'\t') return;
    m_pty.write(utf8Encode(unicode));
    m_scrollPx = 0.f;
}


/*
    sendKey(SDL_Keycode key, bool shift, bool ctrl):
    - Params:   SDL_Keycode key, bool shift, bool ctrl
    - Returns:  void
    - Desc:     Encodes a non-printing key the way a terminal is expected to.
                Arrows and Home/End switch between the normal CSI forms and the
                SS3 forms when the program has asked for application cursor keys
                -- getting that wrong is what makes arrow keys print letters
                inside a full-screen editor. Ctrl with a letter becomes the
                matching control character, which is how interrupt and
                end-of-file reach the shell at all.
*/
void Terminal::sendKey(SDL_Keycode key, bool shift, bool ctrl) {
    const char* seq = nullptr;
    std::string tmp;

    const bool app = m_appCursorKeys;
    switch (key) {
        case SDLK_RETURN: case SDLK_KP_ENTER: seq = "\r";   break;
        case SDLK_BACKSPACE:                  seq = "\x7f"; break;
        case SDLK_TAB:                        seq = shift ? "\x1b[Z" : "\t"; break;
        case SDLK_ESCAPE:                     seq = "\x1b"; break;
        case SDLK_UP:    seq = app ? "\x1bOA" : "\x1b[A"; break;
        case SDLK_DOWN:  seq = app ? "\x1bOB" : "\x1b[B"; break;
        case SDLK_RIGHT: seq = app ? "\x1bOC" : "\x1b[C"; break;
        case SDLK_LEFT:  seq = app ? "\x1bOD" : "\x1b[D"; break;
        case SDLK_HOME:  seq = app ? "\x1bOH" : "\x1b[H"; break;
        case SDLK_END:   seq = app ? "\x1bOF" : "\x1b[F"; break;
        case SDLK_DELETE:   seq = "\x1b[3~"; break;
        case SDLK_PAGEUP:   seq = "\x1b[5~"; break;
        case SDLK_PAGEDOWN: seq = "\x1b[6~"; break;
        default: break;
    }

    if (!seq && ctrl && key >= SDLK_A && key <= SDLK_Z) {
        tmp = std::string(1, static_cast<char>(key - SDLK_A + 1));
        seq = tmp.c_str();
    }
    if (seq) m_pty.write(seq, std::strlen(seq));
}


/*
    handleKeyInput(SDL_Keycode key, bool shift, bool ctrl):
    - Params:   SDL_Keycode key, bool shift, bool ctrl
    - Returns:  void
    - Desc:     Routes a key to the shell, and snaps the view back to the bottom
                first -- typing while scrolled back should show what is being
                typed. Printable characters arrive through handleTextInput
                instead, so only the keys that need encoding are handled here.
*/
void Terminal::handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui) {
    if (!m_focused || !m_pty.isOpen()) return;

    /* A modifier on its own is the first half of a shortcut, not input. It must
       not reach the shell and must not clear the selection -- otherwise the
       Command in Command+C would wipe what the C was about to copy. */
    switch (key) {
        case SDLK_LCTRL:  case SDLK_RCTRL:
        case SDLK_LSHIFT: case SDLK_RSHIFT:
        case SDLK_LALT:   case SDLK_RALT:
        case SDLK_LGUI:   case SDLK_RGUI:
            return;
        default: break;
    }

    /* Ctrl+C has to stay an interrupt -- that is the whole point of it in a
       terminal -- so the clipboard lives on Command, plus the Ctrl+Shift forms
       every Linux terminal uses. Ctrl alone is never a clipboard shortcut here,
       which is the one place UILO does not treat the two as interchangeable. */
    const bool clip = gui || (ctrl && shift);
    if (clip) {
        switch (key) {
            case SDLK_C:
                /* Copy only when there is something to copy; otherwise fall
                   through so Ctrl+Shift+C still reaches the shell. */
                if (m_hasSelection) { copySelection(); m_dirty = true; return; }
                break;
            case SDLK_V: pasteClipboard(); m_dirty = true; return;
            case SDLK_A: selectAll();      return;
            default: break;
        }
    }

    m_scrollPx = 0.f;
    m_blinkOn    = true;
    m_blinkTimer = 0.f;
    clearSelection();
    sendKey(key, shift, ctrl);
    m_dirty = true;
}

}
