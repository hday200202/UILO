#include <UILO.hpp>
using namespace uilo;

Page* buildMainPage();

int main(int argc, char** argv) {
    Renderer renderer;
    renderer.init(1280, 720, "Terminal", 16U);

    UILO uilo(renderer, buildMainPage());
    uilo.setScale(OS::scale());

    Palette palette;
    palette.set("termBg", Color::fromHex("#2f2d30"));
    uilo.setPalette(palette);
    Theme::current().setOuterPadding(0.f);

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

    while (uilo.isRunning()) {
        uilo.pollEvents();
        frame();
    }

    return 0;
}

Page* buildMainPage() {
    return page(
        column(Modifier(), ColumnOptions().setOuterPadding(0.f), contains {
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