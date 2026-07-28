#pragma once
#ifdef UILO_WT

// UILO -> Wt bridge.
//
// Write a UI with UILO's normal factories (column, row, text, button, ...)
// and serve it as a web app. Nothing in your UI code mentions Wt: the bridge
// walks the UILO element tree once per browser session and builds an
// equivalent tree of Wt widgets styled to match what UILO would have drawn
// natively.
//
//     #include "uilo_wt/UiloWt.hpp"
//     using namespace uilo;
//
//     static Page* build(wt::Session& s) {
//         Text* clock = text(Modifier().setHeight(40_px),
//                            TextOptions().setContent("--"));
//         s.every(std::chrono::seconds(1), [clock] {
//             clock->setString(currentTime());   // pushed to the browser
//         });
//         return page(column(Modifier(), ColumnOptions().setColorRole("app.bg"),
//                            { clock }), "main");
//     }
//
//     int main(int argc, char** argv) { return wt::run(argc, argv, build); }
//
// Reached by including <UILO.hpp> in a build configured with -DUILO_WT=ON,
// which compiles out bgfx and SDL and links Wt instead, so the server binary
// carries no GPU or windowing dependency.

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "../UILO.hpp"
#include "../Palette.hpp"
#include "../elements/Factory.hpp"

namespace Wt { class WApplication; }

namespace uilo::wt {

/*
    Config:
    - Desc: Per-application settings that have no UILO equivalent, because
            they describe the page rather than the UI inside it.
*/
struct Config {
    std::string title = "UILO";

    // Palette every session starts with. Elements resolve their colour roles
    // through it exactly as they would natively.
    Palette palette = Palette::defaultDark();

    // Font stack for every Text. UILO loads a .ttf by path; the browser needs
    // a CSS font family, and there is no way to derive one from the other.
    std::string fontFamily =
        "system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif";

    // UILO's `charSize` is a pixel height spanning ascent+descent, whereas CSS
    // `font-size` sizes the em square. For most fonts the former is ~1.16x the
    // latter, so charSize is scaled by this to keep glyphs the size UILO drew
    // them. Nudge it if your native and web builds disagree.
    float charSizeToFontSize = 0.86f;
};

/*
    Session:
    - Desc: One browser tab. Wt builds an application instance per session, so
            each gets its own UILO instance and its own element tree -- element
            pointers captured in your callbacks belong to that session alone
            and are never shared between users.
*/
class Session {
public:
    virtual ~Session() = default;

    // The UILO instance owning this session's tree. Use it for the palette
    // and for getElement<T>("name") lookups.
    virtual UILO& ui() = 0;

    // Registers another page and translates it, hidden. Call it from the
    // builder for every page beyond the one you return; the returned page is
    // simply the one shown first.
    //
    // Every page is translated up front and kept in the document, so switching
    // is a visibility change rather than a rebuild -- a page kept its scroll
    // position and its half-typed text while you were away, which is what a
    // desktop UILO app does when you setPage() back and forth.
    virtual void addPage(Page* page) = 0;

    // Shows a registered page by name, and puts it in the address bar, so the
    // browser's back button and a pasted link both land where you expect.
    virtual void showPage(const std::string& name) = 0;

    // The page currently on screen.
    virtual std::string currentPage() const = 0;

    // Runs `fn` whenever the shown page changes, including navigation the user
    // drove from the address bar or the back button.
    virtual void onPageChanged(std::function<void(const std::string&)> fn) = 0;

    // Re-read the UILO tree and push anything that changed to the browser.
    // Called automatically after every UI event (clicks, edits, slider drags)
    // and after every every() tick, so you only need it when you mutate the
    // tree from somewhere the bridge doesn't already know about.
    virtual void sync() = 0;

    // Run `fn` every `interval` for the life of the session, then sync. This
    // is the polling loop a dashboard wants: read your data source, push it
    // into the UILO tree with setString/setOptions, and the browser updates.
    virtual void every(std::chrono::milliseconds interval,
                       std::function<void()> fn) = 0;

    // Swap the palette and restyle the whole tree. Every element that uses
    // colour *roles* rather than literal colours changes with it.
    virtual void setPalette(const Palette& palette) = 0;

    // The underlying Wt application. You should not need this -- it is here so
    // that a one-off need for something Wt-specific doesn't force a fork.
    virtual Wt::WApplication& application() = 0;
};

// Builds one session's UI. Called once per browser session, on that session's
// thread. Return a Page built with uilo::page(...); the bridge takes ownership.
using Builder = std::function<Page*(Session&)>;

/*
    run():
    - Desc: Starts the web server and serves `build` at the document root.
            Blocks until the server stops, and returns its exit code, so it is
            a drop-in for the body of main().
    - Note: Argv is Wt's -- `--http-address`, `--http-port`, `--docroot`.
*/
int run(int argc, char** argv, Builder build, Config config = {});

} // namespace uilo::wt

#endif // UILO_WT
