#include <UILO.hpp>
#include <iostream>

using namespace uilo;


/*
    Forward Declarations:
    - My prefered way of building UIs in UILO
*/
Palette     buildPalette();
Container*  buildTopBar();
Container*  buildFilebrowser(const std::filesystem::path& path);
Container*  buildMainContentContainer();
Container*  buildBottomBar();
Container*  buildMainContentContainerChildRow(const std::string& text);

Container*  panel(
    bool isColumn       = true, 
    Dimension width     = 100_pct, 
    Dimension height    = 100_pct, 
    contains children   = {}, 
    bool scrollable     = false
);

Page*       buildMainPage();


/*
    main():
    - Initialize your renderer          (width, height, window title)
    - Initialize your UILO instance     (renderer, main page)
    - Initialize and set your palette
    - Set the render scale to the OS render scale
*/
int main() {
    Renderer renderer;
    renderer.init(1280, 720, "Containers");

    UILO uilo(renderer, buildMainPage());

    uilo.setPalette(buildPalette());
    uilo.setScale(OS::scale());

    while (uilo.isRunning()) {
        // updates
        uilo.pollEvents();
        uilo.update();

        // rendering
        renderer.beginFrame();
        renderer.clear();

        uilo.render();

        renderer.endFrame();
    }

    return 0;
}


/*
    buildPalette():
    - Setting a few reusable color roles, and some global theme customizations
*/
Palette buildPalette() {
    Palette palette;

    palette.set("panel",            Color::fromHex("#3b3b3b"));
    palette.set("panelHover",       Color::fromHex("#525252"));
    palette.set("background",       Color::fromHex("#292828"));
    palette.set("infoPanel",        Color::fromHex("#525252"));
    palette.set("resizerHover",     Color::fromHex("#a0a0a0"));
    palette.set("navIcon",          Color::fromHex("#b5b4b4"));
    palette.set("navIconHovered",   Color::fromHex("#ffffff"));
    palette.set("accent",           Color::fromHex("#c485ff"));
    palette.set("transparent",      Color::Transparent);

    Theme::current().setRounding(10.f);

    return palette;
}


/*
    buildTopBar():
    - 64 pixel tall row for the top bar of the app
    - Brighter color on hover
*/
Container* buildTopBar() {
    return row(
        Modifier()
            .setHeight(64_px)
            .setOuterPadding(4.f)
            .setOnHoverEnter( [](Row* r) { 
                r->getOptions().setColorRole("panelHover"); 
            })
            .setOnHoverExit( [](Row* r) { 
                r->getOptions().setColorRole("panel"); 
            }),
        RowOptions()
            .setColorRole("panel"),
        contains{}
    );
}


/*
    buildFileBrowser():
    - Building a resizable filebrowser side bar from a root path
    - See buildMainPage for resizer setup
*/
Container* buildFilebrowser(const std::filesystem::path& path) {
    return filebrowser(
        Modifier()
            .setAlign(Align::Left | Align::Top)
            .setWidth(256_px)
            .setOuterPadding(4.f),
        FileBrowserOptions()
            .setRootPath(path.string())
            // .setSelectedColorRole("accent")
    );
}


/*
    buildMainContentContainer():
    - A scrollable column with a few children
*/
Container* buildMainContentContainer() {
    return column(
        Modifier()
            .setOuterPadding(4.f),
        ColumnOptions()
            .setColorRole("panel")
            .setScrollable(true),
    contains {
        textbox(
            Modifier().setHeight(100_pct),
            TextboxOptions()
                .setCharSize(16)
                .setMultiline(true)
                .setShowLineNumbers(true)
                .setTextAlignX(Align::Left)
                .setTextAlignY(Align::Top)
                .setFont("assets/fonts/AdwaitaMonoNerdFont.ttf")
                .setCurrentLineNumberColorRole("accent")
                .setLineNumberAlign(Align::Left)
                .setCursorWidth(1.f)
                .setLineNumberBold(true)
        )
    });
}


/*
    buildBottomBar():
    - A resizeable bottom bar
*/
Container* buildBottomBar() {
    return row(
        Modifier()
            .setOuterPadding(4.f)
            .setHeight(128_px),
        RowOptions()
            .setColorRole("panel")
    );
}


/*
    buildMainContentContainerChildRow():
    - Example for child elements of the MainContentContainer
*/
Container* buildMainContentContainerChildRow(const std::string& text) {
    return row(
        Modifier()
            .setOuterPadding(4.f)
            .setHeight(64_px),
        RowOptions()
            .setColorRole("infoPanel")
    );
}


