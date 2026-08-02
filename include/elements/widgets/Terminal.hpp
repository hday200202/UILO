#pragma once

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "../interactible/Interactible.hpp"
#include "../../platform/Pty.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    TerminalCell:
    - Desc: One character cell: the codepoint it shows and the colours and style
            it is drawn with. Colours are resolved to literals when the cell is
            written rather than stored as palette roles, because a terminal's
            colours come from the program it is running, not from the theme.
*/
struct TerminalCell {
    char32_t ch        = U' ';
    Color    fg        = Color{200, 200, 200, 255};
    Color    bg        = Color{0, 0, 0, 0};
    bool     bold      = false;
    bool     italic    = false;
    bool     underline = false;
    // Reverse video. Kept as a flag and applied when the cell is drawn rather
    // than by swapping the two colours here: a swap cannot be undone, because
    // "default background" is an absence of colour and swapping loses it.
    bool     inverse   = false;
};


/*
    TerminalOptions:
    - Desc: Everything a Terminal draws and how it starts its shell: the font and
            cell metrics, the default colours, the sixteen-colour palette the
            escape sequences index into, the scrollback depth, and which shell to
            launch. Colours come as a literal plus a role like the rest of the
            library, but only for the widget's own surface -- the per-cell
            colours a program emits are always literals.
    - A terminal has to be monospaced to lay out as a grid, so the font should be
      one. Cell size is measured from the font rather than configured, which is
      what keeps the grid aligned at any character size.
*/
class TerminalOptions {
public:
    TerminalOptions() = default;

    // Shell. Empty follows $SHELL, then /bin/sh.
    TerminalOptions& setShell(const std::string& s)      { m_shell = s; return *this; }
    // Start the shell on the first update rather than waiting for start().
    TerminalOptions& setAutoStart(bool v)                { m_autoStart = v; return *this; }

    // Text
    TerminalOptions& setFont(const std::string& path)    { m_fontPath = path; return *this; }
    TerminalOptions& setCharSize(unsigned int n)         { m_charSize = n; return *this; }
    // Multiplier on the font's own line height, for tightening or opening up
    // the rows without changing the glyph size.
    TerminalOptions& setLineSpacing(float f)             { m_lineSpacing = f; return *this; }

    // Surface
    TerminalOptions& setBackgroundColor(const Color& c)          { m_bgColor = c; return *this; }
    TerminalOptions& setBackgroundColorRole(const std::string& r){ m_bgColorRole = r; return *this; }
    TerminalOptions& setForegroundColor(const Color& c)          { m_fgColor = c; return *this; }
    TerminalOptions& setForegroundColorRole(const std::string& r){ m_fgColorRole = r; return *this; }
    TerminalOptions& setRounding(float r)                { m_rounding = r; return *this; }
    TerminalOptions& setPadding(float p)                 { m_padding = p; return *this; }

    // Cursor
    TerminalOptions& setCursorColor(const Color& c)           { m_cursorColor = c; return *this; }
    TerminalOptions& setCursorColorRole(const std::string& r) { m_cursorColorRole = r; return *this; }
    TerminalOptions& setCursorBlinkRate(float seconds)        { m_blinkRate = seconds; return *this; }

    TerminalOptions& setSelectionColor(const Color& c)        { m_selectionColor = c; return *this; }

    // How many scrolled-off rows to keep. 0 keeps none.
    TerminalOptions& setScrollback(int lines)            { m_scrollback = lines; return *this; }
    TerminalOptions& setScrollSpeed(float s)             { m_scrollSpeed = s; return *this; }

    // One of the sixteen colours an escape sequence can name. Index 0-7 are the
    // normal ones, 8-15 the bright ones.
    TerminalOptions& setAnsiColor(int index, const Color& c);

    // Fired when the shell exits, with its own text as the argument.
    TerminalOptions& setOnExit(std::function<void()> f)  { m_onExit = std::move(f); return *this; }
    // Fired for every chunk the shell writes, before it is parsed. For logging
    // or for a caller that wants to watch the raw stream.
    TerminalOptions& setOnOutput(std::function<void(const std::string&)> f) {
        m_onOutput = std::move(f); return *this;
    }

