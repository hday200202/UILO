// DatePicker: the two ways to put one on screen.
//
//   Left  -- embedded in the layout, like any other element.
//   Right -- a button that opens a centred popup over the whole window.
//
// The range-mode picker at the bottom left shows the second selection mode:
// click two days and the span between them fills in.

#include "../include/UILO.hpp"
#include "../include/renderer/Renderer.hpp"
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <cstdio>
#include <iostream>

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

// Set by the popup and the embedded picker so the label can show what was
// picked; kept as file scope to keep the example's callbacks short.
static Text* g_readout      = nullptr;
static Text* g_rangeReadout = nullptr;

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

    // ---- The popup picker ------------------------------------------------
    // Not part of the page tree: open() gives it its own backdrop.
    auto* popupPicker = datepicker(
        Modifier(),
        DatePickerOptions()
            .setCloseOnSelect(true)
            .setShowYearNavigation(true)
            .setOnDateSelected([](const Date& d) {
                std::cout << "popup picked " << DateAndTime::format(d, "dddd, MMMM D, YYYY") << std::endl;
                if (g_readout) g_readout->setString(DateAndTime::format(d, "MMM D, YYYY"));
            })
            .setOnCancelled([]() { std::cout << "popup cancelled" << std::endl; }));

    // ---- The range picker ------------------------------------------------
    // With fixed week rows off the grid is only as tall as the month needs, so
    // its content height changes as you page. Re-sizing on every month change
    // keeps the card tight around it.
    auto* rangePicker = datepicker(
        Modifier(),
        DatePickerOptions()
            .setMode(DatePickerMode::Range)
            .setFirstDayOfWeek(Weekday::Monday)
            .setWeekdayLabelStyle(WeekdayLabelStyle::Short)
            .setShowFooter(false)
            .setFixedWeekRows(false)
            .setOnRangeSelected([](const Date& a, const Date& b) {
                const std::string s = DateAndTime::format(a, "MMM D") + " - "
                                    + DateAndTime::format(b, "MMM D") + "  ("
                                    + std::to_string(DateAndTime::daysBetween(a, b) + 1) + " days)";
                std::cout << "range " << s << std::endl;
                if (g_rangeReadout) g_rangeReadout->setString(s);
            }));
    rangePicker->getOptions().setOnMonthChanged(
        [rangePicker](int, unsigned) { rangePicker->sizeToContent(); });
    rangePicker->sizeToContent();

    // ---- Page ------------------------------------------------------------
    g_readout      = text(Modifier().setHeight(24_px),
                          TextOptions().setContent("no date picked")
                                       .setCharSize(15).setColorRole("textMuted"));
    g_rangeReadout = text(Modifier().setHeight(24_px),
                          TextOptions().setContent("no range picked")
                                       .setCharSize(15).setColorRole("textMuted"));

    auto* openButtonLabel = text(Modifier().setWidth(100_pct).setHeight(100_pct),
        TextOptions().setContent("Pick a date...").setCharSize(15)
                     .setColorRole("onAccent")
                     .setTextAlignX(Align::CenterX).setTextAlignY(Align::CenterY));

    Container* root = column(
        Modifier(),
        ColumnOptions().setColorRole("app.bg"),
        contains{
            spacer(Modifier().setHeight(24_px)),
            row(Modifier().setHeight(40_px), RowOptions(), contains{
                spacer(Modifier().setWidth(24_px)),
                text(Modifier(), TextOptions().setContent("uilo::DatePicker")
                                              .setCharSize(22).setBold(true)
                                              .setColorRole("text"))
            }),
            spacer(Modifier().setHeight(16_px)),

            row(Modifier(), RowOptions(), contains{
                spacer(Modifier().setWidth(24_px)),

                // ---- Embedded, single date ----
                column(Modifier().setWidth(340_px), ColumnOptions(), contains{
                    text(Modifier().setHeight(26_px),
                         TextOptions().setContent("Embedded, single").setCharSize(14)
                                      .setColorRole("textMuted")),
                    // sizeToContent() pins the height to exactly what the
                    // metrics need, so no footer means no room kept for one.
                    datepicker(
                        Modifier(),
                        DatePickerOptions()
                            .setShowFooter(false)
                            .setOnDateSelected([](const Date& d) {
                                std::cout << "embedded picked " << DateAndTime::toISO(d) << std::endl;
                            })
                            .setOnMonthChanged([](int y, unsigned m) {
                                std::cout << "month -> " << DateAndTime::monthName(m) << " " << y << std::endl;
                            }))->sizeToContent(),
                    spacer(Modifier().setHeight(20_px)),

                    // ---- Embedded, range ----
                    text(Modifier().setHeight(26_px),
                         TextOptions().setContent("Embedded, range").setCharSize(14)
                                      .setColorRole("textMuted")),
                    rangePicker
                }),

                spacer(Modifier().setWidth(40_px)),

                // ---- Popup trigger ----
                column(Modifier().setWidth(320_px), ColumnOptions(), contains{
                    text(Modifier().setHeight(26_px),
                         TextOptions().setContent("Centred popup").setCharSize(14)
                                      .setColorRole("textMuted")),
                    button(
                        Modifier()
                            .setHeight(40_px)
                            .setOnLeftClick([popupPicker, &ui](Element*) { popupPicker->open(ui); }),
                        ButtonOptions().setColorRole("accent").setRounding(8.f)
                                       .setLabel(openButtonLabel)),
                    spacer(Modifier().setHeight(12_px)),
                    g_readout,
                    spacer(Modifier().setHeight(24_px)),
                    text(Modifier().setHeight(24_px),
                         TextOptions().setContent("Range readout:").setCharSize(14)
                                      .setColorRole("textMuted")),
                    g_rangeReadout,
                    spacer(Modifier().setHeight(24_px)),
                    // A few DateAndTime results, so the utility is visible too.
                    text(Modifier().setHeight(22_px),
                         TextOptions()
                            .setContent("today: " + DateAndTime::format(DateAndTime::today(), "dddd, MMMM D, YYYY"))
                            .setCharSize(13).setColorRole("textMuted")),
                    text(Modifier().setHeight(22_px),
                         TextOptions()
                            .setContent("ISO week " + std::to_string(DateAndTime::weekOfYear(DateAndTime::today()))
                                        + " | day " + std::to_string(DateAndTime::dayOfYear(DateAndTime::today()))
                                        + " of " + std::to_string(DateAndTime::daysInYear(DateAndTime::today().year)))
                            .setCharSize(13).setColorRole("textMuted")),
                    text(Modifier().setHeight(22_px),
                         TextOptions()
                            .setContent("now: " + DateAndTime::toISO(DateAndTime::nowLocal())
                                        + "  UTC" + (DateAndTime::timeZoneOffsetMinutes() >= 0 ? "+" : "")
                                        + std::to_string(DateAndTime::timeZoneOffsetMinutes() / 60))
                            .setCharSize(13).setColorRole("textMuted"))
                })
            })
        }, "root");

    ui.addPage(page(root, "main_page"));
    ui.setPage("main_page");

    std::fprintf(stderr, "[UILO] bgfx renderer: %s\n",
                 bgfx::getRendererName(bgfx::getCaps()->rendererType));

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
