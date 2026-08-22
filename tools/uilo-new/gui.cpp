/*
    gui.cpp:
    - Desc: The window front-end. It is a form over the same scaffold::create
            the command line calls, so the two cannot produce different projects.
            Built with UILO, which also makes it the first thing to check when a
            layout change lands.
*/

#include "scaffold.hpp"

#include <UILO.hpp>
#include <renderer/Renderer.hpp>
#include <utils/FileDialog.hpp>

#include <cstdio>
#include <string>

#define ROUNDING 8.f

using namespace uilo;

namespace {

Palette generatorPalette() {
    Palette p;
    p.set("app.bg",   { 24,  26,  35, 255});
    p.set("panel",    { 38,  41,  54, 255});
    p.set("panelAlt", { 52,  56,  73, 255});
    p.set("field",    { 30,  33,  44, 255});
    p.set("text",     {206, 214, 238, 255});
    p.set("textDim",  {132, 141, 172, 255});
    p.set("accent",   {126, 160, 240, 255});
    p.set("onAccent", {255, 255, 255, 255});
    p.set("outline",  { 62,  67,  84, 255});
    p.set("ok",       {126, 208, 150, 255});
    p.set("bad",      {226, 122, 122, 255});
    return p;
}

Text* caption(const std::string& content) {
    return text(
        Modifier()
            .setHeight(22_px),
        TextOptions()
            .setContent(content)
            .setCharSize(13)
            .setColorRole("textDim")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
    );
}

Textbox* field(const std::string& placeholder, const std::string& initial) {
    Textbox* box = textbox(
        Modifier()
            .setHeight(36_px),
        TextboxOptions()
            .setPlaceholder(placeholder)
            .setCharSize(15)
            .setBackgroundColorRole("field")
            .setTextColorRole("text")
            .setOutlineColorRole("outline")
            .setOutlineThickness(1.f)
            .setRounding(ROUNDING)
    );
    if (!initial.empty()) box->setString(initial);
    return box;
}

} // namespace


