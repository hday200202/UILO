#pragma once
#ifdef UILO_WT

/*
    UILO -> Wt bridge.

    The entry point is UILO::runWeb(WebConfig) (declared in UILO.hpp) -- the web
    counterpart of the desktop update/render loop. There is no Session type: you
    drive the plain UILO instance exactly as on desktop, and runWeb serves it.

    The two helpers below cover the rare web-only needs that have no desktop
    equivalent.
*/

#include <chrono>
#include <functional>

namespace Wt { class WApplication; }

namespace uilo::wt {

// The current session's Wt application. You should not need this -- it exists so
// a one-off Wt-specific need doesn't force a fork.
Wt::WApplication& application();

// Runs `fn` every `interval` for the life of the session, then re-syncs the
// tree. The web stand-in for periodic work you'd do in the desktop update loop
// (a clock, polling a data source, ...).
void every(std::chrono::milliseconds interval, std::function<void()> fn);

} // namespace uilo::wt

#endif   /* UILO_WT */
