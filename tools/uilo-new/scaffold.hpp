#pragma once

/*
    scaffold.hpp:
    - Desc: What "new UILO project" means, kept apart from how it was asked for.
            The command line and the GUI both hand a Options here and print
            whatever comes back, so the two front-ends cannot drift on what they
            produce.
*/

#include <filesystem>
#include <string>
#include <vector>

namespace uilonew {

/* Which starter to write. Both produce the same folder shape; they differ in
   what main.cpp does and what the README tells you to try. */
enum class Template { Minimal, Panels };

const char* templateName(Template t);
Template    templateFromName(const std::string& name, bool* ok = nullptr);


/*
    Options:
    - Desc: One request to create a project. `path` is the only thing the caller
            has to supply -- the project's name defaults to the last component of
            it, which is what "give it a path" means.
*/
struct Options {
    std::filesystem::path path;
    std::string           name;                     /* default: path's filename */
    std::string           uiloRepo = "https://github.com/hday200202/UILO.git";
    std::string           uiloRef  = "main";        /* branch, tag or SHA */
    Template              tmpl     = Template::Minimal;
    bool                  force    = false;         /* write into a non-empty dir */
};


/*
    Result:
    - Desc: What happened, in a shape both front-ends can render: a line per file
            written, and either an error or the command to run next.
*/
struct Result {
    bool                     ok = false;
    std::string              error;                 /* empty when ok */
    std::vector<std::string> written;               /* paths, relative to the project */
    std::filesystem::path    root;
};

/* Whether a name is usable as a CMake target and a binary name. */
bool validName(const std::string& name, std::string* why = nullptr);

/* Checks the request without touching the disk. Same errors create() would
   give, so a GUI can grey its button out rather than failing on the click. */
std::string validate(const Options& options);

/* Writes the project. Creates every parent directory it needs. */
Result create(const Options& options);

} // namespace uilonew
