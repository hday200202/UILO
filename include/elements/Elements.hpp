#pragma once

/*
    Elements.hpp:
    - Desc: Umbrella header pulling in every element type and the factory
            functions that build them. Including this is the ordinary way to use
            UILO's element tree; the individual headers exist for anything that
            wants to keep its includes narrow.
*/

// Containers
#include "containers/Column.hpp"
#include "containers/Row.hpp"
#include "containers/Canvas.hpp"

// Decoration
#include "decoration/Spacer.hpp"
#include "decoration/Image.hpp"
#include "decoration/Icon.hpp"
#include "decoration/Text.hpp"
#include "decoration/Waveform.hpp"

// Interactible
#include "interactible/Interactible.hpp"
#include "interactible/Slider.hpp"
#include "interactible/Button.hpp"
#include "interactible/Dropdown.hpp"
#include "interactible/Knob.hpp"
#include "interactible/Resizer.hpp"
#include "interactible/Textbox.hpp"

// Composite widgets
#include "widgets/Filebrowser.hpp"
#include "widgets/DatePicker.hpp"
#include "widgets/DateField.hpp"

// Lowercase factory functions
#include "Factory.hpp"