// DatePicker as a date field: a button that opens a calendar popup and shows
// whatever you pick -- the same shape as a dropdown, a header that reflects the
// current value and a panel that drops out of it.
//
// The picker is not part of the page tree. open() gives it its own full-window
// backdrop and centres it, so it can't also be a child somewhere else -- keep
// the pointer and trigger it from the field.

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace uilo;

float ROUNDING = 8.f;

static Palette makeDarkPalette() {
    Palette p;
    p.set("app.bg",     { 33,  35,  47, 255});
    p.set("panel",      { 44,  47,  60, 255});
    p.set("panelAlt",   { 55,  58,  74, 255});
    p.set("field.hover",{ 66,  70,  88, 255});
    p.set("text",       {180, 190, 220, 255});
    p.set("textMuted",  {130, 138, 170, 255});
    p.set("accent",     {151, 120, 206, 255});
    p.set("onAccent",   {255, 255, 255, 255});
    return p;
}

Container* buildDateField(UILO& ui);

int main() {
    Renderer renderer;
    if (!renderer.init(1280, 720, "DatePicker", 16)) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    UILO ui;
    ui.setRenderer(renderer);
    ui.setScale(OS::scale());
    ui.setPalette(makeDarkPalette());
    ui.addPage(page(buildDateField(ui), "main_page"));
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

Container* buildDateField(UILO& ui) {
    // The field's label. Held so the picker's callback can write the chosen
    // date back into it -- this is what makes the field read like a dropdown.
    Text* label = text(
        Modifier().setAlign(Align::Left | Align::CenterY),
        TextOptions()
            .setContent("Pick a date")
            .setColorRole("text")
            .setCharSize(16)
            .setTextAlignY(Align::CenterY));

    // The popup. closeOnSelect makes one click settle it, exactly like choosing
    // a dropdown item; the callback fills the label in.
    DatePicker* picker = datepicker(
        Modifier(),
        DatePickerOptions()
            .setCloseOnSelect(true)
            .setShowYearNavigation(true)
            .setOnDateSelected([label](const Date& d) {
                label->setString(DateAndTime::format(d, "MMM D, YYYY"));
            }));

    return column(
        Modifier(),
        ColumnOptions().setColorRole("app.bg"),
        contains{
            column(
                Modifier().setWidth(280_px).setAlign(Align::CenterX | Align::CenterY),
                ColumnOptions(),
                contains{
                    text(Modifier().setHeight(28_px),
                         TextOptions().setContent("Date").setCharSize(14)
                                      .setColorRole("textMuted")),

                    // The field itself: a calendar icon, the value, and a
                    // chevron pushed to the right by the percent spacer. The
                    // whole row is the click target, so opening the popup is one
                    // handler on the row -- no inner button to fight with.
                    row(
                        Modifier()
                            .setHeight(48_px)
                            .setOnLeftClick([picker, &ui](Element*) { picker->open(ui); })
                            .setOnHoverEnter([](Row* r) { r->getOptions().setColorRole("field.hover"); })
                            .setOnHoverExit ([](Row* r) { r->getOptions().setColorRole("panelAlt"); }),
                        RowOptions().setColorRole("panelAlt").setRounding(ROUNDING),
                        contains{
                            spacer(Modifier().setWidth(14_px)),
                            icon(Modifier().setWidth(20_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::calendar)
                                              .setColorRole("textMuted")),
                            spacer(Modifier().setWidth(12_px)),
                            label,
                            spacer(Modifier().setWidth(100_pct)),
                            icon(Modifier().setWidth(20_px).setAlign(Align::CenterY),
                                 IconOptions().setIcon(Resources::icons::chevron_down)
                                              .setColorRole("textMuted")),
                            spacer(Modifier().setWidth(14_px))
                        })
                })
        }, "root");
}
