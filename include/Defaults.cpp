#include "Defaults.hpp"

namespace uilo {

/*
    installDefaultTheme(Theme& theme):
    - Params:   Theme& theme
    - Returns:  void
    - Desc:     Fills a theme with defaultTheme(). Theme.hpp declares this and
                this file defines it, which is what keeps the dependency
                pointing one way: every *Options header includes Theme.hpp, so
                Theme.hpp including Defaults.hpp -- and through it every element
                type -- would close a cycle. Only this translation unit needs to
                know both halves.
*/
void installDefaultTheme(Theme& theme) {
    theme = defaultTheme();
}

} // namespace uilo
