#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace uilo {

/*
    Resources
    - Desc: Process-wide registry for assets UILO ships with, plus anything the
            application wants to register alongside them. Reached through
            Resources::get().
    - The built-in icon set is embedded in the binary (see
             assets/EmbeddedIcons.hpp, generated from assets/icons/), so
             nothing has to be found on disk at runtime. Lookup is by name:

                icon({}, IconOptions().setIcon(Resources::icons::arrow_left));
                icon({}, IconOptions().setIcon("arrow-left"));

             The first form is a generated constant -- it autocompletes and a
             typo fails to compile. Both resolve through the same registry, so
             an application-registered icon behaves exactly like a built-in.
    - Registering is only about *source markup*. Parsing and rasterizing happen
      in the Icon element, lazily, cached per on-screen size.
    - Fonts work the same way, one level simpler. Resources::fonts::default_ names
      the DejaVu Sans built into the binary, which is also what any text that
      never had a font set renders with:

                text({}, TextOptions().setFont(Resources::fonts::default_));
                text({}, TextOptions());   identical -- default is implicit
*/
class Resources {
public:
    /* Defined by the generated header included at the bottom of this file: one
       `static constexpr std::string_view` per built-in icon, holding the. */
    struct icons;

    /* Named fonts. */
    struct fonts;

    static Resources& get();

    Resources(const Resources&)            = delete;
    Resources& operator=(const Resources&) = delete;

    /*
        IconRegistry
        - name -> SVG markup. Built-ins are served straight out of the embedded
          table; overrides and application icons live in a map beside it, so
          registering a name that already exists shadows the built-in rather
          than mutating it.
    */
    class IconRegistry {
    public:
        /* Empty view when the name is unknown -- callers treat that as "draw
           nothing" rather than an error, since a missing icon should not take. */
        std::string_view find(std::string_view name) const;
        std::string_view operator[](std::string_view name) const { return find(name); }
        bool contains(std::string_view name) const { return !find(name).empty(); }

        /* Registers (or shadows) an icon from markup already in memory. */
        void add(std::string name, std::string markup);
        /* Reads an .svg off disk and registers its contents. False if the file
           could not be read; the registry is left untouched. */
        bool addFile(std::string name, const std::filesystem::path& file);
        /* Registers every .svg in a directory, named after each file's stem.
           Returns how many were added. */
        std::size_t addDirectory(const std::filesystem::path& dir);

        void remove(std::string_view name);

        /* Every known name, built-ins and registered, sorted. */
        std::vector<std::string_view> names() const;
        /* Built-ins plus registered additions. */
        std::size_t size() const;
        /* Just the icons compiled into the binary. */
        static std::size_t builtInCount();

    private:
        /* Transparent hash/equal so find(string_view) looks up without
           allocating a std::string. Same pattern as Palette's role map. */
        struct StringHash {
            using is_transparent = void;
            std::size_t operator()(std::string_view s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
            std::size_t operator()(const std::string& s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
            std::size_t operator()(const char* s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
        };
        struct StringEq {
            using is_transparent = void;
            bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
            bool operator()(std::string_view a, const std::string& b) const noexcept { return a == b; }
            bool operator()(const std::string& a, std::string_view b) const noexcept { return a == b; }
            bool operator()(const std::string& a, const std::string& b) const noexcept { return a == b; }
        };

        std::unordered_map<std::string, std::string, StringHash, StringEq> m_added;
    };

    IconRegistry&       iconRegistry()       { return m_icons; }
    const IconRegistry& iconRegistry() const { return m_icons; }

    /*
        FontRegistry
        - name -> font file path. The built-in "default" maps to an empty path,
          which the renderer already understands as "use the embedded face", so
          the default font needs no file and no lookup table of bytes.
        - resolve() is deliberately forgiving: a string that is not a registered
          name comes back unchanged, so setFont("assets/fonts/X.ttf") keeps
          working exactly as before and names are simply an alternative.
    */
    class FontRegistry {
    public:
        FontRegistry();

        /* A registered name -> its path; anything else -> itself. */
        std::string_view resolve(std::string_view nameOrPath) const;

        /* Points a name at a font file. Registering "default" repoints what
           unset text renders with. */
        void add(std::string name, std::string path);
        void remove(std::string_view name);
        bool contains(std::string_view name) const;

        std::vector<std::string_view> names() const;

    private:
        struct StringHash {
            using is_transparent = void;
            std::size_t operator()(std::string_view s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
            std::size_t operator()(const std::string& s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
            std::size_t operator()(const char* s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
        };
        struct StringEq {
            using is_transparent = void;
            bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
            bool operator()(std::string_view a, const std::string& b) const noexcept { return a == b; }
            bool operator()(const std::string& a, std::string_view b) const noexcept { return a == b; }
            bool operator()(const std::string& a, const std::string& b) const noexcept { return a == b; }
        };

        std::unordered_map<std::string, std::string, StringHash, StringEq> m_fonts;
    };

    FontRegistry&       fontRegistry()       { return m_fonts; }
    const FontRegistry& fontRegistry() const { return m_fonts; }

private:
    Resources() = default;

    IconRegistry m_icons;
    FontRegistry m_fonts;
};

/*
    Resources:
    - Desc:     Compile-time names for the fonts that ship with UILO, so a call
                site spells one as Resources::fonts::default_ and gets a
                compiler error on a typo rather than a silent fallback at
                runtime.
*/
struct Resources::fonts {
    /* The DejaVu Sans compiled into the binary (assets/EmbeddedFont.hpp). */
    static constexpr std::string_view default_ = "default";
};

} // namespace uilo

/* Defines Resources::icons. Included last so Resources is a complete type by
   the time the nested struct is defined. */
#include "../assets/EmbeddedIcons.hpp"