    const std::string& getShell()       const { return m_shell; }
    bool               getAutoStart()   const { return m_autoStart; }
    const std::string& getFontPath()    const;
    unsigned int       getCharSize()    const;
    float              getLineSpacing() const { return m_lineSpacing; }

    Color              getBackgroundColor()     const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    Color              getForegroundColor()     const { return m_fgColor; }
    const std::string& getForegroundColorRole() const { return m_fgColorRole; }
    float              getRounding()    const;
    float              getPadding()     const { return m_padding; }

    Color              getCursorColor()     const { return m_cursorColor; }
    const std::string& getCursorColorRole() const { return m_cursorColorRole; }
    float              getBlinkRate()   const { return m_blinkRate; }
    Color              getSelectionColor() const { return m_selectionColor; }

    int                getScrollback()  const { return m_scrollback; }
    float              getScrollSpeed() const { return m_scrollSpeed; }
    Color              getAnsiColor(int index) const;

    const std::function<void()>& getOnExit() const { return m_onExit; }
    const std::function<void(const std::string&)>& getOnOutput() const { return m_onOutput; }

private:
    std::string m_shell;
    bool        m_autoStart = true;

    std::string          m_fontPath;
    std::optional<unsigned int> m_charSize;
    float                m_lineSpacing = 1.f;

    Color       m_bgColor     = Color{16, 16, 20, 255};
    std::string m_bgColorRole = "panel";
    Color       m_fgColor     = Color{205, 210, 220, 255};
    std::string m_fgColorRole = "text";

    std::optional<float> m_rounding;
    float                m_padding = 6.f;

    Color       m_cursorColor     = Color{220, 220, 230, 255};
    std::string m_cursorColorRole = "text";
    float       m_blinkRate       = 1.0f;
    Color       m_selectionColor  = Color{70, 130, 200, 110};

    int   m_scrollback  = 2000;
    float m_scrollSpeed = 40.f;

    // xterm's usual sixteen. A program naming "red" means this red, not the
    // theme's, so these are literals rather than roles.
    Color m_ansi[16] = {
        {  0,   0,   0, 255}, {205,  49,  49, 255}, { 13, 188, 121, 255}, {229, 229,  16, 255},
        { 36, 114, 200, 255}, {188,  63, 188, 255}, { 17, 168, 205, 255}, {229, 229, 229, 255},
        {102, 102, 102, 255}, {241,  76,  76, 255}, { 35, 209, 139, 255}, {245, 245,  67, 255},
        { 59, 142, 234, 255}, {214, 112, 214, 255}, { 41, 184, 219, 255}, {255, 255, 255, 255},
    };

    std::function<void()>                   m_onExit;
    std::function<void(const std::string&)> m_onOutput;
};


/*
    setAnsiColor(int index, const Color& c):
    - Params:   int index, const Color& c
    - Returns:  TerminalOptions&
    - Desc:     Overrides one of the sixteen colours escape sequences index into.
                An index outside 0..15 is ignored rather than clamped, so a
                mistake does not silently recolour something else.
*/
inline TerminalOptions& TerminalOptions::setAnsiColor(int index, const Color& c) {
    if (index >= 0 && index < 16) m_ansi[index] = c;
    return *this;
}


/*
    getAnsiColor(int index):
    - Params:   int index
    - Returns:  Color
    - Desc:     One of the sixteen indexed colours, or the default foreground for
                an index outside the range.
*/
inline Color TerminalOptions::getAnsiColor(int index) const {
    return (index >= 0 && index < 16) ? m_ansi[index] : m_fgColor;
}


/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for the cell grid, falling back to the active Theme's. It
                should be monospaced; a proportional face still renders but the
                columns will not line up, since the grid is measured from one
                glyph's advance.
*/
inline const std::string& TerminalOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    getCharSize():
    - Params:   none
    - Returns:  unsigned int
    - Desc:     Character size in unscaled pixels, resolved from this widget,
                then the Theme, then 14.
*/
inline unsigned int TerminalOptions::getCharSize() const {
    return Theme::resolveCharSize(m_charSize, 14);
}


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the terminal's surface, resolved from this
                widget, then the Theme, then 0.
*/
inline float TerminalOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, 0.f);
}