/*
    runGui():
    - Params:   none
    - Returns:  int -- the process exit code
    - Desc:     Opens the generator window and runs until it is closed. Every
                field is read at the moment Create is pressed rather than tracked
                as it changes, so there is no state to keep in step.
*/
int runGui() {
    Renderer renderer;
    if (!renderer.init(720, 560, "New UILO Project", 16)) {
        std::fprintf(stderr, "uilo-new: failed to initialise the renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setPalette(generatorPalette());
    ui.setScale(OS::scale());

    Textbox*  pathBox = field("~/code/myapp", "");
    Textbox*  nameBox = field("defaults to the folder name", "");
    Textbox*  refBox  = field("main", "main");
    Dropdown* tmplBox = dropdown(
        Modifier()
            .setHeight(36_px),
        DropdownOptions()
            .setPlaceholder("minimal")
            .setItemRounding(ROUNDING)
            .setPopupRounding(ROUNDING)
            .setHeaderRounding(ROUNDING),
        {"minimal", "panels"},
        "template"
    );

    Text* status = text(
        Modifier()
            .setHeight(1_flex),
        TextOptions()
            .setContent("Give it a path. The folder is created with bin/, ext/ and src/,\n"
                        "a CMakeLists.txt and a build.sh that fetches everything else.")
            .setCharSize(14)
            .setColorRole("textDim")
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::Top)
    );

    /* Pick the PARENT directory: the project folder itself does not exist yet,
       so there is nothing for a folder picker to select. The chosen directory
       plus the name field is the path that gets created. */
    auto onBrowse = [&](Element*) {
        if (!fileDialogsAvailable()) {
            status->getOptions().setColorRole("bad");
            status->setString("No file dialog available on this system.\n"
                              "On Linux that means installing zenity or kdialog.");
            return;
        }

        FileDialogOptions dialog;
        dialog.title = "Where should the project go?";

        /* Start wherever the box already points, so a second Browse does not
           send the user back to the beginning. */
        const std::filesystem::path typed = pathBox->getString();
        if (!typed.empty()) {
            std::error_code ec;
            dialog.startPath = std::filesystem::is_directory(typed, ec)
                             ? typed : typed.parent_path();
        }

        const std::optional<std::filesystem::path> parent = selectFolderDialog(dialog);
        if (!parent) return;

        /* Keep whatever the user had typed as the folder name, so browsing
           changes only where it lands. */
        std::string leaf = nameBox->getString();
        if (leaf.empty() && !typed.empty()) leaf = typed.filename().string();
        if (leaf.empty()) leaf = "myapp";

        pathBox->setString((*parent / leaf).string());
        status->getOptions().setColorRole("textDim");
        status->setString("Path set. Press Create project when you are ready.");
    };

    auto onCreate = [&](Element*) {
        uilonew::Options options;
        options.path = pathBox->getString();
        options.name = nameBox->getString();
        options.uiloRef = refBox->getString().empty() ? "main" : refBox->getString();

        const std::string chosen = tmplBox->getSelectedItem();
        options.tmpl = uilonew::templateFromName(chosen.empty() ? "minimal" : chosen);

        if (options.path.empty()) {
            status->getOptions().setColorRole("bad");
            status->setString("Give it a path first.");
            return;
        }

        const uilonew::Result result = uilonew::create(options);
        if (!result.ok) {
            status->getOptions().setColorRole("bad");
            status->setString(result.error);
            return;
        }

        std::string report = "Created " + result.root.string() + "\n\n";
        for (const std::string& file : result.written) report += "  " + file + "\n";
        report += "\n  cd " + result.root.string() + "\n  ./build.sh run\n\n"
                  "The first build fetches UILO and its dependencies.";
        status->getOptions().setColorRole("ok");
        status->setString(report);
    };

    ui.addPage(page(
        column(
            Modifier(),
            ColumnOptions()
                .setColorRole("app.bg")
                .setInnerPadding(24.f),
            contains{
                text(
                    Modifier()
                        .setHeight(38_px),
                    TextOptions()
                        .setContent("New UILO project")
                        .setCharSize(26)
                        .setColorRole("text")
                        .setTextAlignX(Align::Left)
                        .setTextAlignY(Align::CenterY)
                ),

                caption("Path"),
                row(
                    Modifier()
                        .setHeight(36_px),
                    RowOptions(),
                    contains{
                        pathBox,
                        spacer(Modifier().setWidth(8_px)),
                        button(
                            Modifier()
                                .setWidth(110_px)
                                .setOnLeftClick(onBrowse),
                            ButtonOptions()
                                .setColorRole("panelAlt")
                                .setRounding(6.f)
                                .setLabel(text(
                                    Modifier(),
                                    TextOptions()
                                        .setContent("Browse...")
                                        .setCharSize(14)
                                        .setColorRole("text")
                                        .setTextAlignX(Align::CenterX)
                                        .setTextAlignY(Align::CenterY)
                                ))
                        ),
                    }
                ),
                spacer(Modifier().setHeight(12_px)),

                caption("Name (optional)"),
                nameBox,
                spacer(Modifier().setHeight(12_px)),

                row(
                    Modifier()
                        .setHeight(58_px),
                    RowOptions(),
                    contains{
                        column(
                            Modifier().setWidth(1_flex),
                            ColumnOptions(),
                            contains{ caption("Template"), tmplBox }
                        ),
                        spacer(Modifier().setWidth(16_px)),
                        column(
                            Modifier().setWidth(1_flex),
                            ColumnOptions(),
                            contains{ caption("UILO ref"), refBox }
                        ),
                    }
                ),

                spacer(Modifier().setHeight(16_px)),

                button(
                    Modifier()
                        .setHeight(46_px)
                        .setOnLeftClick(onCreate),
                    ButtonOptions()
                        .setColorRole("accent")
                        .setRounding(8.f)
                        .setLabel(text(
                            Modifier(),
                            TextOptions()
                                .setContent("Create project")
                                .setCharSize(16)
                                .setColorRole("onAccent")
                                .setTextAlignX(Align::CenterX)
                                .setTextAlignY(Align::CenterY)
                        ))
                ),

                spacer(Modifier().setHeight(16_px)),

                column(
                    Modifier()
                        .setHeight(1_flex),
                    ColumnOptions()
                        .setColorRole("panel")
                        .setRounding(8.f)
                        .setInnerPadding(14.f),
                    contains{ status }
                ),
            },
            "root"
        ),
        "main"
    ));
    ui.setPage("main");

    while (ui.isRunning()) {
        ui.pollEvents();
        ui.update();

        renderer.beginFrame();
        renderer.clear(ui.getPalette().get("app.bg"));
        ui.render();
        renderer.endFrame();
    }
    return 0;
}
