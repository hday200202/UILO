// DateField: a date input in one element. The field shows the current value and
// opens a calendar popup when clicked; picking a date writes it back.
//
// It adapts to the size it is given -- give it room for the text and it shows
// icon, value and chevron; make it squarer and it drops to just the icon. That
// is the whole difference between the fields below.
//
// The bare DatePicker is still there underneath (getPicker()) for anything that
// wants the calendar without the field.

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

static Palette makeDarkPalette() {
    Palette p;
    p.set("app.bg",     { 33,  35,  47, 255});
    p.set("panel",      { 44,  47,  60, 255});
    p.set("panelAlt",   { 55,  58,  74, 255});
    p.set("text",       {180, 190, 220, 255});
    p.set("textMuted",  {130, 138, 170, 255});
    p.set("accent",     {151, 120, 206, 255});
    p.set("onAccent",   {255, 255, 255, 255});
    return p;
}

static Element* fieldCaption(const std::string& s) {
    return text(
        Modifier()
            .setHeight(26_px),
        TextOptions()
            .setContent(s)
            .setCharSize(14)
            .setColorRole("textMuted")
    );
}

static Element* gap(Dimension height) {
    return spacer(
        Modifier()
            .setHeight(height)
    );
}

Container* buildRoot();

int main() {
    Renderer renderer;
    if (!renderer.init(1280, 720, "DateField", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setScale(OS::scale());
    ui.getTheme().setPalette(makeDarkPalette());
    ui.addPage(page(buildRoot(), "main_page"));
    ui.setPage("main_page");

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

Container* buildRoot() {
    // The whole wiring for a date input: no UILO reference, no popup to keep
    // hold of, no label to write back into.
    auto* plain = datefield(
        Modifier()
            .setWidth(260_px)
            .setHeight(48_px),
        DateFieldOptions()
            .setOnDateChanged([](const Date& d) {
                std::printf("picked %s\n", DateAndTime::toISO(d).c_str());
            })
    );

    // Range mode is a picker option, and the field shows both ends once the
    // span closes.
    auto* range = datefield(
        Modifier()
            .setWidth(260_px)
            .setHeight(48_px),
        DateFieldOptions()
            .setPlaceholder("Pick a range")
            .setPickerOptions(
                DatePickerOptions()
                    .setMode(DatePickerMode::Range)
                    .setFirstDayOfWeek(Weekday::Monday)
            )
            .setOnRangeChanged([](const Date& a, const Date& b) {
                std::printf("range %s .. %s (%d days)\n",
                            DateAndTime::toISO(a).c_str(),
                            DateAndTime::toISO(b).c_str(),
                            DateAndTime::daysBetween(a, b) + 1);
            })
    );

    // Square, so the same widget comes out as an icon button. Nothing here says
    // "icon only" -- it falls out of the aspect ratio.
    auto* compact = datefield(
        Modifier()
            .setWidth(48_px)
            .setHeight(48_px),
        DateFieldOptions()
            .setInitialDate(DateAndTime::today())
            .setOnDateChanged([](const Date& d) {
                std::printf("compact picked %s\n", DateAndTime::toISO(d).c_str());
            })
    );

    // Starts with a value, spells the month out, and turns off year navigation
    // in its popup.
    auto* preset = datefield(
        Modifier()
            .setWidth(300_px)
            .setHeight(44_px),
        DateFieldOptions()
            .setFormat("dddd, MMMM D, YYYY")
            .setCharSize(15)
            .setInitialDate(DateAndTime::addDays(DateAndTime::today(), 14))
            .setLabelAspectThreshold(4.f)
            .setPickerOptions(
                DatePickerOptions()
                    .setShowYearNavigation(false)
            )
    );

    return column(
        Modifier(),
        ColumnOptions()
            .setColorRole("app.bg"),
        contains {
            // Explicit height, centred: a percent-height column here would share
            // the window with the spacers instead of taking what it needs.
            column(
                Modifier()
                    .setWidth(320_px)
                    .setHeight(400_px)
                    .setAlign(Align::CenterX | Align::CenterY),
                ColumnOptions(),
                contains {
                    text(
                        Modifier()
                            .setHeight(40_px),
                        TextOptions()
                            .setContent("uilo::DateField")
                            .setCharSize(22)
                            .setBold(true)
                            .setColorRole("text")
                    ),

                    gap(12_px),

                    fieldCaption("Default"),
                    plain,

                    gap(18_px),

                    fieldCaption("Range mode"),
                    range,

                    gap(18_px),

                    fieldCaption("Square: icon only, same widget"),
                    compact,

                    gap(18_px),

                    fieldCaption("Preset value, long format"),
                    preset
                }
            )
        },
        "root"
    );
}
