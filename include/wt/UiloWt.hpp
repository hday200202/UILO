#pragma once
#ifdef UILO_WT

/* UILO -> Wt bridge. */

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
    - Desc:     Per-application settings that have no UILO equivalent, because
                they describe the page rather than the UI inside it.
*/
struct Config {
    std::string title = "UILO";

    /* Palette every session starts with. Elements resolve their colour roles
       through it exactly as they would natively. */
    Palette palette = Palette::defaultDark();

    /* Font stack for every Text. UILO loads a .ttf by path; the browser needs
       a CSS font family, and there is no way to derive one from the other. */
    std::string fontFamily =
        "system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif";

    /* UILO's `charSize` is a pixel height spanning ascent+descent, whereas CSS
       `font-size` sizes the em square. */
    float charSizeToFontSize = 0.86f;
};

/*
    Session:
    - Desc:     One browser tab. Wt builds an application instance per session,
                so each gets its own UILO instance and its own element tree --
                element pointers captured in your callbacks belong to that
                session alone and are never shared between users.
*/
class Session {
public:
    virtual ~Session() = default;

    /* The UILO instance owning this session's tree. Use it for the palette and
       for getElement<T>("name") lookups. */
    virtual UILO& ui() = 0;

    /* Registers another page and translates it, hidden. */
    virtual void addPage(Page* page) = 0;

    /* Shows a registered page by name, and puts it in the address bar, so the
       browser's back button and a pasted link both land where you expect. */
    virtual void showPage(const std::string& name) = 0;

    /* The page currently on screen. */
    virtual std::string currentPage() const = 0;

    /* Runs `fn` whenever the shown page changes, including navigation the user
       drove from the address bar or the back button. */
    virtual void onPageChanged(std::function<void(const std::string&)> fn) = 0;

    /* Re-read the UILO tree and push anything that changed to the browser. */
    virtual void sync() = 0;

    /* Run `fn` every `interval` for the life of the session, then sync. */
    virtual void every(std::chrono::milliseconds interval,
                       std::function<void()> fn) = 0;

    /* Swap the palette and restyle the whole tree. Every element that uses
       colour *roles* rather than literal colours changes with it. */
    virtual void setPalette(const Palette& palette) = 0;

    /* The underlying Wt application. You should not need this -- it is here so
       that a one-off need for something Wt-specific doesn't force a fork. */
    virtual Wt::WApplication& application() = 0;
};

/* Builds one session's UI. */
using Builder = std::function<Page*(Session&)>;

/*
    run(int argc, char** argv, Builder build, Config config):
    - Params:   int argc, char** argv, Builder build, Config config
    - Returns:  int -- the server's exit code
    - Desc:     Starts the web server and serves `build` at the document root.
                Blocks until the server stops, so it is a drop-in for the body
                of main(). Argv is Wt's own -- `--http-address`, `--http-port`,
                `--docroot`.
*/
int run(int argc, char** argv, Builder build, Config config = {});

} // namespace uilo::wt

#endif   /* UILO_WT */
