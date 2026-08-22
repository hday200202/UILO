/*
    MacFileDialog.mm:
    - Desc: The macOS backend for utils/FileDialog.hpp: NSOpenPanel and
            NSSavePanel, run modally. Compiled only on macOS; every other
            platform takes the implementation in utils/FileDialog.cpp, which is
            guarded to compile to nothing here.
    - Panels are run with runModal rather than a sheet, so the call blocks and
      returns the answer. A sheet would need the caller to own a completion
      handler and a window to attach to, which is not what a Browse button
      wants.
*/

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_OSX

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UTType.h>

#include "../utils/FileDialog.hpp"

namespace uilo {

namespace {

NSString* toNS(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

std::filesystem::path fromURL(NSURL* url) {
    if (!url) return {};
    return std::filesystem::path([[url path] UTF8String]);
}

/*
    applyCommon(NSSavePanel* panel, const FileDialogOptions& options):
    - Params:   NSSavePanel* panel, const FileDialogOptions& options
    - Returns:  void
    - Desc:     Puts the title, the starting directory and the filter list on a
                panel. NSOpenPanel derives from NSSavePanel, so both kinds are
                configured here.
    - allowAllFiles leaves the panel's type list unset, which is how AppKit
      spells "anything": setting a list and then adding a wildcard type is not
      the same thing, and would grey out files the user can see.
*/
void applyCommon(NSSavePanel* panel, const FileDialogOptions& options) {
    if (!options.title.empty()) [panel setTitle:toNS(options.title)];

    if (!options.startPath.empty()) {
        std::error_code ec;
        const bool isDir = std::filesystem::is_directory(options.startPath, ec);
        const std::filesystem::path dir = isDir ? options.startPath
                                                : options.startPath.parent_path();
        if (!dir.empty())
            [panel setDirectoryURL:[NSURL fileURLWithPath:toNS(dir.string())
                                              isDirectory:YES]];
        if (!isDir && options.startPath.has_filename())
            [panel setNameFieldStringValue:toNS(options.startPath.filename().string())];
    }

    if (!options.defaultName.empty())
        [panel setNameFieldStringValue:toNS(options.defaultName)];

    if (!options.filters.empty() && !options.allowAllFiles) {
        NSMutableArray<UTType*>* types = [NSMutableArray array];
        for (const FileFilter& filter : options.filters) {
            for (const std::string& extension : filter.extensions) {
                UTType* type = [UTType typeWithFilenameExtension:toNS(extension)];
                if (type) [types addObject:type];
            }
        }
        if ([types count] > 0) [panel setAllowedContentTypes:types];
    }
}

/*
    prepareApp():
    - Params:   none
    - Returns:  void
    - Desc:     Makes sure there is an NSApplication and that it is frontmost
                before a panel is run. A UILO app already has one -- SDL creates
                it -- but a tool that has not opened a window yet does not, and
                a panel run without one either does not appear or opens behind
                whatever is in front. Raising the app is also what stops the
                panel from being buried by the editor the user launched from.
*/
void prepareApp() {
    if (NSApp == nil) [NSApplication sharedApplication];
    if ([NSApp activationPolicy] == NSApplicationActivationPolicyProhibited)
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

} // namespace


bool fileDialogsAvailable() { return true; }


std::optional<std::filesystem::path> openFileDialog(const FileDialogOptions& options) {
    @autoreleasepool {
        prepareApp();
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        applyCommon(panel, options);

        if ([panel runModal] != NSModalResponseOK) return std::nullopt;
        return fromURL([[panel URLs] firstObject]);
    }
}


std::vector<std::filesystem::path> openFilesDialog(const FileDialogOptions& options) {
    @autoreleasepool {
        prepareApp();
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:YES];
        applyCommon(panel, options);

        std::vector<std::filesystem::path> chosen;
        if ([panel runModal] != NSModalResponseOK) return chosen;
        for (NSURL* url in [panel URLs]) chosen.push_back(fromURL(url));
        return chosen;
    }
}


std::optional<std::filesystem::path> saveFileDialog(const FileDialogOptions& options) {
    @autoreleasepool {
        prepareApp();
        NSSavePanel* panel = [NSSavePanel savePanel];
        applyCommon(panel, options);

        if ([panel runModal] != NSModalResponseOK) return std::nullopt;
        return fromURL([panel URL]);
    }
}


std::optional<std::filesystem::path> selectFolderDialog(const FileDialogOptions& options) {
    @autoreleasepool {
        prepareApp();
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        [panel setCanCreateDirectories:YES];
        applyCommon(panel, options);

        if ([panel runModal] != NSModalResponseOK) return std::nullopt;
        return fromURL([[panel URLs] firstObject]);
    }
}

} // namespace uilo

#endif   /* __APPLE__ && TARGET_OS_OSX */
