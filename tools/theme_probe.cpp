/*
    theme_probe.cpp:
    - Desc: Theme probe. Binds elements to a UILO and asserts what the theme
            filled in, what it left alone, and what changed when the theme was
            replaced. Its subject is the one rule the whole system rests on: a
            theme fills gaps and never overrides a setting the call site made.
    - Needs no window and no GPU. A UILO with no renderer is enough, because
            binding -- not drawing -- is what applies a theme.
    - Prints one line per assertion and exits non-zero if any failed, so it is a
            test rather than a diff-me dump like layout_probe.cpp.
    - Not wired into CMake; build it with tools/probe.sh theme.
*/

#include "UILO.hpp"
#include "Defaults.hpp"
#include <cstdio>
#include <string>

using namespace uilo;

static int g_fail = 0;

static void checkInt(const char* what, long got, long want) {
    const bool ok = got == want;
    if (!ok) ++g_fail;
    std::printf("  %-4s %-56s got=%ld want=%ld\n", ok ? "ok" : "FAIL", what, got, want);
}

static void checkStr(const char* what, const std::string& got, const std::string& want) {
    const bool ok = got == want;
    if (!ok) ++g_fail;
    std::printf("  %-4s %-56s got=\"%s\" want=\"%s\"\n",
                ok ? "ok" : "FAIL", what, got.c_str(), want.c_str());
}

static void checkTrue(const char* what, bool got) {
    if (!got) ++g_fail;
    std::printf("  %-4s %s\n", got ? "ok" : "FAIL", what);
}

static void heading_(const char* name) { std::printf("\n=== %s ===\n", name); }

/* Binds a tree to a UILO, which is the moment a theme is applied. */
static void bind(UILO& ui, Container* root, const char* name) {
    ui.addPage(page(root, name));
    ui.setPage(name);
}

