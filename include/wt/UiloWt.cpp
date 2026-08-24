#ifdef UILO_WT

#include "UiloWt.hpp"
#include "Translator.hpp"
#include "../UILO.hpp"

#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WEnvironment.h>
#include <Wt/WServer.h>
#include <Wt/WTimer.h>

#include <memory>
#include <string>
#include <vector>

#include "../renderer/Renderer.hpp"

namespace uilo {
namespace wt {
namespace {

/*
    UiloApplication:
    - Desc: One browser connection. Renders a UILO into this connection's Wt
            widgets and keeps them in step through a Translator.

            Which UILO depends on how the server was started. The shared form
            takes one by reference and every connection reflects it. The
            per-session form hands over a WebApp this application then owns, so
            the UILO -- and the application state behind it -- is this
            connection's alone and dies with it.
*/
class UiloApplication : public Wt::WApplication {
public:
    UiloApplication(const Wt::WEnvironment& env, UILO& ui, const WebConfig& config)
        : Wt::WApplication(env), m_ui(ui) {
        init(config);
    }

    UiloApplication(const Wt::WEnvironment& env, std::unique_ptr<WebApp> owned,
                    const WebConfig& config)
        : Wt::WApplication(env), m_owned(std::move(owned)), m_ui(m_owned->ui()) {
        init(config);
    }

    // Re-reads the UILO and pushes what changed. Every event runs this through
    // the translator; the timer helper calls it directly.
    void sync() { m_translator->sync(); }

private:
    void init(const WebConfig& config) {
        setTitle(Wt::WString::fromUTF8(config.title));
        addBaseRules();

        m_host = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        m_host->setStyleClass("uilo-root");

        m_translator = std::make_unique<detail::Translator>(*this, m_host, m_ui, config);

        // First render: reconcilePages() translates and shows the active page.
        m_translator->sync();
    }

    void addBaseRules() {
        auto& sheet = styleSheet();
        sheet.addRule("html, body",
                      "margin:0;padding:0;width:100%;height:100%;overflow:hidden;");
        sheet.addRule(".uilo-root", "position:absolute;inset:0;overflow:hidden;");
        // Every page fills the same box; only the active one is un-hidden.
        sheet.addRule(".uilo-page", "position:absolute;inset:0;overflow:hidden;");
        // A floating layer (a popup and its backdrop) sits above the page.
        sheet.addRule(".uilo-overlay",
                      "position:absolute;inset:0;overflow:hidden;z-index:1000;");
        // Flex items default to a content-based minimum, which would stop
        // children shrinking to the size UILO gives them; UILO has no such floor.
        sheet.addRule(".uilo-el", "box-sizing:border-box;min-width:0;min-height:0;");
        sheet.addRule(".uilo-resizer-bar", "opacity:0;transition:opacity .15s;");
        sheet.addRule(".uilo-resizer:hover .uilo-resizer-bar", "opacity:1;");
    }

    // m_owned is declared first on purpose: in the per-session form m_ui is a
    // reference into it, and members are initialised in declaration order.
    std::unique_ptr<WebApp>             m_owned;   // null in the shared form
    UILO&                               m_ui;
    Wt::WContainerWidget*               m_host = nullptr;
    std::unique_ptr<detail::Translator> m_translator;
};

} // namespace


Wt::WApplication& application() {
    return *Wt::WApplication::instance();
}

void every(std::chrono::milliseconds interval, std::function<void()> fn) {
    auto* app = dynamic_cast<UiloApplication*>(Wt::WApplication::instance());
    if (!app) return;
    auto* timer = app->addChild(std::make_unique<Wt::WTimer>());
    timer->setInterval(interval);
    timer->timeout().connect([app, fn = std::move(fn)] { fn(); app->sync(); });
    timer->start();
}

} // namespace wt


namespace wt {
namespace {

/*
    serverArgs(const WebConfig& config):
    - Params:   const WebConfig& config
    - Returns:  std::vector<std::string>
    - Desc:     Wt is configured from a command line; synthesize one from the
                config so the caller passes nothing. Returned as strings --
                WRun wants char*, which the callers point at these.
*/
std::vector<std::string> serverArgs(const WebConfig& config) {
    std::vector<std::string> args = {
        "uilo",
        "--docroot",      config.docRoot,
        "--http-address", config.address,
        "--http-port",    std::to_string(config.port),
    };
    if (!config.resourcesDir.empty()) {
        args.emplace_back("--resources-dir");
        args.push_back(config.resourcesDir);
    }
    return args;
}


/*
    argPointers(std::vector<std::string>& args):
    - Params:   std::vector<std::string>& args
    - Returns:  std::vector<char*>
    - Desc:     The char* view of serverArgs()'s strings that WRun takes.
*/
std::vector<char*> argPointers(std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (std::string& a : args) argv.push_back(a.data());
    return argv;
}

} // namespace
} // namespace wt


/*
    UILO::runWeb(const WebConfig& config):
    - Params:   const wt::WebConfig& config
    - Returns:  int
    - Desc:     Serves this UILO over the web and blocks until the server
                stops. Set the UILO up exactly as on desktop first
                (addPage/setPage/setTheme); this stands up Wt, points every
                browser connection at this instance, and returns the server's
                exit code. No render/update loop and no argv -- the server
                settings come from the config. Every connection shares this
                instance, and Wt drives connections from different threads:
                two browsers are two views of one UI, with no lock between
                them. Use the factory overload for anything multi-user.
*/
int UILO::runWeb(const wt::WebConfig& config) {
    // The web build never draws, but UILO still expects a renderer to exist.
    static Renderer headless;
    setRenderer(headless);

    std::vector<std::string> args = wt::serverArgs(config);
    std::vector<char*>       argv = wt::argPointers(args);

    UILO& ui = *this;
    return Wt::WRun(static_cast<int>(argv.size()), argv.data(),
        [&ui, config](const Wt::WEnvironment& env) {
            return std::make_unique<wt::UiloApplication>(env, ui, config);
        });
}


/*
    UILO::runWeb(factory, const WebConfig& config):
    - Params:   std::function<std::unique_ptr<wt::WebApp>()> factory, const wt::WebConfig& config
    - Returns:  int
    - Desc:     The per-session counterpart: serves an app whose UILO is built
                fresh for each browser connection, and blocks until the server
                stops. `factory` runs once per connection, on that
                connection's own thread, and builds that session's UILO the
                same way a desktop app builds its one (addPage/setPage/
                setTheme). The WebApp it returns is owned by the connection and
                destroyed with it, so sessions share no pages, no palette and
                no application state -- and cannot race each other's widget
                trees. Anything the factory itself touches is still shared, so
                whatever it reads (a global theme, a file on disk) has to be
                safe to read from several threads at once.
*/
int UILO::runWeb(std::function<std::unique_ptr<wt::WebApp>()> factory,
                 const wt::WebConfig& config) {
    // One headless renderer for every session: the web build never draws, and
    // UILO only requires that a renderer exist.
    static Renderer headless;

    std::vector<std::string> args = wt::serverArgs(config);
    std::vector<char*>       argv = wt::argPointers(args);

    return Wt::WRun(static_cast<int>(argv.size()), argv.data(),
        [factory, config](const Wt::WEnvironment& env) {
            std::unique_ptr<wt::WebApp> app = factory();
            app->ui().setRenderer(headless);
            return std::make_unique<wt::UiloApplication>(env, std::move(app), config);
        });
}

} // namespace uilo

#endif   /* UILO_WT */
