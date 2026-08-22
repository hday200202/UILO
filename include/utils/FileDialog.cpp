/*
    FileDialog.cpp:
    - Desc: The Windows and Linux backends for FileDialog.hpp. macOS is in
            platform/MacFileDialog.mm and this file compiles to nothing there,
            the same way MacStubs.cpp is guarded the other way round.
    - Windows uses IFileDialog, the shell's own dialog since Vista. Linux has no
      one dialog, so it drives whichever helper is installed -- the XDG desktop
      portal's zenity, or kdialog on KDE -- and reports unavailable when there
      is none rather than pretending.
*/

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if !(defined(__APPLE__) && TARGET_OS_OSX)

#include "FileDialog.hpp"

#include <algorithm>

#if defined(_WIN32)

#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>

namespace uilo {

namespace {

/* UTF-8 to UTF-16, which is the only string type the shell dialogs take. */
std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int need = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), need);
    return out;
}

std::string narrow(const std::wstring& s) {
    if (s.empty()) return {};
    const int need = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(),
                                         nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(),
                        out.data(), need, nullptr, nullptr);
    return out;
}

/*
    FilterSpecs:
    - Desc: The COMDLG_FILTERSPEC array the dialog wants, plus the strings it
            points into. The API takes bare pointers and keeps no copies, so the
            backing strings have to outlive the call -- which is the whole
            reason this is a type and not a function.
*/
struct FilterSpecs {
    std::vector<std::wstring>     storage;
    std::vector<COMDLG_FILTERSPEC> specs;

    explicit FilterSpecs(const FileDialogOptions& options) {
        storage.reserve(options.filters.size() * 2 + 2);
        for (const FileFilter& filter : options.filters) {
            std::string patterns;
            for (const std::string& extension : filter.extensions) {
                if (!patterns.empty()) patterns += ";";
                patterns += "*." + extension;
            }
            storage.push_back(widen(filter.label));
            storage.push_back(widen(patterns));
        }
        if (options.allowAllFiles || options.filters.empty()) {
            storage.push_back(widen("All files"));
            storage.push_back(widen("*.*"));
        }
        for (size_t i = 0; i + 1 < storage.size(); i += 2)
            specs.push_back({ storage[i].c_str(), storage[i + 1].c_str() });
    }
};

/* Runs a configured dialog and collects what came back. */
std::vector<std::filesystem::path> runDialog(const FileDialogOptions& options,
                                             bool save, bool folders, bool multiple) {
    std::vector<std::filesystem::path> chosen;

    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool weInitialised = SUCCEEDED(init);

    IFileDialog* dialog = nullptr;
    const CLSID which = save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    if (SUCCEEDED(CoCreateInstance(which, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&dialog)))) {
        DWORD flags = 0;
        dialog->GetOptions(&flags);
        if (folders)  flags |= FOS_PICKFOLDERS;
        if (multiple) flags |= FOS_ALLOWMULTISELECT;
        dialog->SetOptions(flags | FOS_FORCEFILESYSTEM);

        if (!options.title.empty()) dialog->SetTitle(widen(options.title).c_str());

        FilterSpecs filters(options);
        if (!folders && !filters.specs.empty())
            dialog->SetFileTypes(static_cast<UINT>(filters.specs.size()), filters.specs.data());

        if (!options.defaultName.empty())
            dialog->SetFileName(widen(options.defaultName).c_str());

        if (!options.startPath.empty()) {
            std::error_code ec;
            const bool isDir = std::filesystem::is_directory(options.startPath, ec);
            const std::filesystem::path dir = isDir ? options.startPath
                                                    : options.startPath.parent_path();
            IShellItem* item = nullptr;
            if (!dir.empty() && SUCCEEDED(SHCreateItemFromParsingName(
                    widen(dir.string()).c_str(), nullptr, IID_PPV_ARGS(&item)))) {
                dialog->SetFolder(item);
                item->Release();
            }
        }

        if (SUCCEEDED(dialog->Show(nullptr))) {
            auto take = [&](IShellItem* item) {
                PWSTR raw = nullptr;
                if (item && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw))) {
                    chosen.emplace_back(narrow(raw));
                    CoTaskMemFree(raw);
                }
            };

            IFileOpenDialog* openDialog = nullptr;
            if (multiple && SUCCEEDED(dialog->QueryInterface(IID_PPV_ARGS(&openDialog)))) {
                IShellItemArray* items = nullptr;
                if (SUCCEEDED(openDialog->GetResults(&items))) {
                    DWORD count = 0;
                    items->GetCount(&count);
                    for (DWORD i = 0; i < count; ++i) {
                        IShellItem* item = nullptr;
                        if (SUCCEEDED(items->GetItemAt(i, &item))) { take(item); item->Release(); }
                    }
                    items->Release();
                }
                openDialog->Release();
            } else {
                IShellItem* item = nullptr;
                if (SUCCEEDED(dialog->GetResult(&item))) { take(item); item->Release(); }
            }
        }
        dialog->Release();
    }

    if (weInitialised) CoUninitialize();
    return chosen;
}

} // namespace

bool fileDialogsAvailable() { return true; }

