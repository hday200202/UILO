/*
    main.cpp:
    - Desc: The command line front-end, and the launcher for the GUI. Run with a
            path it creates a project and exits; run with nothing it opens the
            window, which is the same scaffolding code behind a form.
*/

#include "scaffold.hpp"

#include <cstdio>
#include <cstring>
#include <string>

int runGui();   /* gui.cpp */

namespace {

void usage() {
    std::puts(
        "uilo-new -- start a new UILO project\n"
        "\n"
        "usage:\n"
        "  uilo-new <path> [options]     create a project at <path>\n"
        "  uilo-new                      open the window instead\n"
        "\n"
        "options:\n"
        "  --name <name>       project and binary name (default: the last part of <path>)\n"
        "  --template <t>      minimal (default) or panels\n"
        "  --ref <ref>         UILO branch, tag or commit to pin (default: main)\n"
        "  --repo <url>        where to fetch UILO from\n"
        "  --force             write into a directory that is not empty\n"
        "  --gui               open the window even with a path given\n"
        "  -h, --help          this text\n"
        "\n"
        "The project is written with bin/, ext/ and src/ directories, a\n"
        "CMakeLists.txt and a build.sh. Nothing is downloaded here -- the first\n"
        "run of the generated build.sh fetches UILO, checks for the tools and\n"
        "libraries it needs, and offers to install whatever is missing.");
}

/* Reads the value that follows a flag, reporting the flag itself when the value
   is missing so the message names what the user actually typed. */
bool takeValue(int argc, char** argv, int& i, const char* flag, std::string& out) {
    if (i + 1 >= argc) {
        std::fprintf(stderr, "uilo-new: %s needs a value\n", flag);
        return false;
    }
    out = argv[++i];
    return true;
}

} // namespace


int main(int argc, char** argv) {
    uilonew::Options options;
    bool wantGui = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") { usage(); return 0; }
        if (arg == "--gui")   { wantGui = true; continue; }
        if (arg == "--force") { options.force = true; continue; }

        if (arg == "--name")     { if (!takeValue(argc, argv, i, "--name", options.name))     return 2; continue; }
        if (arg == "--ref")      { if (!takeValue(argc, argv, i, "--ref",  options.uiloRef))  return 2; continue; }
        if (arg == "--repo")     { if (!takeValue(argc, argv, i, "--repo", options.uiloRepo)) return 2; continue; }
        if (arg == "--template") {
            std::string value;
            if (!takeValue(argc, argv, i, "--template", value)) return 2;
            bool known = false;
            options.tmpl = uilonew::templateFromName(value, &known);
            if (!known) {
                std::fprintf(stderr, "uilo-new: unknown template '%s' (minimal, panels)\n", value.c_str());
                return 2;
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::fprintf(stderr, "uilo-new: unknown option '%s'\n", arg.c_str());
            return 2;
        }
        if (!options.path.empty()) {
            std::fprintf(stderr, "uilo-new: more than one path given\n");
            return 2;
        }
        options.path = arg;
    }

    if (wantGui || options.path.empty()) return runGui();

    const uilonew::Result result = uilonew::create(options);
    if (!result.ok) {
        std::fprintf(stderr, "uilo-new: %s\n", result.error.c_str());
        return 1;
    }

    for (const std::string& file : result.written)
        std::printf("  %s\n", file.c_str());

    const std::string root = result.root.string();
    std::printf("\nCreated %s\n\n  cd %s\n  ./build.sh run\n\n"
                "The first build fetches UILO and its dependencies, so it takes a while.\n",
                root.c_str(), root.c_str());
    return 0;
}