Container* panel(
    bool isColumn, 
    Dimension width,
    Dimension height,
    contains children, 
    bool scrollable
) {
    if (isColumn) return column(
        Modifier()
            .setOuterPadding(4.f)
            .setWidth(width)
            .setHeight(height),
        ColumnOptions()
            .setColorRole("panel")
            .setScrollable(scrollable),
        children
    );

    return row(
        Modifier()
            .setOuterPadding(4.f)
            .setWidth(width)
            .setHeight(height),
        RowOptions()
            .setColorRole("panel")
            .setScrollable(scrollable),
        children
    );
}


/*
    buildMainPage()
    - Build a page using our custom element functions
    - root container (column)
        top bar
        row for filebrowser and main content column
            filebrowser
            resizer (looks left at filebrowser)
            main content column
        resizer (looks down at bottom bar)
        bottom bar
*/
Page* buildMainPage() {
    auto iconSpacer = [](Align align = Align::Top){ 
        return spacer(Modifier().setHeight(12_px).setAlign(align));
    };

    auto navIcon = [](const std::string_view& iconStr, Align align = Align::Top | Align::CenterX){
        return icon(
            Modifier()
                .setWidth(32_px)
                .setHeight(32_px)
                .setAlign(align)
                .setCursor(CursorType::Hand)
                .setOnHoverEnter([](Icon* i){ 
                    i->getOptions().setColorRole("accent"); 
                })
                .setOnHoverExit([](Icon* i){ 
                    i->getOptions().setColorRole("navIcon"); 
                }),
            IconOptions()
                .setIcon(iconStr)
                .setStrokeWidth(2.f)
                .setColorRole("navIcon")
        );
    };

    return page(
        column(
            Modifier(),
            ColumnOptions()
                .setColorRole("background"),
        contains{
            row(
                Modifier(),
                RowOptions()
                    .setColorRole("background"),
            contains{
                panel(true, 48_px, 100_pct, {
                    iconSpacer(),
                    navIcon(Resources::icons::file),

                    iconSpacer(),
                    navIcon(Resources::icons::search),

                    iconSpacer(),
                    navIcon(Resources::icons::git_pull_request),

                    iconSpacer(),
                    navIcon(Resources::icons::play),

                    iconSpacer(),
                    navIcon(Resources::icons::database),

                    iconSpacer(),
                    navIcon(Resources::icons::terminal),

                    navIcon(Resources::icons::user, Align::Bottom | Align::CenterX),
                    iconSpacer(Align::Bottom),

                    navIcon(Resources::icons::settings, Align::Bottom | Align::CenterX),
                    iconSpacer(Align::Bottom),
                }),

                buildFilebrowser("../"),

                resizer(
                    Modifier()
                        .setWidth(24_px)
                        .setOnUpdateEnd([alphaF = 0.f](Resizer* r) mutable {
                            constexpr Color target = {255, 255, 255, 100};
                            constexpr float fadeSec = 0.18f;
                            const bool active = r->isHovered() || r->isDragging();
                            const float step = (r->getDeltaTime() / fadeSec) * (float)target.a;
                            alphaF += active ? +step : -step;
                            if (alphaF < 0.f) alphaF = 0.f;
                            if (alphaF > (float)target.a) alphaF = (float)target.a;
                            Color c = r->getOptions().getColor();
                            c.r = target.r; c.g = target.g; c.b = target.b;
                            c.a = (uint8_t)(alphaF + 0.5f);
                            r->getOptions().setColor(c);
                        }), 
                    ResizerOptions()
                        .setDirection(ResizerDir::Left)
                        .setResizeWidthMin(15_pct)
                        .setResizeWidthMax(50_pct)
                ),
                column(Modifier(), ColumnOptions(), contains{
                    buildMainContentContainer(),
                    resizer(
                        Modifier()
                            .setHeight(24_px)
                            .setOnUpdateEnd([alphaF = 0.f](Resizer* r) mutable {
                                constexpr Color target = {255, 255, 255, 100};
                                constexpr float fadeSec = 0.18f;
                                const bool active = r->isHovered() || r->isDragging();
                                const float step = (r->getDeltaTime() / fadeSec) * (float)target.a;
                                alphaF += active ? +step : -step;
                                if (alphaF < 0.f) alphaF = 0.f;
                                if (alphaF > (float)target.a) alphaF = (float)target.a;
                                Color c = r->getOptions().getColor();
                                c.r = target.r; c.g = target.g; c.b = target.b;
                                c.a = (uint8_t)(alphaF + 0.5f);
                                r->getOptions().setColor(c);
                            }),
                        ResizerOptions()
                            .setDirection(ResizerDir::Bottom)
                            // .setResizeHeightMin(15_pct)
                            .setResizeHeightMax(50_pct)
                    ),

                    buildBottomBar()
                })
            }),
        }), "main_page"
    );
}