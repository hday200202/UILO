#pragma once

#include "elements/Elements.hpp"
#include "Page.hpp"

namespace uilo {

inline Column* column(Modifier modifier, contains children, const std::string& name = "") {
    return new Column(modifier, children, name);
}

inline Row* row(Modifier modifier, contains children, const std::string& name = "") {
    return new Row(modifier, children, name);
}

inline Spacer* spacer(Modifier modifier, const std::string& name = "") {
    return new Spacer(modifier, name);
}

inline Page* page(Container* root, const std::string& name) {
    return new Page(root, name);
}

} // namespace uilo
