#pragma once

#include <optional>
#include <string>

#include "../Palette.hpp"

namespace uilo {

/*
    Theme
    - Desc: Application-wide defaults for the things every element draws with:
            the palette, corner rounding, outer padding, text size, font, icon
            stroke weight. Set once, and everything that has not been told
            otherwise follows.

                Theme::current()
                    .setPalette(Palette::defaultDark())
                    .setRounding(6.f)
                    .setFont("assets/Inter.ttf");

    - Three levels decide any themed value, in this order:

            1. what the element was given     .setRounding(0.f)
            2. what the theme says            Theme::current().setRounding(6.f)
            3. the type's own default         a plain Row is square

      So a manual setting always wins, and a theme only fills in what was left
      alone. Every field here is optional for that reason: a theme that says
      nothing about rounding changes nothing about rounding, which is why
      introducing one cannot alter how existing code looks.
    - Read where it is used, not copied at construction, so changing the theme
      restyles what is already on screen. There is no need to rebuild a page.
    - Composite widgets keep their own internal type scale. Theme::charSize()
      fills in for Text, Textbox and Dropdown -- the general-purpose controls,
      which already treat an unset size as "pick something sensible" -- and
      leaves a DatePicker's header/weekday/day sizes to the widget, since those
      are proportions of each other rather than of the application.
    - Outline is deliberately not here. A border applied to everything reads as
      noise rather than as a style -- every container and gap in a layout ends up
      ringed -- so setOutline{Color,ColorRole,Thickness} lives only on the
      individual Options, where it says something.
    - One theme per process. That is what "global" means here, and it is worth
      knowing for the Wt backend: sessions each own a UILO but share this, so
      per-session theming goes through UILO::setPalette() (and per-element
      settings) rather than through here.
*/
class Theme {
public:
    Theme() = default;

    /* The active theme. */
    /* The one every themed getter consults. */
    static Theme& current();
    static void   set(const Theme& theme);
    /* Back to saying nothing, so every type falls back to its own default. */
    static void   reset();

    /* Setters */
    Theme& setPalette(const Palette& p)      { m_palette = p; return *this; }
    /* Corner radius for every rounded surface in the library, a widget's inner
       parts included: an entry row in a FileBrowser and a day cell in a. */
    Theme& setRounding(float r)               { m_rounding = r; return *this; }
    /* Inset applied to every element that has not asked for its own. */
    Theme& setOuterPadding(float px)          { m_outerPadding = px; return *this; }
    Theme& setCharSize(unsigned int n)         { m_charSize = n; return *this; }
    /* Path to a .ttf, or empty for the embedded default face. */
    Theme& setFont(const std::string& path)    { m_font = path; return *this; }
    Theme& setIconStrokeWidth(float w)         { m_iconStroke = w; return *this; }

    Theme& clearPalette()         { m_palette.reset();      return *this; }
    Theme& clearRounding()        { m_rounding.reset();     return *this; }
    Theme& clearOuterPadding()    { m_outerPadding.reset(); return *this; }
    Theme& clearCharSize()        { m_charSize.reset();     return *this; }
    Theme& clearFont()            { m_font.reset();         return *this; }
    Theme& clearIconStrokeWidth() { m_iconStroke.reset();   return *this; }

    /* Raw reads. */
    /* Empty when the theme says nothing about that property. */
    const std::optional<Palette>&      paletteOpt()      const { return m_palette; }
    const std::optional<float>&        roundingOpt()     const { return m_rounding; }
    const std::optional<float>&        outerPaddingOpt() const { return m_outerPadding; }
    const std::optional<unsigned int>& charSizeOpt()     const { return m_charSize; }
    const std::optional<std::string>&  fontOpt()         const { return m_font; }
    const std::optional<float>&        iconStrokeOpt()   const { return m_iconStroke; }

    /* Resolvers. */
    /* What an Options getter calls: the element's own value if it has one,
       then the. */
    static float        resolveRounding(const std::optional<float>& own, float fallback);
    static float        resolveOuterPadding(const std::optional<float>& own, float fallback);
    static unsigned int resolveCharSize(const std::optional<unsigned int>& own, unsigned int fallback);
    static float        resolveIconStrokeWidth(const std::optional<float>& own, float fallback);
    /* A font path uses "" for unset, the convention everywhere else in the
       library, so there is no optional to pass. */
    static const std::string& resolveFont(const std::string& own);

    /* Whether a palette has been themed, and the palette itself -- an empty
       one when it has not, so this is always safe to resolve against. */
    static bool           hasPalette();
    static const Palette& palette();

private:
    std::optional<Palette>      m_palette;
    std::optional<float>        m_rounding;
    std::optional<float>        m_outerPadding;
    std::optional<unsigned int> m_charSize;
    std::optional<std::string>  m_font;
    std::optional<float>        m_iconStroke;
};

} // namespace uilo