/*
    Terminal:
    - Desc: A terminal emulator: a real shell on a pseudo-terminal, its output
            parsed into a character grid and drawn, and keystrokes encoded back
            to it. Native only -- it needs a PTY and a process, neither of which
            the Wt backend has, so on the web it renders as an inert panel.
    - The grid is sized from the font's own metrics and the element's bounds, so
      resizing the window reflows the shell: the new dimensions are pushed
      through to the PTY, which raises SIGWINCH and makes a running program
      redraw at the new size.
    - Output is parsed by a small state machine covering what a shell and the
      common full-screen programs actually emit: SGR colour and style including
      256-colour and true colour, cursor addressing, erase, insert and delete,
      scrolling regions, and the alternate screen. Anything unrecognised is
      swallowed rather than printed, so an unhandled sequence leaves no litter
      on screen.
    - Scrollback is kept as whole rows pushed off the top. Scrolling the view
      only changes what is drawn; the shell is never told, because as far as it
      is concerned the screen is still the same size.
*/
class Terminal : public Interactible {
public:
    explicit Terminal(
        Modifier modifier,
        TerminalOptions options = {},
        const std::string& name = ""
    );
    ~Terminal() override;

    const TerminalOptions& getOptions() const { return m_options; }
    TerminalOptions&       getOptions()       { return m_options; }
    void                   setOptions(const TerminalOptions& opts);

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    bool checkLeftClick(const Vec2f& mousePos) override;
    bool checkHover(const Vec2f& mousePos) override;
    bool checkScroll(
        const Vec2f& mousePos,
        float delta,
        bool precise = false,
        bool momentum = false
    ) override;
    void onDeactivate() override;

    void handleTextInput(char32_t unicode) override;
    void handleKeyInput(SDL_Keycode key, bool shift, bool ctrl, bool gui) override;
    bool wantsTextInput() const override { return true; }

    // Scrolling a terminal moves the view back through scrollback, so a flick
    // should coast exactly as it does over a textbox.
    bool wantsScrollMomentum() const override { return true; }

    // Starts the shell if it is not already running. Reports false and leaves
    // an explanation in lastError() when it could not.
    bool start();
    // Ends the shell. The widget stays usable and can be start()ed again.
    void stop();
    bool isRunning() const;
    const std::string& lastError() const { return m_pty.lastError(); }

    // Feeds input to the shell as though it had been typed.
    void send(const std::string& text);
    // The live screen -- what the shell believes it has drawn -- rows joined by
    // newlines and trailing blanks trimmed. This ignores the scroll position:
    // scrolling back is a view of history and does not move the screen.
    std::string screenText() const;

    // What is actually visible right now, which is the live screen only while
    // scrolled to the bottom and history above that.
    std::string viewText() const;

    // How far the view sits above the live screen, in pixels. Zero means the
    // bottom, which is where new output appears.
    float scrollOffset()   const { return m_scrollPx; }
    bool  isScrolledBack() const { return m_scrollPx > 0.f; }
    // Jumps back down to the live screen.
    void  scrollToBottom();

    // The dragged-out selection, empty when there is none. Rows are joined by
    // newlines and trailing blanks are trimmed, so a block selected out of
    // padded cells pastes as the text that was visible rather than as a
    // rectangle of spaces.
    std::string selectedText() const;
    bool        hasSelection() const { return m_hasSelection; }
    void        clearSelection();
    // Selects everything, scrollback included.
    void        selectAll();

    // The absolute line number of the top row of the live screen. Absolute
    // numbers count every line the terminal has ever produced, so they keep
    // meaning as the screen scrolls; lower numbers are further back in history.
    long topLine() const { return m_linesScrolled; }
    // Selects a range given in those numbers, for selecting something found
    // programmatically rather than dragged out by hand.
    void selectRange(long fromLine, int fromCol, long toLine, int toCol);

    // Puts the selection on the system clipboard. No selection is a no-op, so a
    // stray Cmd+C never wipes what is already there.
    void copySelection() const;
    // Sends the clipboard to the shell as typed input, wrapped in bracketed
    // paste markers when the running program has asked for them.
    void pasteClipboard();

    int columns() const { return m_cols; }
    int rows()    const { return m_rows; }

private:
    // ---- screen -------------------------------------------------------
    TerminalCell&       cell(int col, int row);
    const TerminalCell& cell(int col, int row) const;
    void  resizeGrid(int cols, int rows);
    void  clearRegion(int fromCol, int fromRow, int toCol, int toRow);
    void  scrollUp(int lines);
    void  scrollDown(int lines);
    void  newline();
    void  putChar(char32_t c);
    void  useAlternateScreen(bool on, bool saveRestore);

