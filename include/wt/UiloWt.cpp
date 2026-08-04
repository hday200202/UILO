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
    - Desc: One browser connection. Renders the shared UILO into this
            connection's Wt widgets and keeps them in step through a Translator.
            There is no per-application UILO or builder any more -- every
            connection reflects the one UILO that runWeb() was called on.
*/
class UiloApplication : public Wt::WApplication {
public:
    UiloApplication(const Wt::WEnvironment& env, UILO& ui, const WebConfig& config)
        : Wt::WApplication(env), m_ui(ui) {
        setTitle(Wt::WString::fromUTF8(config.title));
        addBaseRules();

        m_host = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        m_host->setStyleClass("uilo-root");

        m_translator = std::make_unique<detail::Translator>(*this, m_host, m_ui, config);

        // First render: reconcilePages() translates and shows the active page.
        m_translator->sync();
    }

    // Re-reads the UILO and pushes what changed. Every event runs this through
    // the translator; the timer helper calls it directly.
    void sync() { m_translator->sync(); }

private:
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


/*
    UILO::runWeb(const WebConfig& config):
    - Serves this UILO over the web and blocks until the server stops. Set the
      UILO up exactly as on desktop first (addPage/setPage/setPalette); this
      stands up Wt, points every browser connection at this instance, and
      returns the server's exit code. No render/update loop and no argv -- the
      server settings come from the config.
*/
int UILO::runWeb(const wt::WebConfig& config) {
    // The web build never draws, but UILO still expects a renderer to exist.
    static Renderer headless;
    setRenderer(headless);

    // Wt is configured from a command line; synthesize one from the config so
    // the caller passes nothing.
    const std::string port = std::to_string(config.port);
    std::vector<std::string> args = {
        "uilo",
        "--docroot",      config.docRoot,
        "--http-address", config.address,
        "--http-port",    port,
    };
    if (!config.resourcesDir.empty()) {
        args.emplace_back("--resources-dir");
        args.push_back(config.resourcesDir);
    }
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (std::string& a : args) argv.push_back(a.data());

    UILO& ui = *this;
    return Wt::WRun(static_cast<int>(argv.size()), argv.data(),
        [&ui, config](const Wt::WEnvironment& env) {
            return std::make_unique<wt::UiloApplication>(env, ui, config);
        });
}

} // namespace uilo

#endif   /* UILO_WT */
