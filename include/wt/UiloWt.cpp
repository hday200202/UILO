#ifdef UILO_WT

#include "UiloWt.hpp"
#include "Translator.hpp"

#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WEnvironment.h>
#include <Wt/WTimer.h>

#include "../renderer/Renderer.hpp"

namespace uilo::wt {
namespace {

/*
    UiloApplication:
    - Desc: One browser session. Owns that session's UILO instance, the tree
            the builder produced, and the Wt widgets translated from it.
            Implements Session, so the builder never sees the Wt side.
*/
class UiloApplication : public Wt::WApplication, public Session {
public:
    UiloApplication(const Wt::WEnvironment& env, const Builder& build, const Config& config)
        : Wt::WApplication(env), m_translator(*this, config) {
        setTitle(Wt::WString::fromUTF8(config.title));
        addBaseRules();

        // The renderer is the headless no-op from the UILO_WT build. UILO
        // needs one to exist, but the bridge never ticks or draws the tree, so
        // nothing is ever asked of it.
        m_ui.setRenderer(m_renderer);
        m_ui.setPalette(config.palette);

        Page* page = build(*this);
        if (!page) return;

        // Hands the page (and, through it, every element) to UILO, which owns
        // them for the rest of the session. Must happen before translating:
        // it is what binds elements to this UILO, and colour roles resolve
        // through that binding.
        m_ui.addPage(page);
        m_ui.setPage(page->getName());

        auto* host = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        host->setStyleClass("uilo-root");
        m_translator.build(*page, host);
    }

    UILO& ui() override { return m_ui; }
    void  sync() override { m_translator.sync(); }
    Wt::WApplication& application() override { return *this; }

    void setPalette(const Palette& palette) override {
        m_ui.setPalette(palette);
        m_translator.sync();
    }

    void every(std::chrono::milliseconds interval, std::function<void()> fn) override {
        auto* timer = addChild(std::make_unique<Wt::WTimer>());
        timer->setInterval(interval);
        timer->timeout().connect([this, fn = std::move(fn)] {
            fn();
            m_translator.sync();
        });
        timer->start();
    }

private:
    void addBaseRules() {
        auto& sheet = styleSheet();
        sheet.addRule("html, body",
                      "margin:0;padding:0;width:100%;height:100%;overflow:hidden;");
        sheet.addRule(".uilo-root", "position:absolute;inset:0;overflow:hidden;");
        // A flex item defaults to a content-based minimum size, which would
        // stop children shrinking to the size UILO gives them. UILO has no
        // such floor, so it is removed here for every translated element.
        sheet.addRule(".uilo-el", "box-sizing:border-box;min-width:0;min-height:0;");
        // Resizer handles sit invisible until pointed at, the way the UILO
        // examples fade theirs in from a per-frame handler.
        sheet.addRule(".uilo-resizer-bar", "opacity:0;transition:opacity .15s;");
        sheet.addRule(".uilo-resizer:hover .uilo-resizer-bar", "opacity:1;");
    }

    Renderer           m_renderer;
    UILO               m_ui;
    detail::Translator m_translator;
};

} // namespace

int run(int argc, char** argv, Builder build, Config config) {
    return Wt::WRun(argc, argv,
        [build = std::move(build), config = std::move(config)](const Wt::WEnvironment& env) {
            return std::make_unique<UiloApplication>(env, build, config);
        });
}

} // namespace uilo::wt

#endif // UILO_WT