    // ---- parser -------------------------------------------------------
    void feed(const std::string& bytes);
    void handleControl(char c);
    void handleCsi(char final);
    void handleOsc();
    void applySgr();
    Color indexedColor(int n) const;

    // ---- input --------------------------------------------------------
    void sendKey(SDL_Keycode key, bool shift, bool ctrl);

    // ---- geometry -----------------------------------------------------
    Rectf contentArea() const;
    float gridTopY() const;
    void  remeasure(float dt);

    // ---- selection ----------------------------------------------------
    // Selection is anchored in absolute line numbers -- how many lines have
    // ever been produced, not where they currently sit on screen -- so it stays
    // on the same text while the view scrolls and while new output pushes the
    // screen upward.
    long  absoluteTopLine() const { return m_linesScrolled; }
    long  hitTestLine(const Vec2f& pos) const;
    int   hitTestColumn(const Vec2f& pos) const;
    // Reports the row's own width in `len`: a row captured before a resize
    // is only as wide as the grid was then, and reading it against the
    // current width would run off the end of it.
    const TerminalCell* lineAt(long absolute, int& len) const;
    void  normalizedSelection(long& fromLine, int& fromCol,
                              long& toLine,   int& toCol) const;

    TerminalOptions m_options;
    Pty             m_pty;

    std::vector<TerminalCell>              m_grid;      // cols * rows
    std::vector<TerminalCell>              m_altGrid;
    std::deque<std::vector<TerminalCell>>  m_scrollback;
    int  m_cols = 80;
    int  m_rows = 24;

    int  m_curCol = 0, m_curRow = 0;
    int  m_savedCol = 0, m_savedRow = 0;
    // Saved alongside the cursor by DECSET 1049, because that sequence saves
    // the graphic rendition too -- which is what stops a program's parting
    // colours leaking onto the main screen after it exits.
    TerminalCell m_savedPen;
    int  m_scrollTop = 0, m_scrollBot = 0;     // inclusive region
    bool m_altScreen   = false;
    bool m_cursorVisible = true;
    bool m_appCursorKeys = false;
    bool m_wrapPending   = false;
    // Set by CSI ?2004h. A program that asks for this wants to be told where a
    // paste starts and ends, so it can take the text literally instead of
    // acting on any newlines in it.
    bool m_bracketedPaste = false;

    // Total lines ever pushed off the top, which is what makes an absolute line
    // number meaningful.
    long m_linesScrolled = 0;

    // A drag across the window edge steps through a new grid size every few
    // pixels, and telling the child about each one makes a full-screen program
    // restart its layout over and over while it is still working on the last
    // change. The size has to hold still briefly before it is applied, so one
    // drag costs one reshape.
    int   m_pendingCols  = 0;
    int   m_pendingRows  = 0;
    float m_sizeSettled  = 0.f;

    TerminalCell m_pen;                        // current attributes

    // parser state
    enum class P { Ground, Esc, Csi, Osc } m_state = P::Ground;
    std::string m_csi;
    std::string m_osc;
    std::string m_utf8;                        // partial codepoint across reads

    // view
    bool  m_focused    = false;
    // Pixels scrolled back into history, not rows: a trackpad delta is a
    // fraction of a row, and rounding each one to a whole row would throw most
    // of them away and make the view lurch instead of glide.
    float m_scrollPx   = 0.f;

    // selection, in absolute line numbers (see absoluteTopLine)
    bool m_hasSelection = false;
    bool m_mouseDown    = false;
    long m_selAnchorLine = 0, m_selHeadLine = 0;
    int  m_selAnchorCol  = 0, m_selHeadCol  = 0;
    float m_blinkTimer = 0.f;
    bool  m_blinkOn    = true;
    bool  m_started    = false;
    bool  m_exitFired  = false;

    // metrics
    float m_cellW = 0.f, m_cellH = 0.f;
    float m_lastScale = 0.f;
    unsigned int m_lastCharSize = 0;
    uint32_t m_fontId = 0xFFFFFFFFu;
    bool  m_fontLoaded = false;

    std::string m_pending;                     // bytes read, not yet parsed
};

}
