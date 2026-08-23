# Theme.hpp

`include/utils/Theme.hpp`

[← index](../README.md)

## Types

- [Theme](#theme)
- [UILO_THEMED](#uilo-themed)

## Functions

- [`installDefaultTheme(Theme& theme)`](#installdefaulttheme)
- [`edit(std::string_view role)`](#edit)
- [`lookup(std::string_view role)`](#lookup)
- [`cascade(std::string_view role)`](#cascade)
- [`hasRole(std::string_view role)`](#hasrole)

---

### Theme

What everything looks like before anyone says otherwise: the palette, and a prototype for every [Modifier](../elements/Modifier.hpp.md#modifier) and *Options type in the library. A [UILO](../UILO.hpp.md#uilo) owns one, so two of them can look different in the same process -- which is what a Wt session needs.

```cpp
ui.getTheme().palette().set("accent", {90, 140, 255, 255});
ui.setTheme(myTheme);
```


> The prototypes live in Defaults.hpp. That file is the whole of the library's look, and editing it is the intended way to restyle an application wholesale.

> A theme is applied when an element binds to its [UILO](../UILO.hpp.md#uilo), not when the element is constructed -- an element is usually built before the [UILO](../UILO.hpp.md#uilo) exists. Only fields the call site never set are filled in, so a theme fills gaps and never overrides a choice. Changing a [UILO](../UILO.hpp.md#uilo)'s theme re-applies it to everything already built, so restyling needs no rebuild.

> Named roles work the same way the palette's do, for shape rather than colour. A theme can register any number of variants per type under a name, and constructing a type with that name records it so the right prototype is found at bind time:

```cpp
TextOptions h1 = theme.edit<TextOptions>("h1");
h1.setCharSize(42);
text(Modifier("h1"), TextOptions("h1"));
```

[Modifier](../elements/Modifier.hpp.md#modifier) and *Options roles are looked up separately, so the same name can mean a size on one and a colour on the other. An unknown role is not an error: it falls back to the type's default, the way an unknown CSS class leaves an element unstyled. Ask hasRole() when a typo would matter.

---

### UILO_THEMED

The two constructors and the role accessor every themed type shares, spelled once. Takes the type it is expanded in.

```cpp
T()          the type's baseline; the theme fills it in
             when the element binds
T("h1")      the same, but recording that it wants the
             theme's "h1" variant
```

A type opts in by writing UILO_THEMED([TextOptions](../elements/decoration/Text.hpp.md#textoptions)) where its default constructor used to be. Its member initialisers stay exactly as they were and become the baseline that shows through wherever neither the caller nor the theme has an opinion.

---

### installDefaultTheme

```cpp
installDefaultTheme(Theme& theme)
```

**Parameters**

- `Theme& theme`

**Returns** — void

Fills a theme with everything Defaults.hpp declares. Declared here and defined in Defaults.cpp, so a [UILO](../UILO.hpp.md#uilo) can build its theme without this header -- which every themed type includes -- having to pull in every element type in turn.

---

### edit

```cpp
edit(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — T& -- the stored prototype, to change in place

The prototype for a type and role, created empty on first use.

> A role holds only what it changes, never a copy of the default. Seeding it from the default would look convenient and then go stale: the copy would keep whatever the default held at that moment, so a later change to the default would reach every plain element and silently skip the ones that named a role. cascade() composes the two at read time instead, which is what keeps the two in step.

> The reference is invalidated by a later edit() of the same type, so change one thing and let go of it.

---

### lookup

```cpp
lookup(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — const T* -- null when unset

The prototype this theme holds for a type and role. The pointer stays valid until that role is redefined or the theme is replaced, which is long enough to inherit from.

---

### cascade

```cpp
cascade(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — T -- the role over the type's default

What an element with this role inherits at bind time. A role says only what it changes, so the type's default has to fill in the rest: "primary" names a fill and nothing else, and still has to arrive carrying the radius every button shares.

> Composed on every read rather than when the role was defined. edit() seeds a new role from the default as a convenience, but that is a snapshot -- editing the default afterwards has to reach the roles built on it, or changing one shared value would quietly skip every element that named a role. That is exactly what a stylesheet does: the class refines the element's own style rather than replacing it.

> An unknown role is not an error; it comes back as the plain default.

---

### hasRole

```cpp
hasRole(std::string_view role)
```

**Parameters**

- `std::string_view role`

**Returns** — bool

Whether this theme defines that role for T. Worth asking before relying on one, since binding is silent about a name it does not know.
