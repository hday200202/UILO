#include <UILO.hpp>
using namespace uilo;

Page* buildMainPage();

int main(int argc, char** argv) {
    Renderer renderer;
    renderer.init(1280, 720, "Terminal", 16U);

    UILO uilo(renderer, buildMainPage());
    uilo.setScale(OS::scale());

    /* Added to the default palette rather than replacing it, so the built-in
       widgets keep the roles Defaults.hpp gives them. */
    uilo.getTheme().palette().set("termBg", Color::fromHex("#2f2d30"));

    /*
        A frame, so it can be drawn from the loop below and from the live-resize
        callback alike.
    */
    auto frame = [&] {
        uilo.update();

        renderer.beginFrame();
        renderer.clear();
        uilo.render();
        renderer.endFrame();
    };

    /*
        macOS runs its own event loop while a window edge is being dragged, which
        leaves this loop stopped for the whole gesture. A terminal cannot afford
        that: the shell keeps writing, fills the pipe, and then blocks, so a
        full-screen program like btop stalls mid-resize and has to catch up
        afterwards. Drawing a frame from here keeps it being read.
    */
    uilo.setOnLiveResize(frame);
    Keybinds& kb = uilo.getKeybinds();
    kb.bindAction("zoom_in", {SDL_SCANCODE_LCTRL, SDL_SCANCODE_EQUALS}, [&](){ uilo.setScale(uilo.getScale() + 0.25f); }, true);

    while (uilo.isRunning()) {
        uilo.pollEvents();
        frame();
    }

    return 0;
}

Page* buildMainPage() {
    return page(
        column(Modifier(), ColumnOptions().setOuterPadding(0.f).setInnerPadding(0.f), contains {
            terminal(
                Modifier(),
                TerminalOptions()
                    .setCharSize(16)
                    .setFont("assets/fonts/AdwaitaMonoNerdFont.ttf")
                    .setScrollSpeed(50.f)
                    .setScrollback(5000)
                    .setBackgroundColorRole("termBg")
            )
        }),
        "main_page"
    );
}