#else   /* ---- Linux and the other unices ---- */

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace uilo {

namespace {

/* Single-quoted for /bin/sh: everything is literal inside, so only the quote
   itself needs escaping. */
std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else           out += c;
    }
    return out + "'";
}

bool haveCommand(const char* name) {
    const std::string probe = "command -v " + std::string(name) + " >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

/* Which helper to drive, decided once. kdialog first on KDE, zenity otherwise,
   since each looks native only on its own desktop. */
const char* helper() {
    static const char* found = [] () -> const char* {
        const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
        const bool kde = desktop && std::string(desktop).find("KDE") != std::string::npos;
        if (kde && haveCommand("kdialog")) return "kdialog";
        if (haveCommand("zenity"))         return "zenity";
        if (haveCommand("kdialog"))        return "kdialog";
        if (haveCommand("qarma"))          return "qarma";
        if (haveCommand("matedialog"))     return "matedialog";
        return nullptr;
    }();
    return found;
}

/* Runs a command and returns everything it printed, trailing newline removed. */
std::string capture(const std::string& command) {
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) return {};

    std::string out;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()))
        out += buffer.data();
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

std::string zenityFilters(const FileDialogOptions& options) {
    std::string args;
    for (const FileFilter& filter : options.filters) {
        std::string spec = filter.label + " |";
        for (const std::string& extension : filter.extensions) spec += " *." + extension;
        args += " --file-filter=" + shellQuote(spec);
    }
    if (options.allowAllFiles || options.filters.empty())
        args += " --file-filter=" + shellQuote("All files | *");
    return args;
}

/* kdialog takes one filter string: "*.png *.jpg|Images". */
std::string kdialogFilters(const FileDialogOptions& options) {
    std::string spec;
    for (const FileFilter& filter : options.filters) {
        if (!spec.empty()) spec += "\n";
        std::string patterns;
        for (const std::string& extension : filter.extensions) {
            if (!patterns.empty()) patterns += " ";
            patterns += "*." + extension;
        }
        spec += patterns + "|" + filter.label;
    }
    if (options.allowAllFiles || options.filters.empty()) {
        if (!spec.empty()) spec += "\n";
        spec += "*|All files";
    }
    return spec;
}

std::vector<std::filesystem::path> runDialog(const FileDialogOptions& options,
                                             bool save, bool folders, bool multiple) {
    std::vector<std::filesystem::path> chosen;
    const char* tool = helper();
    if (!tool) return chosen;

    const std::string start = options.startPath.empty() ? std::string()
                                                        : options.startPath.string();
    std::string command;

    if (std::string(tool) == "kdialog") {
        const std::string what = folders ? "--getexistingdirectory"
                               : save    ? "--getsavefilename"
                                         : "--getopenfilename";
        command = std::string(tool);
        if (!options.title.empty()) command += " --title " + shellQuote(options.title);
        if (multiple)               command += " --multiple --separate-output";
        command += " " + what + " " + shellQuote(start.empty() ? "." : start);
        if (!folders) command += " " + shellQuote(kdialogFilters(options));
    } else {
        command = std::string(tool) + " --file-selection";
        if (!options.title.empty()) command += " --title=" + shellQuote(options.title);
        if (save)     command += " --save --confirm-overwrite";
        if (folders)  command += " --directory";
        if (multiple) command += " --multiple --separator='\\n'";
        if (!start.empty()) command += " --filename=" + shellQuote(start);
        else if (!options.defaultName.empty())
            command += " --filename=" + shellQuote(options.defaultName);
        if (!folders) command += zenityFilters(options);
    }

    command += " 2>/dev/null";

    const std::string output = capture(command);
    if (output.empty()) return chosen;

    size_t start_at = 0;
    while (start_at <= output.size()) {
        const size_t nl = output.find('\n', start_at);
        const std::string line = output.substr(start_at, nl == std::string::npos
                                                       ? std::string::npos : nl - start_at);
        if (!line.empty()) chosen.emplace_back(line);
        if (nl == std::string::npos) break;
        start_at = nl + 1;
    }
    return chosen;
}

} // namespace

bool fileDialogsAvailable() { return helper() != nullptr; }

#endif  /* _WIN32 */


/* ---- the four calls, shared by both backends above ---------------------- */

std::optional<std::filesystem::path> openFileDialog(const FileDialogOptions& options) {
    auto chosen = runDialog(options, false, false, false);
    if (chosen.empty()) return std::nullopt;
    return chosen.front();
}

std::vector<std::filesystem::path> openFilesDialog(const FileDialogOptions& options) {
    return runDialog(options, false, false, true);
}

std::optional<std::filesystem::path> saveFileDialog(const FileDialogOptions& options) {
    auto chosen = runDialog(options, true, false, false);
    if (chosen.empty()) return std::nullopt;
    return chosen.front();
}

std::optional<std::filesystem::path> selectFolderDialog(const FileDialogOptions& options) {
    auto chosen = runDialog(options, false, true, false);
    if (chosen.empty()) return std::nullopt;
    return chosen.front();
}

} // namespace uilo

#endif  /* !(__APPLE__ && TARGET_OS_OSX) */
