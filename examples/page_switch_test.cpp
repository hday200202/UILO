// Regression test for Element::setUILO double-registration: alternating the
// active page re-walks each page's tree; before the guard, every element got
// emplaced into m_elementPool again, causing a double delete at shutdown.
#include "../include/UILO.hpp"
#include <cstdio>

using namespace uilo;

static Container* makeTree(const char* name) {
    return column(
        Modifier(),
        ColumnOptions(),
        contains{
            row(Modifier().setHeight(50_pct), RowOptions()),
            row(Modifier().setHeight(50_pct), RowOptions()),
        },
        name
    );
}

int main() {
    // setActivePage takes caller-owned pages and re-walks the page tree via
    // Page::setUILO every time the page becomes active again.
    Page* a = page(makeTree("treeA"), "a");
    Page* b = page(makeTree("treeB"), "b");
    {
        UILO ui;
        for (int i = 0; i < 5; ++i) {
            ui.setActivePage(a);
            ui.setActivePage(b);
        }
    } // UILO destructs here; a double-registered pool aborts/crashes.
    std::puts("page_switch_test: OK");
    return 0;
}
