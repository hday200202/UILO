#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace uilo {

/*
    ContextMenuItem:
    - Desc:     One line of a context menu: a label, an optional icon, and
                either an action to run or a submenu to open. A separator is an
                item with nothing on it but the flag.
    - Kept free of every element type on purpose. Modifier stores these, and
      Modifier is included by everything, so this header has to depend on
      nothing but the standard library.
    - Build them with menuItem(), menuSeparator() and menuSubmenu() rather than
      filling the struct in by hand.
*/
struct ContextMenuItem {
    std::string                  label;
    std::string                  icon;        /* Resources::icons::*, or empty */
    std::function<void()>        action;
    std::vector<ContextMenuItem> submenu;
    bool                         separator = false;
    bool                         enabled   = true;

    /* Greyed out and unclickable, which is how an item says "not right now"
       without disappearing and moving everything below it. */
    ContextMenuItem& setEnabled(bool v)             { enabled = v; return *this; }
    ContextMenuItem& setIcon(std::string_view p)    { icon = p;    return *this; }

    bool isSubmenu() const { return !submenu.empty(); }
};


/*
    menuItem(std::string label, std::function<void()> action):
    - Params:   std::string label, std::function<void()> action
    - Returns:  ContextMenuItem
    - Desc:     An ordinary item that runs `action` when it is picked.
*/
inline ContextMenuItem menuItem(std::string label, std::function<void()> action) {
    ContextMenuItem item;
    item.label  = std::move(label);
    item.action = std::move(action);
    return item;
}


/*
    menuItem(std::string label, std::string icon, std::function<void()> action):
    - Params:   std::string label, std::string_view icon, std::function<void()>
                action
    - Returns:  ContextMenuItem
    - Desc:     An item with an icon ahead of its label. Every item in a menu
                shares one icon column, so one item having an icon indents the
                labels of the rest to match.
*/
inline ContextMenuItem menuItem(
    std::string label,
    std::string_view icon,
    std::function<void()> action
) {
    ContextMenuItem item = menuItem(std::move(label), std::move(action));
    item.icon = icon;
    return item;
}


/*
    menuSeparator():
    - Params:   none
    - Returns:  ContextMenuItem
    - Desc:     A dividing rule. It takes a row of its own but cannot be
                highlighted, so keyboard navigation steps over it.
*/
inline ContextMenuItem menuSeparator() {
    ContextMenuItem item;
    item.separator = true;
    return item;
}


/*
    menuSubmenu(std::string label, std::vector<ContextMenuItem> children):
    - Params:   std::string label, std::vector<ContextMenuItem> children
    - Returns:  ContextMenuItem
    - Desc:     An item that opens a nested menu beside itself instead of
                running an action. Nests to any depth.
*/
inline ContextMenuItem menuSubmenu(
    std::string label,
    std::vector<ContextMenuItem> children
) {
    ContextMenuItem item;
    item.label   = std::move(label);
    item.submenu = std::move(children);
    return item;
}


/*
    menuSubmenu(std::string label, std::string icon, std::vector<ContextMenuItem> children):
    - Params:   std::string label, std::string_view icon,
                std::vector<ContextMenuItem> children
    - Returns:  ContextMenuItem
    - Desc:     menuSubmenu with an icon ahead of the label.
*/
inline ContextMenuItem menuSubmenu(
    std::string label,
    std::string_view icon,
    std::vector<ContextMenuItem> children
) {
    ContextMenuItem item = menuSubmenu(std::move(label), std::move(children));
    item.icon = icon;
    return item;
}


/*
    ContextMenuBuilder:
    - Desc:     What Modifier stores: a callable that produces the items. The
                list form of setContextMenu wraps a fixed vector in one of these,
                so there is a single code path and a menu whose contents depend
                on application state costs nothing extra.
*/
using ContextMenuBuilder = std::function<std::vector<ContextMenuItem>()>;

} // namespace uilo