int main() {
    /* ------------------------------------------------------------------ */
    heading_("a UILO starts on Defaults.hpp");
    {
        UILO ui;
        checkTrue("the default theme carries a palette",
                  ui.getPalette().has("accent"));
        checkTrue("and the roles the helpers are built on",
                  ui.getTheme().hasRole<TextOptions>("h1"));
    }

    /* ------------------------------------------------------------------ */
    heading_("the theme fills in what the call site left alone");
    {
        UILO ui;
        Text* plain = text(Modifier(), TextOptions().setContent("x"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{plain}), "p1");

        checkStr("an unstyled Text takes the theme's colour role",
                 plain->getOptions().getColorRole(), "text");
        checkTrue("and no character size, so it still sizes from its height",
                  !plain->getOptions().hasCharSize());
    }

    /* ------------------------------------------------------------------ */
    heading_("a named role brings its own values");
    {
        UILO ui;
        Text* head = text(Modifier("h1"), TextOptions("h1").setContent("Title"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{head}), "p2");

        checkInt("TextOptions(\"h1\") takes the role's character size",
                 head->getOptions().getCharSize(), 42);
        checkTrue("and its weight", head->getOptions().getBold());
        checkInt("Modifier(\"h1\") takes the role's line box",
                 (long)head->getModifier().getHeight().value, 56);
    }

    /* ------------------------------------------------------------------ */
    heading_("an explicit setting always wins");
    {
        UILO ui;
        Text* pinned = text(
            Modifier("h1"),
            TextOptions("h1")
                .setContent("Title")
                .setCharSize(11)
        );
        bind(ui, column(Modifier(), ColumnOptions(), contains{pinned}), "p3");

        checkInt("a size set at the call site survives the h1 role",
                 pinned->getOptions().getCharSize(), 11);
        checkTrue("while the rest of the role still applies",
                  pinned->getOptions().getBold());
    }

    /* ------------------------------------------------------------------ */
    heading_("an unknown role falls back to the type's default");
    {
        UILO ui;
        Text* odd = text(Modifier("nope"), TextOptions("nope").setContent("x"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{odd}), "p4");

        checkStr("an unknown role is not an error",
                 odd->getOptions().getColorRole(), "text");
        checkTrue("and brings no size with it", !odd->getOptions().hasCharSize());
    }

    /* ------------------------------------------------------------------ */
    heading_("changing the theme restyles what is already built");
    {
        UILO ui;
        Text* head = text(Modifier("h1"), TextOptions("h1").setContent("Title"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{head}), "p5");
        checkInt("built at the default h1 size", head->getOptions().getCharSize(), 42);

        Theme bigger = defaultTheme();
        bigger.edit<TextOptions>("h1").setCharSize(64);
        ui.setTheme(bigger);

        checkInt("setTheme re-applies to the existing element, no rebuild",
                 head->getOptions().getCharSize(), 64);
    }

    /* ------------------------------------------------------------------ */
    heading_("editing the theme in place, then refreshing");
    {
        UILO ui;
        Text* head = text(Modifier("h2"), TextOptions("h2").setContent("Title"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{head}), "p6");

        ui.getTheme().edit<TextOptions>("h2").setCharSize(20);
        ui.refreshTheme();
        checkInt("refreshTheme picks up an in-place edit",
                 head->getOptions().getCharSize(), 20);
    }

    /* ------------------------------------------------------------------ */
    heading_("a role cascades over the type's default");
    {
        UILO ui;
        Button* plain = button(Modifier(), ButtonOptions());
        Button* named = button(Modifier(), ButtonOptions("primary"));
        bind(ui, column(Modifier(), ColumnOptions(), contains{plain, named}), "p9");

        /* The radius every button shares, changed after the roles were defined.
           An element that named a role has to see it too. */
        ui.getTheme().edit<ButtonOptions>().setRounding(7.f);
        ui.refreshTheme();

        checkInt("a plain button takes the new default radius",
                 (long)plain->getOptions().getRounding(), 7);
        checkInt("and so does one that named a role",
                 (long)named->getOptions().getRounding(), 7);
        checkStr("while the role's own setting still wins",
                 named->getOptions().getColorRole(), "accent");
    }

    /* ------------------------------------------------------------------ */
    heading_("a role's own value is not overwritten by the default");
    {
        UILO ui;
        Column* card_ = card(contains{});
        bind(ui, column(Modifier(), ColumnOptions(), contains{card_}), "p10");

        ui.getTheme().edit<ColumnOptions>().setRounding(0.f);
        ui.refreshTheme();

        checkInt("card keeps its own radius when the default changes",
                 (long)card_->getOptions().getRounding(), 10);
    }

    /* ------------------------------------------------------------------ */
    heading_("a widget's inner parts keep following the theme");
    {
        /* inheritRounding is how a composite widget hands a radius to a part it
           built. Passing nothing must leave that part following the theme, not
           pin it to "no rounding" -- getting this wrong silently freezes every
           inner part of every widget. */
        UILO ui;
        Button* part = button(
            Modifier(),
            ButtonOptions().inheritRounding(std::nullopt, 3.f)
        );
        bind(ui, column(Modifier(), ColumnOptions(), contains{part}), "p11");

        checkInt("with the theme silent, the widget's own fallback shows",
                 (long)part->getOptions().getRounding(), 3);

        ui.getTheme().edit<ButtonOptions>().setRounding(14.f);
        ui.refreshTheme();
        checkInt("and a themed radius still reaches it",
                 (long)part->getOptions().getRounding(), 14);
    }

    /* ------------------------------------------------------------------ */
    heading_("two UILOs, two looks, same process");
    {
        UILO a;
        UILO b;

        Theme red = defaultTheme();
        red.palette().set("accent", Color{255, 0, 0, 255});
        a.setTheme(red);

        Theme blue = defaultTheme();
        blue.palette().set("accent", Color{0, 0, 255, 255});
        b.setTheme(blue);

        checkInt("first UILO's accent is red",  a.getPalette().get("accent").r, 255);
        checkInt("second UILO's accent is blue", b.getPalette().get("accent").b, 255);
        checkInt("and they did not bleed into each other",
                 a.getPalette().get("accent").b, 0);
    }

    /* ------------------------------------------------------------------ */
    heading_("the helpers are the roles");
    {
        UILO ui;
        Text*   big  = h1("Title");
        Text*   note = caption("note");
        Column* box  = card(contains{ h3("Inner") });
        bind(ui, column(Modifier(), ColumnOptions(), contains{big, note, box}), "p7");

        checkInt("h1() is the h1 role",      big->getOptions().getCharSize(), 42);
        checkInt("caption() is the caption role", note->getOptions().getCharSize(), 14);
        checkStr("card() is the card role",  box->getOptions().getColorRole(), "panel");
        checkInt("and card() brings the role's border",
                 (long)box->getOptions().getOutlineThickness(), 1);
    }

    /* ------------------------------------------------------------------ */
    heading_("a theme reaches a widget's inner parts");
    {
        UILO ui;
        Button* b = primaryButton("Go", [] {});
        bind(ui, column(Modifier(), ColumnOptions(), contains{b}), "p8");

        checkStr("primaryButton() takes the accent fill",
                 b->getOptions().getColorRole(), "accent");
        checkTrue("and its label is a themed Text of its own",
                  b->getOptions().getLabel() != nullptr);
        if (b->getOptions().getLabel()) {
            checkStr("which reads against that fill",
                     b->getOptions().getLabel()->getOptions().getColorRole(), "onAccent");
        }
    }

    std::printf("\n%s\n", g_fail == 0 ? "all theme assertions pass"
                                      : "THEME ASSERTIONS FAILED");
    return g_fail == 0 ? 0 : 1;
}
