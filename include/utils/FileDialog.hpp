#pragma once

/*
    FileDialog.hpp:
    - Desc: The operating system's own open, save and folder dialogs behind one
            call. Each returns what the user chose, or nothing when they
            cancelled, so a caller never has to ask which platform it is on.
    - The dialogs are the platform's real ones -- NSOpenPanel on macOS,
      IFileDialog on Windows, and the desktop portal (or zenity/kdialog) on
      Linux. Nothing is drawn by UILO, which is why these are free functions
      rather than elements.
    - Every one of them BLOCKS until the user answers, and while a modal dialog
      is up the application's own loop is not running, so the window behind it
      stops redrawing. That is what the platform dialogs do, and for the usual
      case -- a Browse button next to a path field -- it is what you want. Call
      them from a click handler, not from inside a render pass.
    - On macOS they need an application to belong to, which means calling them
      from a UILO app: the Renderer's window brings up the NSApplication the
      panels attach to. A bare console program that never opened a window gets
      an immediate cancel rather than a panel, because macOS will not run a
      save/open panel for a process that is not a running app.
*/

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace uilo {

/*
    FileFilter:
    - Desc: One entry in a dialog's type menu: a label, and the extensions it
            matches. Extensions are given bare -- "png", not "*.png" -- and each
            platform is handed the spelling it expects.
*/
struct FileFilter {
    std::string              label;        /* "Images" */
    std::vector<std::string> extensions;   /* {"png", "jpg"} */
};


/*
    FileDialogOptions:
    - Desc: What to put on the dialog before it opens. Every field is optional;
            a default-constructed one gives the platform's own defaults.
*/
struct FileDialogOptions {
    std::string             title;         /* window title */
    std::filesystem::path   startPath;     /* directory to open in, or a file to preselect */
    std::string             defaultName;   /* the name a save dialog starts with */
    std::vector<FileFilter> filters;
    /* Offers "All files" alongside the filters, so a filter never hides
       something the user knows is there. */
    bool                    allowAllFiles = true;
};


/* One existing file. Empty when the dialog was cancelled or is unavailable. */
std::optional<std::filesystem::path> openFileDialog(const FileDialogOptions& options = {});

/* Several existing files. Empty when cancelled. */
std::vector<std::filesystem::path> openFilesDialog(const FileDialogOptions& options = {});

/* A path to write to, which may not exist yet. The platform asks about
   overwriting on the caller's behalf. */
std::optional<std::filesystem::path> saveFileDialog(const FileDialogOptions& options = {});

/* An existing directory. */
std::optional<std::filesystem::path> selectFolderDialog(const FileDialogOptions& options = {});

/*
    fileDialogsAvailable():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a dialog can actually be shown. True on macOS and
                Windows; on Linux it depends on a portal or zenity/kdialog being
                installed, so a headless or minimal system reports false and the
                four calls above all return nothing rather than hanging.
*/
bool fileDialogsAvailable();

} // namespace uilo
