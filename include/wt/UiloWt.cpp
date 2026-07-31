#ifdef UILO_WT

#include "UiloWt.hpp"
#include "Translator.hpp"

#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WEnvironment.h>
#include <Wt/WTimer.h>

#include <algorithm>
#include <map>
#include <vector>

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
        : Wt::WApplication(env), m_translator(*this, m_ui, config) {
        setTitle(Wt::WString::fromUTF8(config.title));
        addBaseRules();

        // The renderer is the headless no-op from the UILO_WT build. UILO
        // needs one to exist, but the bridge never ticks or draws the tree, so
        // nothing is ever asked of it.
        m_ui.setRenderer(m_renderer);
        m_ui.setPalette(config.palette);

        m_host = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        m_host->setStyleClass("uilo-root");

        // Anything the builder registers with addPage() lands in m_pages; the
        // page it returns is simply the one to open on.
        Page* initial = build(*this);
        if (initial) addPage(initial);
        if (m_pages.empty()) return;

        // A deep link wins over the builder's choice, so a bookmarked page
        // opens where it was bookmarked.
        const std::string fromUrl = pageFromPath(internalPath());
        const std::string first   = initial ? initial->getName() : m_order.front();
        show(m_pages.count(fromUrl) ? fromUrl : first, false);

        internalPathChanged().connect([this](const std::string& path) {
            // Back/forward and typed URLs arrive here. Don't write the path
            // back out: the browser already moved it, and doing so would push
            // a duplicate history entry.
            const std::string name = pageFromPath(path);
            if (m_pages.count(name)) show(name, false);
        });
    }

    void addPage(Page* page) override {
        if (!page || m_pages.count(page->getName())) return;

        // Hands the page (and, through it, every element) to UILO, which owns
        // it for the rest of the session. Must happen before translating: it is
        // what binds elements to this UILO, and colour roles resolve through
        // that binding.
        m_ui.addPage(page);
        m_ui.setPage(page->getName());

        auto* host = m_host->addWidget(std::make_unique<Wt::WContainerWidget>());
        host->setStyleClass("uilo-page");
        host->setHidden(true);
        m_translator.build(*page, host);

        m_pages.emplace(page->getName(), host);
        m_order.push_back(page->getName());
    }

    void showPage(const std::string& name) override { show(name, true); }

    std::string currentPage() const override { return m_current; }

    void onPageChanged(std::function<void(const std::string&)> fn) override {
        m_onPageChanged.push_back(std::move(fn));
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
    // Pages are addressed as "/name". The leading slash is all that separates
    // the two forms, so the conversion is a trim in each direction.
    static std::string pageFromPath(const std::string& path) {
        std::string name = path;
        while (!name.empty() && name.front() == '/') name.erase(name.begin());
        while (!name.empty() && name.back()  == '/') name.pop_back();
        return name;
    }

    void show(const std::string& name, bool pushUrl) {
        auto it = m_pages.find(name);
        if (it == m_pages.end() || name == m_current) return;

        if (auto prev = m_pages.find(m_current); prev != m_pages.end())
            prev->second->setHidden(true);
        it->second->setHidden(false);
        m_current = name;

        // Keeps UILO's own notion of the active page in step, so getElement()
        // and anything else reading it agree with what is on screen.
        m_ui.setPage(name);

        if (pushUrl) setInternalPath("/" + name, false);
        for (const auto& fn : m_onPageChanged) fn(name);

        m_translator.sync();
    }

    void addBaseRules() {
        auto& sheet = styleSheet();
        sheet.addRule("html, body",
                      "margin:0;padding:0;width:100%;height:100%;overflow:hidden;");
        sheet.addRule(".uilo-root", "position:absolute;inset:0;overflow:hidden;");
        // Every page fills the same box; only one is ever un-hidden.
        sheet.addRule(".uilo-page", "position:absolute;inset:0;overflow:hidden;");
        // A floating layer (a popup and its backdrop) sits above the page,
        // filling the window the same way. Only shown while its element is in
        // UILO's floating list.
        sheet.addRule(".uilo-overlay", "position:absolute;inset:0;overflow:hidden;z-index:1000;");
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

    Wt::WContainerWidget*                             m_host = nullptr;
    std::map<std::string, Wt::WContainerWidget*>      m_pages;   // name -> host
    std::vector<std::string>                          m_order;   // registration order
    std::string                                       m_current;
    std::vector<std::function<void(const std::string&)>> m_onPageChanged;
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
