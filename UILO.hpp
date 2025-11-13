/* ---------------------------------------------------------------------------- //
 Uilo - Retained-mode UI library for SFML
 Author: Harrison Day 2025
 License: MPL-2.0 (see LICENSE.md for full terms)

 This software is provided as-is. Redistribution and modification are allowed
 under the terms of the Mozilla Public License 2.0.
// ---------------------------------------------------------------------------- */

#ifndef UILO_HPP
#define UILO_HPP

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <future>
#include <cmath>
#include <algorithm>
#include <optional>
#include <memory>
#include <fstream>

#include "assets/EmbeddedAssets.hpp"

namespace uilo {

// ---------------------------------------------------------------------------- //
// Rounded Rectangle Helper
// ---------------------------------------------------------------------------- //
inline sf::ConvexShape createRoundedRect(const sf::Vector2f& size, float radius, const sf::Color& color, unsigned int cornerPoints = 8) {
	sf::ConvexShape shape;
	
	if (radius <= 0.f || size.x <= 0.f || size.y <= 0.f) {
		// No rounding, create simple rectangle
		shape.setPointCount(4);
		shape.setPoint(0, {0.f, 0.f});
		shape.setPoint(1, {size.x, 0.f});
		shape.setPoint(2, {size.x, size.y});
		shape.setPoint(3, {0.f, size.y});
		shape.setFillColor(color);
		return shape;
	}
	
	// Clamp radius to half of smallest dimension
	radius = std::min(radius, std::min(size.x, size.y) * 0.5f);
	
	// Total points: 4 corners * cornerPoints each
	const unsigned int totalPoints = cornerPoints * 4;
	shape.setPointCount(totalPoints);
	
	unsigned int index = 0;
	const float pi = 3.14159265f;
	
	// Top-right corner
	for (unsigned int i = 0; i < cornerPoints; ++i) {
		float angle = -pi * 0.5f + (pi * 0.5f * i) / (cornerPoints - 1);
		shape.setPoint(index++, {
			size.x - radius + radius * std::cos(angle),
			radius + radius * std::sin(angle)
		});
	}
	
	// Bottom-right corner
	for (unsigned int i = 0; i < cornerPoints; ++i) {
		float angle = 0.f + (pi * 0.5f * i) / (cornerPoints - 1);
		shape.setPoint(index++, {
			size.x - radius + radius * std::cos(angle),
			size.y - radius + radius * std::sin(angle)
		});
	}
	
	// Bottom-left corner
	for (unsigned int i = 0; i < cornerPoints; ++i) {
		float angle = pi * 0.5f + (pi * 0.5f * i) / (cornerPoints - 1);
		shape.setPoint(index++, {
			radius + radius * std::cos(angle),
			size.y - radius + radius * std::sin(angle)
		});
	}
	
	// Top-left corner
	for (unsigned int i = 0; i < cornerPoints; ++i) {
		float angle = pi + (pi * 0.5f * i) / (cornerPoints - 1);
		shape.setPoint(index++, {
			radius + radius * std::cos(angle),
			radius + radius * std::sin(angle)
		});
	}
	
	shape.setFillColor(color);
	return shape;
}

// ---------------------------------------------------------------------------- //
// Forward Declarations
// ---------------------------------------------------------------------------- //
class Element;
class Container;
class Row;
class Column;
class Grid;
class Page;
class UILO;
class Slider;
class Knob;
class Text;
class TextBox;
class Spacer;
class Button;
class Dropdown;

// ---------------------------------------------------------------------------- //
// Global Ownership
// ---------------------------------------------------------------------------- //
inline static std::vector<std::unique_ptr<Element>> uilo_owned_elements;
inline static std::vector<std::unique_ptr<Element>> high_priority_elements;

// inline static std::unordered_map<std::string, Slider*> sliders;
// inline static std::unordered_map<std::string, Container*> containers;
// inline static std::unordered_map<std::string, Text*> texts;
// inline static std::unordered_map<std::string, Spacer*> spacers;
// inline static std::unordered_map<std::string, Button*> buttons;
// inline static std::unordered_map<std::string, Dropdown*> dropdowns;
// inline static std::unordered_map<std::string, Grid*> grids;

// Global render scale for viewport calculations
inline static float g_renderScale = 1.f;
extern sf::Font* g_defaultFont;

inline void setDefaultFont(sf::Font& font) {
	g_defaultFont = &font;
}

// Ensure embedded font is loaded
inline sf::Font* getDefaultFont() {
	if (!g_defaultFont) {
		static sf::Font embeddedFont;
		static bool initialized = false;
		if (!initialized) {
			bool fontLoaded = embeddedFont.openFromMemory(EMBEDDED_FONT.data(), EMBEDDED_FONT.size());
			if (fontLoaded) {
				g_defaultFont = &embeddedFont;
			} else {
				std::cerr << "UILO Error: Failed to load embedded font from memory!\n";
			}
			initialized = true;
		}
	}
	return g_defaultFont;
}

template <typename T, typename... Args>
T* obj(Args&&... args) {
	auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
	T* raw = ptr.get();
	uilo_owned_elements.emplace_back(std::move(ptr));
	return raw;
}

void cleanupMarkedElements();

// ---------------------------------------------------------------------------- //
// Alignment Enum
// ---------------------------------------------------------------------------- //
enum class Align : uint8_t {
	NONE        = 0,
	TOP         = 1 << 0,
	BOTTOM      = 1 << 1,
	LEFT        = 1 << 2,
	RIGHT       = 1 << 3,
	CENTER_X    = 1 << 4,
	CENTER_Y    = 1 << 5,
};

inline Align operator|(Align lhs, Align rhs);
inline Align operator&(Align lhs, Align rhs);
inline bool hasAlign(Align value, Align flag);

// ---------------------------------------------------------------------------- //
// Element Type Enum
// ---------------------------------------------------------------------------- //
enum class EType {
	Element,
	Row,
	ScrollableRow,
	Column,
	ScrollableColumn,
	Grid,
	FreeColumn,
	Text,
	Button,
};

// ---------------------------------------------------------------------------- //
// Button Styles
// ---------------------------------------------------------------------------- //
enum class ButtonStyle {
	Default,
	Pill,
	Rect,
};

// ---------------------------------------------------------------------------- //
// Slider Orientation
// ---------------------------------------------------------------------------- //
enum class SliderOrientation {
	Vertical,
	Horizontal,
};

// ---------------------------------------------------------------------------- //
// Text Box Styles
// ---------------------------------------------------------------------------- //
enum class TBStyle : uint8_t {
	Default		= 0,
	Pill		= 1 << 0,
	Wrap		= 1 << 1,
	CenterText	= 1 << 2,
	Password	= 1 << 3,
};

inline TBStyle operator|(TBStyle lhs, TBStyle rhs);
inline TBStyle operator&(TBStyle lhs, TBStyle rhs);
inline bool hasStyle(TBStyle value, TBStyle flag);

// ---------------------------------------------------------------------------- //
// Cursor Type Enum
// ---------------------------------------------------------------------------- //
enum class CursorType {
	Arrow,              // Default arrow
	Hand,               // Pointing hand
	IBeam,              // Text selection
	SizeHorizontal,     // Horizontal resize
	SizeVertical,       // Vertical resize
	SizeNWSE,           // Diagonal resize (top-left to bottom-right)
	SizeNESW,           // Diagonal resize (top-right to bottom-left)
	SizeAll,            // Move/drag all directions
	Cross,              // Crosshair
	NotAllowed,         // Not allowed/forbidden
};

using funcPtr = std::function<void()>;

class Modifier {
public:
	Modifier() = default;   
	Modifier& setWidth(float pct);
	Modifier& setHeight(float pct);
	Modifier& setfixedWidth(float px);
	Modifier& setfixedHeight(float px);
	Modifier& align(Align alignment);
	Modifier& setColor(sf::Color color);
	Modifier& onLClick(funcPtr cb);
	Modifier& onRClick(funcPtr cb);
	Modifier& setVisible(bool visible);
	Modifier& setHighPriority(bool highPriority);
	Modifier& setRounded(float radius);
	Modifier& setPadding(float padding);
	Modifier& fitContentWidth(bool fit);
	Modifier& fitContentHeight(bool fit);

	float getWidth() const;
	float getHeight() const;
	float getFixedWidth() const;
	float getFixedHeight() const;
	Align getAlignment() const;
	sf::Color getColor() const;
	const funcPtr& getOnLClick() const;
	const funcPtr& getOnRClick() const;
	bool isVisible() const;
	bool isHighPriority() const;
	float getRounded() const;
	float getPadding() const;
	bool getFitContentWidth() const;
	bool getFitContentHeight() const;

private:
	float m_widthPct = 1.f;
	float m_heightPct = 1.f;
	float m_fixedWidth = 0.f;
	float m_fixedHeight = 0.f;
	float m_rounded = 0.f;
	float m_padding = 0.f;
	bool m_isVisible = true;
	bool m_highPriority = false;
	bool m_fitContentWidth = false;
	bool m_fitContentHeight = false;
	Align m_alignment = Align::NONE;
	sf::Color m_color = sf::Color::Transparent;
	std::function<void()> m_onLClick = nullptr;
	std::function<void()> m_onRClick = nullptr;
};

inline Modifier default_mod;

// ---------------------------------------------------------------------------- //
// Element Base Class
// ---------------------------------------------------------------------------- //
class Element {
public:
	sf::RectangleShape m_bounds;
	sf::RectangleShape m_pastBounds;
	Modifier m_modifier;
	bool m_uiloOwned = false;
	bool m_isDirty = true;
	bool m_markedForDeletion = false;
	bool m_doRender = true;
	bool m_isHovered = false;
	std::vector<std::shared_ptr<sf::Drawable>> m_customGeometry;
	UILO* m_uilo = nullptr;

	std::string m_name = "";

	Element();
	virtual ~Element();

	virtual void update(sf::RectangleShape& parentBounds);
	virtual void updateChildren();
	virtual void render(sf::RenderTarget& target) {
		sf::RenderStates states;
		states.transform.translate(m_bounds.getPosition());
		for (auto& d : m_customGeometry) {
			target.draw(*d, states);
		}
	}
	virtual void handleEvent(const sf::Event& event);
	virtual bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button);
	virtual void checkHover(const sf::Vector2f& pos);
	virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {}
	void setModifier(const Modifier& modifier);
	virtual EType getType() const;
	sf::Vector2f getPosition() const { return m_bounds.getPosition(); }
	void setPosition(const sf::Vector2f& newPos) { m_bounds.setPosition(newPos); }
	sf::Vector2f getSize() const { return m_bounds.getSize(); }
	bool isHovered() const { return m_isHovered; }
	void setCustomGeometry(std::vector<std::shared_ptr<sf::Drawable>>& customGeometry);
	virtual void setUilo(UILO* uilo) { m_uilo = uilo; }

protected:
	void resize(const sf::RectangleShape& parent = sf::RectangleShape(), const bool inSlot = false);
	void applyModifiers();
};

// ---------------------------------------------------------------------------- //
// Container Base Class
// ---------------------------------------------------------------------------- //
class Container : public Element {
public:
	Container(std::initializer_list<Element*> elements = {}, const std::string& name = "");
	Container(Modifier modifier, std::initializer_list<Element*> elements = {}, const std::string& name = "");
	~Container();

	void addElement(Element* element);
	void addElements(std::initializer_list<Element*> elements);
	virtual void handleEvent(const sf::Event& event) override;
	virtual void updateChildren() override;
	const std::vector<Element*>& getElements() const;
	virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override {};
	void clear();
	virtual void setUilo(UILO* uilo) override;

	inline int getElementIndex(Element* element) const {
		auto it = std::find(m_elements.begin(), m_elements.end(), element);
		if (it != m_elements.end()) {
			return static_cast<int>(std::distance(m_elements.begin(), it));
		}
		return -1; // Element not found
	}

	inline void insertElementAt(Element* element, int index) {
		if (index < 0 || index > static_cast<int>(m_elements.size())) {
			std::cerr << "Index out of bounds in container: " << m_name << std::endl;
			return;
		}
		auto it = std::find(m_elements.begin(), m_elements.end(), element);
		if (it != m_elements.end()) {
			m_elements.erase(it);
		}
		m_elements.insert(m_elements.begin() + index, element);
	}

	inline void removeElement(Element* element) {
		auto it = std::find(m_elements.begin(), m_elements.end(), element);
		if (it != m_elements.end()) {
			m_elements.erase(it);
		} else {
			std::cerr << "Element not found in container: " << m_name << std::endl;
		}
	}

	inline void swapElements(Element* a, Element* b) {
		auto itA = std::find(m_elements.begin(), m_elements.end(), a);
		auto itB = std::find(m_elements.begin(), m_elements.end(), b);
		if (itA != m_elements.end() && itB != m_elements.end()) {
			std::iter_swap(itA, itB);
		} else {
			std::cerr << "One or both elements not found in container: " << m_name << std::endl;
		}
	}

	inline void swapElements(int indexA, int indexB) {
		if (indexA < 0 || indexB < 0 || indexA >= static_cast<int>(m_elements.size()) || indexB >= static_cast<int>(m_elements.size())) {
			std::cerr << "Index out of bounds in container: " << m_name << std::endl;
			return;
		}
		std::iter_swap(m_elements.begin() + indexA, m_elements.begin() + indexB);
	}

protected:
	std::vector<Element*> m_elements;
};

// ---------------------------------------------------------------------------- //
// Row Container
// ---------------------------------------------------------------------------- //
class Row : public Container {
public:
	using Container::Container;

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;
	virtual EType getType() const override;
	virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override
	{ for (auto& e : m_elements) e->checkScroll(pos, verticalDelta, horizontalDelta); }

private:
	inline void applyVerticalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};

// ---------------------------------------------------------------------------- //
// Scrollable Row Container
// ---------------------------------------------------------------------------- //
class ScrollableRow : public Row {
public:
	using Row::Row;
	using Row::checkClick;
	using Row::checkHover;

	void update(sf::RectangleShape& parentBounds) override;
	void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override;
	virtual EType getType() const override;

	void render(sf::RenderTarget& target) override;

	void setScrollSpeed(float speed) { m_scrollSpeed = speed; }
	void setOffset(float offset) { m_offset = offset; }
	float getOffset() const { return m_offset; }
	float getScrollSpeed() const { return m_scrollSpeed; }
	
	void lock() { m_locked = true; }
	void unlock() { m_locked = false; }
	bool isLocked() const { return m_locked; }

private:
	float m_offset = 0.f;
	float m_scrollSpeed = 10.f;
	bool m_locked = false;
};

// ---------------------------------------------------------------------------- //
// Column Container
// ---------------------------------------------------------------------------- //
class Column : public Container {
public:
	using Container::Container;

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;
	virtual EType getType() const override;
	virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override
	{ for (auto& e : m_elements) e->checkScroll(pos, verticalDelta, horizontalDelta); }

private:
	inline void applyHorizontalAlignment(Element* e, const sf::RectangleShape& parentBounds);
};

// ---------------------------------------------------------------------------- //
// Scrollable Column Container
// ---------------------------------------------------------------------------- //
class ScrollableColumn : public Column {
public:
	using Column::Column;
	using Column::render;
	using Column::checkClick;
	using Column::checkHover;

	void update(sf::RectangleShape& parentBounds) override;
	void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override;
	virtual EType getType() const override;

	void setScrollSpeed(float speed) { m_scrollSpeed = speed; }
	void setOffset(float offset) { m_offset = offset; }
	float getOffset() const { return m_offset; }
	float getScrollSpeed() const { return m_scrollSpeed; }
	
	void lock() { m_locked = true; }
	void unlock() { m_locked = false; }
	bool isLocked() const { return m_locked; }

private:
	float m_offset = 0.f;
	float m_scrollSpeed = 10.f;
	bool m_locked = false;
};

// ---------------------------------------------------------------------------- //
// Grid Container
// ---------------------------------------------------------------------------- //
class Grid : public Container {
public:
	Grid(
		Modifier modifier = default_mod,
		float cellWidth = 100.f,
		float cellHeight = 100.f,
		int columns = 0,
		int rows = 0,
		std::initializer_list<Element*> elements = {},
		const std::string& name = ""
	);

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;
	void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override;
	virtual EType getType() const override;

	void setCellSize(float width, float height) { m_cellWidth = width; m_cellHeight = height; }
	void setGridDimensions(int columns, int rows) { m_columns = columns; m_rows = rows; }
	void setScrollSpeed(float speed) { m_scrollSpeed = speed; }
	
	float getHorizontalOffset() const { return m_horizontalOffset; }
	float getVerticalOffset() const { return m_verticalOffset; }
	void setHorizontalOffset(float offset) { m_horizontalOffset = offset; }
	void setVerticalOffset(float offset) { m_verticalOffset = offset; }
	
	void lockHorizontal() { m_horizontalLocked = true; }
	void unlockHorizontal() { m_horizontalLocked = false; }
	void lockVertical() { m_verticalLocked = true; }
	void unlockVertical() { m_verticalLocked = false; }
	bool isHorizontalLocked() const { return m_horizontalLocked; }
	bool isVerticalLocked() const { return m_verticalLocked; }
	
	void setUilo(UILO* uilo) override;

private:
	float m_cellWidth = 100.f;
	float m_cellHeight = 100.f;
	int m_columns = 0;  // 0 = infinite/unbounded
	int m_rows = 0;     // 0 = infinite/unbounded
	float m_horizontalOffset = 0.f;
	float m_verticalOffset = 0.f;
	float m_scrollSpeed = 10.f;
	bool m_horizontalLocked = false;
	bool m_verticalLocked = false;
};

// ---------------------------------------------------------------------------- //
// Free Column Container
// ---------------------------------------------------------------------------- //
class FreeColumn : public Column {
public:
	sf::Vector2f m_customPosition;

	using Column::Column;
	using Column::render;
	using Column::checkHover;

	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void update(sf::RectangleShape& parentBounds) override;
	void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override {}
	virtual EType getType() const override;
	void setPosition(sf::Vector2f pos);
	sf::Vector2f getPosition() const;
	sf::Vector2f getSize() const;
	sf::Vector2f getCenter() const;
	sf::FloatRect getBounds() const;
	void show() { m_modifier.setVisible(true); }
	void hide() { m_modifier.setVisible(false); }
};

// ---------------------------------------------------------------------------- //
// Text Element
// ---------------------------------------------------------------------------- //
class Text : public Element {
public:
	Text(Modifier modifier = default_mod, const std::string& str = "", sf::Font font = sf::Font(), const std::string& name = "");
	Text(Modifier modifier = default_mod, const std::string& str = "", const std::string& fontPath = "", const std::string& name = "");
	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;

	void setString(const std::string& newStr);
	std::string getString() const { return m_text ? std::string(m_text->getString()) : m_string; }
	float getTextWidth() const { return m_text ? m_text->getLocalBounds().size.x : 0.f; }
	float getTextHeight() const { return m_text ? m_text->getLocalBounds().size.y : 0.f; }
	
	// Get the exact position where a character would be positioned (for cursor placement)
	sf::Vector2f getCharacterPosition(size_t index) const {
		if (!m_text || index > m_string.length()) {
			return {0.f, 0.f};
		}
		// SFML's findCharacterPos gives us the exact position of any character (relative to text)
		return m_text->findCharacterPos(index);
	}
	
	// Get absolute screen coordinates for character positions (used for click detection)
	sf::Vector2f getAbsoluteCharacterPosition(size_t index) const {
		if (!m_text || index > m_string.length()) {
			return {0.f, 0.f};
		}
		// SFML's findCharacterPos gives us the position relative to the text object
		// We need to add the text's actual position to get screen coordinates
		sf::Vector2f relativePos = m_text->findCharacterPos(index);
		sf::Vector2f textPos = m_text->getPosition();
		return sf::Vector2f(relativePos.x + textPos.x, relativePos.y + textPos.y);
	}
	
	// Get the rendered position of the text (where it actually appears on screen)
	sf::Vector2f getRenderedPosition() const {
		if (!m_text) return {0.f, 0.f};
		return m_text->getPosition();
	}

	void setUilo(UILO* uilo) override;

private:
	std::string m_string = "";
	sf::Font m_font;
	std::optional<sf::Text> m_text;
};

// ---------------------------------------------------------------------------- //
// Spacer Element
// ---------------------------------------------------------------------------- //
class Spacer : public Element {
public:
	Spacer(Modifier& modifier, const std::string& name = "");
	void update(sf::RectangleShape& parentBounds) override;
	void setUilo(UILO* uilo) override;
};

// ---------------------------------------------------------------------------- //
// Button Element
// ---------------------------------------------------------------------------- //
class Button : public Element {
public:
	Button(
		Modifier modifier = default_mod,
		ButtonStyle buttonStyle = ButtonStyle::Default,
		const std::string& buttonText = "",
		const std::string& textFont = "",
		sf::Color textColor = sf::Color::White,
		const std::string& name = ""
	);

	Button(
		Modifier modifier,
		ButtonStyle buttonStyle,
		const std::string& buttonText,
		sf::Font& font,
		sf::Color textColor = sf::Color::White,
		const std::string& name = ""
	);

	void update(sf::RectangleShape& parentBounds) override;
	void render (sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;

	void setText(const std::string& newStr);
	std::string getText() const;

	bool isClicked() const { return m_isClicked; }
	bool isHovered() const { return m_isHovered; }

	void setClicked(bool clicked) { m_isClicked = clicked; }
	
	void setUilo(UILO* uilo) override;

private:
	ButtonStyle m_buttonStyle = ButtonStyle::Default;

	sf::RectangleShape m_bodyRect;
	sf::CircleShape m_leftCircle;
	sf::CircleShape m_rightCircle;

	std::unique_ptr<Text> m_text;
	std::unique_ptr<Row> m_textRow;

	bool m_isClicked = false;
	bool m_isHovered = false;
};

// ---------------------------------------------------------------------------- //
// Slider Element
// ---------------------------------------------------------------------------- //
class Slider : public Element {
public:
	Slider(
		Modifier modifier = default_mod,
		sf::Color knobColor = sf::Color::White,
		sf::Color barColor = sf::Color::Black,
		SliderOrientation orientation = SliderOrientation::Vertical,
		float initialValue = 0.75f,
		const std::string& name = ""
	);

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	virtual bool handleDrag(const sf::Vector2f& pos);

	float getValue() const;
	void setValue(float newVal);
	void setQuantization(int steps) { m_quantizationSteps = steps; }
	
	void setUilo(UILO* uilo) override;

	bool m_isDragging = false;

private:
	float m_minVal = 0.f;
	float m_maxVal = 1.f;
	float m_curVal = 0.75f;
	float m_initVal = 0.75f;
	int m_quantizationSteps = 0; // 0 = no quantization

	sf::Color m_knobColor = sf::Color::White;
	sf::Color m_barColor = sf::Color::Black;
	SliderOrientation m_orientation = SliderOrientation::Vertical;

	sf::RectangleShape m_knobRect;
	sf::RectangleShape m_barRect;

	sf::Clock doubleClickTimer;
};

// ---------------------------------------------------------------------------- //
// Knob Element
// ---------------------------------------------------------------------------- //
class Knob : public Element {
public:
	Knob(
		Modifier modifier = default_mod,
		sf::Color knobColor = sf::Color::White,
		sf::Color trackColor = sf::Color(100, 100, 100),
		sf::Color arcColor = sf::Color(0, 150, 255),
		float initialValue = 0.5f,
		const std::string& name = ""
	);

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	bool handleDrag(const sf::Vector2f& pos);

	float getValue() const;
	void setValue(float newVal);
	void setQuantization(int steps) { m_quantizationSteps = steps; }
	
	void setUilo(UILO* uilo) override;

	bool m_isDragging = false;

private:
	float m_minVal = 0.f;
	float m_maxVal = 1.f;
	float m_curVal = 0.5f;
	float m_initVal = 0.5f;
	int m_quantizationSteps = 0; // 0 = no quantization

	sf::Color m_knobColor = sf::Color::White;
	sf::Color m_trackColor = sf::Color(100, 100, 100);
	sf::Color m_arcColor = sf::Color(0, 150, 255);

	sf::Vector2f m_center;
	float m_radius = 0.f;
	
	// Delta-based dragging
	sf::Vector2f m_lastMousePos;
	
	sf::Clock doubleClickTimer;
};

// ---------------------------------------------------------------------------- //
// Dropdown Element
// ---------------------------------------------------------------------------- //
class Dropdown : public Element {
public:
	Dropdown(
		Modifier modifier = default_mod,
		const std::string& defaultText = "",
		const std::vector<std::string>& options = {},
		const std::string& textFont = "",
		sf::Color textColor = sf::Color::White,
		sf::Color optionBackgroundColor = sf::Color(50, 50, 50),
		const std::string& name = ""
	);
	~Dropdown();

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;
	inline void setSelected(const std::string& option) { m_selectedOption = option; }

	std::string getSelected() const;
	
	void setUilo(UILO* uilo) override;

	static Dropdown* s_openDropdown;

	friend class Page;

private:
	Button* m_mainButton;
	FreeColumn* m_optionsColumn;
	bool m_isOpen = false;
	std::string m_selectedOption;
};
// Static pointer to currently open dropdown
inline Dropdown* Dropdown::s_openDropdown = nullptr;

// ---------------------------------------------------------------------------- //
// Textbox Element
// ---------------------------------------------------------------------------- //
class TextBox : public Element{
public:
	TextBox(
		Modifier modifier = default_mod,
		TBStyle style = TBStyle::Default,
		const std::string& fontPath = "",
		const std::string& defaultText = "",
		sf::Color textColor = sf::Color::Black,
		sf::Color activeOutlineColor = sf::Color::Transparent,
		const std::string& name = ""
	);

	TextBox(
		Modifier modifier,
		TBStyle style,
		sf::Font& font,
		const std::string& defaultText = "",
		sf::Color textColor = sf::Color::Black,
		sf::Color activeOutlineColor = sf::Color::Transparent,
		const std::string& name = ""
	);

	~TextBox();

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;

	inline bool isActive() const { return m_isActive; }
	inline void setActive(bool active) { 
		m_isActive = active; 
		if (active) {
			m_cursorPosition = m_currentText.length(); // Place cursor at end by default
			m_showCursor = true;
			m_cursorClock.restart();
		}
	}
	
	inline std::string getText() const { return m_currentText; }
	inline void setText(const std::string& text) { 
		m_currentText = text;
		// Reset cursor when text changes
		m_cursorPosition = m_currentText.length(); // Place cursor at end
		m_showCursor = true;
		m_cursorClock.restart();
		if (m_text) {
			m_text->setString(m_currentText.empty() ? m_defaultText : m_currentText);
		}
	}
	inline void clearText() { setText(""); }
	
	inline void setPlaceholder(const std::string& placeholder) {
		m_defaultText = placeholder;
		if (m_text && m_currentText.empty()) {
			m_text->setString(m_defaultText);
		}
	}

	// Cursor control methods
	inline size_t getCursorPosition() const { return m_cursorPosition; }
	inline void setCursorPosition(size_t pos) { 
		m_cursorPosition = std::min(pos, m_currentText.length()); 
		m_showCursor = true;
		m_cursorClock.restart();
	}
	inline void moveCursorLeft() { 
		if (m_cursorPosition > 0) {
			m_cursorPosition--; 
			m_showCursor = true;
			m_cursorClock.restart();
		}
	}
	inline void moveCursorRight() { 
		if (m_cursorPosition < m_currentText.length()) {
			m_cursorPosition++; 
			m_showCursor = true;
			m_cursorClock.restart();
		}
	}
	inline void insertAtCursor(char c) {
		size_t insertPos = std::min(m_cursorPosition, m_currentText.length());
		m_currentText.insert(insertPos, 1, c);
		m_cursorPosition = insertPos + 1;
		if (m_text) {
			m_text->setString(m_currentText.empty() ? m_defaultText : m_currentText);
		}
		m_showCursor = true;
		m_cursorClock.restart();
	}
	inline void deleteAtCursor() {
		if (!m_currentText.empty() && m_cursorPosition < m_currentText.length()) {
			m_currentText.erase(m_cursorPosition, 1);
			if (m_text) {
				m_text->setString(m_currentText.empty() ? m_defaultText : m_currentText);
			}
			m_showCursor = true;
			m_cursorClock.restart();
		}
	}
	inline void backspaceAtCursor() {
		if (!m_currentText.empty() && m_cursorPosition > 0) {
			size_t deletePos = m_cursorPosition - 1;
			m_currentText.erase(deletePos, 1);
			m_cursorPosition = deletePos;
			if (m_text) {
				m_text->setString(m_currentText.empty() ? m_defaultText : m_currentText);
			}
			m_showCursor = true;
			m_cursorClock.restart();
		}
	}

	// Public cursor position for external access
	size_t m_cursorPosition = 0; // Character index for cursor position
	void setUilo(UILO* uilo) override;

private:
	TBStyle m_style = TBStyle::Default;
	sf::RectangleShape m_bodyRect;

	// if hasStyle(TBStyle::Pill)
	sf::CircleShape m_leftCircle;
	sf::CircleShape m_rightCircle;

	// Cursor rectangle for visual cursor
	sf::RectangleShape m_cursorRect;

	std::unique_ptr<Text> m_text;
	std::unique_ptr<Row> m_textRow;

	// if hasStyle(TBStyle::Wrap)
	std::unique_ptr<ScrollableColumn> m_textColumn;

	std::string m_defaultText = "Default Text";
	std::string m_currentText = "";
	sf::Color m_textColor = sf::Color::Black;

	bool m_isActive = false;
	
	// Cursor functionality
	bool m_showCursor = false;
	mutable sf::Clock m_cursorClock;
	static constexpr float CURSOR_BLINK_INTERVAL = 0.5f; // 500ms

	// Helper method to render cursor at correct position
	void renderCursor(sf::RenderTarget& target) {
		if (!m_text) return;
		
		// Calculate cursor position
		float cursorX, cursorY;
		float cursorHeight = m_bounds.getSize().y * 0.6f; // 60% of textbox height
		float cursorWidth = 2.0f; // 2 pixels wide
		
		if (m_currentText.empty()) {
			// When no text, position cursor at the center of where text would appear
			sf::Vector2f textElementPos = m_text->m_bounds.getPosition();
			sf::Vector2f textElementSize = m_text->m_bounds.getSize();
			if (hasStyle(m_style, TBStyle::CenterText)) {
				cursorX = textElementPos.x + textElementSize.x / 2.0f;
			} else {
				cursorX = textElementPos.x;
			}
		} else {
			// Use SFML's precise character positioning
			// Clamp cursor position to valid range
			size_t safePos = std::min(m_cursorPosition, m_currentText.length());
			
			if (hasStyle(m_style, TBStyle::CenterText)) {
				// For centered text, we need to calculate the cursor position manually
				// since the text alignment affects where characters are positioned
				sf::Vector2f textElementPos = m_text->m_bounds.getPosition();
				sf::Vector2f textElementSize = m_text->m_bounds.getSize();
				
				// Get the actual displayed text for width calculations
				std::string displayText = hasStyle(m_style, TBStyle::Password) ? 
					std::string(m_currentText.length(), '*') : m_currentText;
				
				// Calculate text width up to cursor position
				std::string textToCursor = displayText.substr(0, safePos);
				float textToCursorWidth = 0.f;
				float totalTextWidth = m_text->getTextWidth();
				
				// If we have partial text, calculate its width proportionally
				if (!displayText.empty() && !textToCursor.empty()) {
					textToCursorWidth = totalTextWidth * (float(textToCursor.length()) / float(displayText.length()));
				}
				
				// Calculate centered text start position
				float textStartX = textElementPos.x + (textElementSize.x - totalTextWidth) / 2.0f;
				cursorX = textStartX + textToCursorWidth;
			} else {
				sf::Vector2f charPos = m_text->getCharacterPosition(safePos);
				cursorX = charPos.x;
			}
		}
		
		// Center cursor vertically in the TextBox
		cursorY = m_bounds.getPosition().y + (m_bounds.getSize().y - cursorHeight) * 0.5f;
		
		// Set cursor rectangle properties
		m_cursorRect.setPosition({cursorX, cursorY});
		m_cursorRect.setSize({cursorWidth, cursorHeight});
		m_cursorRect.setFillColor(m_textColor); // Same color as text
		
		// Draw the cursor
		target.draw(m_cursorRect);
	}

public:
	static TextBox* s_activeTextBox;
};
// Static pointer to currently active textbox
inline TextBox* TextBox::s_activeTextBox = nullptr;

// ---------------------------------------------------------------------------- //
// Image Element
// ---------------------------------------------------------------------------- //
static sf::Image default_image;
class Image : public Element {
public:
	Image(
		Modifier modifier = default_mod,
		sf::Image& image = default_image,
		const bool recolor = false,
		const std::string& name = ""
	);

	void setImage(sf::Image& image = default_image, bool recolor = false);
	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;

private:
	std::unique_ptr<sf::Image> m_image;
	std::unique_ptr<sf::Texture> m_tex;
};

// ---------------------------------------------------------------------------- //
// Page View
// ---------------------------------------------------------------------------- //
class Page {
public:
	Page() = default;

	Page(std::initializer_list<Container*> containers = {});
	~Page() = default;

	void update(const sf::RectangleShape& parentBounds);
	void render(sf::RenderTarget& target);
	void handleEvent(const sf::Event& event);
	bool dispatchClick(const sf::Vector2f& pos, sf::Mouse::Button button);
	void dispatchScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta);
	void dispatchHover(const sf::Vector2f& pos);
	void clear();
	void setUilo(UILO* uilo);

private:
	std::vector<Container*> m_containers;
	sf::RectangleShape m_bounds;
	UILO* m_uilo = nullptr;
};

// ---------------------------------------------------------------------------- //
// UILO Application Core
// ---------------------------------------------------------------------------- //
class UILO {
	// Friend declarations for all UI elements
	friend class Element;
	friend class Container;
	friend class Row;
	friend class ScrollableRow;
	friend class Column;
	friend class ScrollableColumn;
	friend class Grid;
	friend class FreeColumn;
	friend class Text;
	friend class Spacer;
	friend class Button;
	friend class Slider;
	friend class Knob;
	friend class TextBox;
	friend class Image;
	friend class Dropdown;
	friend class Page;
	friend void cleanupMarkedElements();
	
public:
	Slider* m_activeDragSlider = nullptr;
	Knob* m_activeDragKnob = nullptr;
	Page* m_currentPage = nullptr;

	UILO();
	UILO(const std::string& windowTitle);
	UILO(sf::RenderWindow& userWindow, sf::View& windowView);
	~UILO();

	void update();
	void update(sf::View& windowView);
	void render();
	void setTitle(const std::string& newTitle);
	bool isRunning() const;
	bool windowShouldUpdate() const;
	void addPage(std::unique_ptr<Page> page, const std::string& name);
	void switchToPage(const std::string& pageName);
	void forceUpdate();
	void forceUpdate(sf::View& windowView);
	void setScale(float scale = 1.f);
	float getScale() const;
	sf::Vector2f getMousePosition() const;
	float getVerticalScrollDelta() const;
	float getHorizontalScrollDelta() const;
	void resetScrollDeltas();

	void setInputBlocked(bool blocked);
	bool isInputBlocked() const;
	bool isMouseDragging() const;
	inline void setFullClean(bool doFullClean) { m_fullClean = doFullClean; }
	
	void setCursor(CursorType cursorType);
	CursorType getCurrentCursor() const;
	void resetCursor();

	Row* getRow(const std::string& name);
	Column* getColumn(const std::string& name);
	Spacer* getSpacer(const std::string& name);
	Button* getButton(const std::string& name);
	Text* getText(const std::string& name);
	Slider* getSlider(const std::string& name);
	Dropdown* getDropdown(const std::string& name);
	Grid* getGrid(const std::string& name);
	TextBox* getTextBox(const std::string& name);

private:
	sf::RenderWindow m_window;
	sf::RenderWindow* m_userWindow = nullptr;
	bool m_windowOwned = true;
	int m_pollCount = 10;
	float m_renderScale = 1.f;
	sf::VideoMode m_defScreenRes;
	sf::View m_defaultView;
	sf::RectangleShape m_bounds;
	std::string m_windowTitle = "";

	std::unordered_map<std::string, Page*> m_pages;

	bool m_running = false;
	bool m_shouldUpdate = true;
	bool m_mouseDragging = false;
	bool m_inputBlocked = false;
	bool m_fullClean = false;

	sf::Vector2u m_lastWindowSize;
	std::optional<std::pair<sf::Vector2f, sf::Mouse::Button>> m_clickInfo;
	std::optional<sf::Vector2f> m_scrollPosition;
	sf::Vector2f m_mousePos;
	float m_verticalScrollDelta = 0.f;
	float m_horizontalScrollDelta = 0.f;

	std::vector<std::unique_ptr<Page>> m_ownedPages;

	std::vector<sf::RectangleShape> m_elementBoundsCache;
	bool m_cacheIsInitialized = false;

	std::unordered_map<std::string, Slider*> m_sliders;
	std::unordered_map<std::string, Container*> m_containers;
	std::unordered_map<std::string, Text*> m_texts;
	std::unordered_map<std::string, Spacer*> m_spacers;
	std::unordered_map<std::string, Button*> m_buttons;
	std::unordered_map<std::string, Dropdown*> m_dropdowns;
	std::unordered_map<std::string, Grid*> m_grids;
	std::unordered_map<std::string, TextBox*> m_textboxes;
	
	// Cursor management
	CursorType m_currentCursorType = CursorType::Arrow;
	std::optional<sf::Cursor> m_arrowCursor;
	std::optional<sf::Cursor> m_handCursor;
	std::optional<sf::Cursor> m_iBeamCursor;
	std::optional<sf::Cursor> m_sizeHorizontalCursor;
	std::optional<sf::Cursor> m_sizeVerticalCursor;
	std::optional<sf::Cursor> m_sizeNWSECursor;
	std::optional<sf::Cursor> m_sizeNESWCursor;
	std::optional<sf::Cursor> m_sizeAllCursor;
	std::optional<sf::Cursor> m_crossCursor;
	std::optional<sf::Cursor> m_notAllowedCursor;

	void pollEvents();
	void initDefaultView();
	void setView(const sf::View& view);
	void _internal_update(sf::RenderWindow& target, sf::View& view);
	void initCursors(); // Initialize all cursor types
};

// ---------------------------------------------------------------------------- //
// Modifier Implementation
// ---------------------------------------------------------------------------- //
inline Modifier& Modifier::setWidth(float pct) { m_widthPct = pct; m_fixedWidth = 0.f; return *this; }
inline Modifier& Modifier::setHeight(float pct) { m_heightPct = pct; m_fixedHeight = 0.f; return *this; }
inline Modifier& Modifier::setfixedWidth(float px) { m_fixedWidth = px; return *this; }
inline Modifier& Modifier::setfixedHeight(float px) { m_fixedHeight = px; return *this; }
inline Modifier& Modifier::align(Align alignment) { m_alignment = alignment; return *this; }
inline Modifier& Modifier::setColor(sf::Color color) { m_color = color; return *this; }
inline Modifier& Modifier::onLClick(funcPtr cb) { m_onLClick = std::move(cb); return *this; }
inline Modifier& Modifier::onRClick(funcPtr cb) { m_onRClick = std::move(cb); return *this; }
inline Modifier& Modifier::setVisible(bool visible) { m_isVisible = visible; return *this; }
inline Modifier& Modifier::setHighPriority(bool highPriority) { m_highPriority = highPriority; return *this; }
inline Modifier& Modifier::setRounded(float radius) { m_rounded = radius; return *this; }
inline Modifier& Modifier::setPadding(float padding) { m_padding = padding; return *this; }
inline Modifier& Modifier::fitContentWidth(bool fit) { m_fitContentWidth = fit; return *this; }
inline Modifier& Modifier::fitContentHeight(bool fit) { m_fitContentHeight = fit; return *this; }

inline float Modifier::getWidth() const { return m_widthPct; }
inline float Modifier::getHeight() const { return m_heightPct; }
inline float Modifier::getFixedWidth() const { return m_fixedWidth; }
inline float Modifier::getFixedHeight() const { return m_fixedHeight; }
inline Align Modifier::getAlignment() const { return m_alignment; }
inline sf::Color Modifier::getColor() const { return m_color; }
inline const funcPtr& Modifier::getOnLClick() const { return m_onLClick; }
inline const funcPtr& Modifier::getOnRClick() const { return m_onRClick; }
inline bool Modifier::isVisible() const { return m_isVisible; }
inline bool Modifier::isHighPriority() const { return m_highPriority; }
inline float Modifier::getRounded() const { return m_rounded; }
inline float Modifier::getPadding() const { return m_padding; }
inline bool Modifier::getFitContentWidth() const { return m_fitContentWidth; }
inline bool Modifier::getFitContentHeight() const { return m_fitContentHeight; }

// ---------------------------------------------------------------------------- //
// Element Implementation
// ---------------------------------------------------------------------------- //
inline Element::Element() {}

inline Element::~Element() {}

inline void Element::update(sf::RectangleShape& parentBounds) {
	m_isDirty = (m_bounds.getPosition() != m_pastBounds.getPosition() || m_bounds.getSize() != m_pastBounds.getSize());
	if (m_isDirty) {
		m_pastBounds.setPosition(m_bounds.getPosition());
		m_pastBounds.setSize(m_bounds.getSize());
	}
}

inline void Element::updateChildren() {}

inline void Element::handleEvent(const sf::Event& event) {
	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
		sf::Vector2f mousePos(mousePressed->position);
		if (m_bounds.getGlobalBounds().contains(mousePos)) {
			if (mousePressed->button == sf::Mouse::Button::Left) {
				if (m_modifier.getOnLClick()) m_modifier.getOnLClick()();
			} else if (mousePressed->button == sf::Mouse::Button::Right) {
				if (m_modifier.getOnRClick()) m_modifier.getOnRClick()();
			}
		}
	}
}

inline bool Element::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (m_bounds.getGlobalBounds().contains(pos)) {
		if (button == sf::Mouse::Button::Left) {
			if (m_modifier.getOnLClick()) m_modifier.getOnLClick()();
			return true;
		} else if (button == sf::Mouse::Button::Right) {
			if (m_modifier.getOnRClick()) m_modifier.getOnRClick()();
			return true;
		}
	}

	return false;
}

inline void Element::checkHover(const sf::Vector2f& pos) {
	m_isHovered = m_bounds.getGlobalBounds().contains(pos);
}

inline void Element::setModifier(const Modifier& modifier) { m_modifier = modifier; }

inline EType Element::getType() const { return EType::Element; }

inline void Element::resize(const sf::RectangleShape& parent, const bool inSlot) {
	const float padding = m_modifier.getPadding();
	const float doublePadding = padding * 2.f;
	
	if (m_modifier.getFixedWidth() != 0) {
		m_bounds.setSize({
			m_modifier.getFixedWidth() - doublePadding,
			m_bounds.getSize().y
		});
	}
	else {
		if (inSlot)
			m_bounds.setSize({
				parent.getSize().x - doublePadding,
				m_bounds.getSize().y
			});
		else
			m_bounds.setSize({
				m_modifier.getWidth() * parent.getSize().x - doublePadding,
				m_bounds.getSize().y
			});
	}

	if (m_modifier.getFixedHeight() != 0) {
		m_bounds.setSize({
			m_bounds.getSize().x,
			m_modifier.getFixedHeight() - doublePadding
		});
	}
	else {
		if (inSlot)
			m_bounds.setSize({
				m_bounds.getSize().x,
				parent.getSize().y - doublePadding
			});
		else
			m_bounds.setSize({
				m_bounds.getSize().x,
				m_modifier.getHeight() * parent.getSize().y - doublePadding
			});
	}
}

inline void Element::applyModifiers() { m_bounds.setFillColor(m_modifier.getColor()); }

inline void Element::setCustomGeometry(std::vector<std::shared_ptr<sf::Drawable>>& customGeometry) {
	m_customGeometry.clear();
	m_customGeometry = customGeometry;
}

// ---------------------------------------------------------------------------- //
// Container Implementation
// ---------------------------------------------------------------------------- //
inline Container::Container(std::initializer_list<Element*> elements, const std::string& name) {
	for (auto& e : elements)
		m_elements.push_back(e);

	m_name = name;

	if (!m_name.empty() && m_uilo)
		m_uilo->m_containers[m_name] = this;
}

inline Container::Container(Modifier modifier, std::initializer_list<Element*> elements, const std::string& name) {
	m_modifier = modifier;

	for (auto& e : elements)
		m_elements.push_back(e);

	m_name = name;

	if (!m_name.empty() && m_uilo)
		m_uilo->m_containers[m_name] = this;
}

inline Container::~Container() {}

inline void Container::addElement(Element* element) { 
	m_elements.push_back(element); 
	if (m_uilo) {
		element->setUilo(m_uilo);
	}
}

inline void Container::addElements(std::initializer_list<Element*> elements) {
	for (auto& e : elements) {
		m_elements.push_back(e);
		if (m_uilo) {
			e->setUilo(m_uilo);
		}
	}
}

inline void Container::handleEvent(const sf::Event& event) {
	for (auto& e : m_elements)
		e->handleEvent(event);
	Element::handleEvent(event);
}

inline const std::vector<Element*>& Container::getElements() const { return m_elements; }

inline void Container::updateChildren() {
	for (auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			e->Element::update(e->m_bounds);
			e->updateChildren();
		}
	}
}

inline void Container::clear() {
	for (auto& e : m_elements) {
		if (Container* childContainer = dynamic_cast<Container*>(e)) {
			childContainer->clear();
		}
		e->m_markedForDeletion = true;
	}

	m_elements.clear();
	cleanupMarkedElements();
}

inline void Container::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	
	// Register this container if it has a name
	if (!m_name.empty() && m_uilo)
		m_uilo->m_containers[m_name] = this;
	
	for (auto& element : m_elements) {
		element->setUilo(uilo);
	}
}

inline void cleanupMarkedElements() {
	auto it = uilo_owned_elements.begin();
	while (it != uilo_owned_elements.end()) {
		Element* element = it->get();
		if (element->m_markedForDeletion) {
			if (!element->m_name.empty() && element->m_uilo) {
				UILO* uilo = element->m_uilo;
				
				auto buttonIt = uilo->m_buttons.find(element->m_name);
				if (buttonIt != uilo->m_buttons.end() && buttonIt->second == element)
					uilo->m_buttons.erase(buttonIt);

				auto sliderIt = uilo->m_sliders.find(element->m_name);
				if (sliderIt != uilo->m_sliders.end() && sliderIt->second == element)
					uilo->m_sliders.erase(sliderIt);

				auto textIt = uilo->m_texts.find(element->m_name);
				if (textIt != uilo->m_texts.end() && textIt->second == element)
					uilo->m_texts.erase(textIt);

				auto spacerIt = uilo->m_spacers.find(element->m_name);
				if (spacerIt != uilo->m_spacers.end() && spacerIt->second == element)
					uilo->m_spacers.erase(spacerIt);

				auto containerIt = uilo->m_containers.find(element->m_name);
				if (containerIt != uilo->m_containers.end() && containerIt->second == element)
					uilo->m_containers.erase(containerIt);

				auto gridIt = uilo->m_grids.find(element->m_name);
				if (gridIt != uilo->m_grids.end() && gridIt->second == element)
					uilo->m_grids.erase(gridIt);
					
				auto dropdownIt = uilo->m_dropdowns.find(element->m_name);
				if (dropdownIt != uilo->m_dropdowns.end() && dropdownIt->second == element)
					uilo->m_dropdowns.erase(dropdownIt);
			}
			it = uilo_owned_elements.erase(it);
		} else {
			++it;
		}
	}
}

// ---------------------------------------------------------------------------- //
// Row Implementation
// ---------------------------------------------------------------------------- //
inline void Row::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	
	// If fitContentWidth is enabled, calculate total width of children and resize
	if (m_modifier.getFitContentWidth()) {
		float totalWidth = 0.f;
		const float padding = m_modifier.getPadding();
		for (const auto& e : m_elements) {
			if (e->m_modifier.isVisible()) {
				const float fixedWidth = e->m_modifier.getFixedWidth();
				const float elemPadding = e->m_modifier.getPadding();
				totalWidth += (fixedWidth > 0.f ? fixedWidth : 0.f) + elemPadding * 2.f;
			}
		}
		m_bounds.setSize({totalWidth + padding * 2.f, m_bounds.getSize().y});
	}
	
	applyModifiers();
	Element::update(parentBounds);

	const float padding = m_modifier.getPadding();
	const float doublePadding = padding * 2.f;

	// Pre-calculate totals with single pass
	float totalPercent = 0.f, totalFixed = 0.f;
	size_t visibleCount = 0;
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			++visibleCount;
			const float fixedWidth = e->m_modifier.getFixedWidth();
			if (fixedWidth > 0.f) {
				totalFixed += fixedWidth;
			} else {
				totalPercent += e->m_modifier.getWidth();
			}
		}
	}
	
	if (visibleCount == 0) return; // Early exit if no visible elements
	
	const float remainingSpace = m_bounds.getSize().x - totalFixed;
	const float percentScale = (totalPercent <= 0.f) ? 1.f : (1.f / totalPercent);
	const sf::Vector2f boundsSize = m_bounds.getSize();
	
	// Update all elements in single pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const float fixedWidth = e->m_modifier.getFixedWidth();
			const float width = (fixedWidth > 0.f) ? fixedWidth : (e->m_modifier.getWidth() * percentScale * remainingSpace);
			const sf::RectangleShape slot({width, boundsSize.y});
			e->update(const_cast<sf::RectangleShape&>(slot));
		}
	}

	// Categorize elements by alignment in single pass
	std::vector<Element*> left, center, right;
	left.reserve(visibleCount);
	center.reserve(visibleCount);
	right.reserve(visibleCount);
	
	float leftWidth = 0.f, centerWidth = 0.f, rightWidth = 0.f;

	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			const float elementWidth = e->m_bounds.getSize().x;
			const float elemPadding = e->m_modifier.getPadding();
			
			if (hasAlign(alignment, Align::RIGHT)) {
				right.push_back(e);
				rightWidth += elementWidth + elemPadding * 2.f;
			} else if (hasAlign(alignment, Align::CENTER_X)) {
				center.push_back(e);
				centerWidth += elementWidth + elemPadding * 2.f;
			} else {
				left.push_back(e);
				leftWidth += elementWidth + elemPadding * 2.f;
			}
		}
	}

	// Position elements efficiently
	const sf::Vector2f boundsPos = m_bounds.getPosition();
	const float boundsWidth = boundsSize.x;
	
	// Left aligned elements
	float xPos = boundsPos.x;
	for (const auto& e : left) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({xPos + elemPadding, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x + elemPadding * 2.f;
	}

	// Center aligned elements
	xPos = boundsPos.x + (boundsWidth - centerWidth) * 0.5f;
	for (const auto& e : center) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({xPos + elemPadding, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x + elemPadding * 2.f;
	}

	// Right aligned elements
	xPos = boundsPos.x + boundsWidth - rightWidth;
	for (const auto& e : right) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({xPos + elemPadding, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x + elemPadding * 2.f;
	}

	// Apply vertical alignment in final pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			const float elemPadding = e->m_modifier.getPadding();
			sf::Vector2f pos = e->m_bounds.getPosition();

			if (hasAlign(alignment, Align::CENTER_Y))
				pos.y = boundsPos.y + elemPadding + (boundsSize.y - e->m_bounds.getSize().y - elemPadding * 2.f) * 0.5f;
			else if (hasAlign(alignment, Align::BOTTOM))
				pos.y = boundsPos.y + boundsSize.y - e->m_bounds.getSize().y - elemPadding;
			else
				pos.y = boundsPos.y + elemPadding;

			e->m_bounds.setPosition(pos);
		}
	}
}

inline void Row::render(sf::RenderTarget& target) {
	// Render background with optional rounded corners
	if (m_modifier.getRounded() > 0.f) {
		auto rounded = createRoundedRect(m_bounds.getSize(), m_modifier.getRounded(), m_bounds.getFillColor());
		rounded.setPosition(m_bounds.getPosition());
		target.draw(rounded);
	} else {
		target.draw(m_bounds);
	}

	// Render normal priority elements first
	for (auto& e : m_elements)
		if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
			e->render(target);

	// Render custom geometry
	sf::RenderStates states;
	states.transform.translate(m_bounds.getPosition());
	for (auto& d : m_customGeometry) {
		target.draw(*d, states);
	}

	// Then render high priority elements on top
	for (auto& e : m_elements)
		if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
			e->render(target);
}

inline bool Row::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	bool childClicked = false;
	for (auto& e : m_elements)
		if (e) {
			if (e->m_modifier.isVisible() && e->m_bounds.getGlobalBounds().contains(pos))
				childClicked = e->checkClick(pos, button);
			if (childClicked) return childClicked;
		}
	return Element::checkClick(pos, button);
}

inline void Row::checkHover(const sf::Vector2f& pos) {
	Element::checkHover(pos);
	for (auto& e : m_elements)
		if (e && e->m_bounds.getGlobalBounds().contains(pos)) e->checkHover(pos);
}

inline EType Row::getType() const { return EType::Row; }

inline void Row::applyVerticalAlignment(Element* e, const sf::RectangleShape& parentBounds) {
	auto align = e->m_modifier.getAlignment();
	auto pos = e->m_bounds.getPosition();

	if (hasAlign(align, Align::CENTER_Y))
		pos.y = parentBounds.getPosition().y + (parentBounds.getSize().y - e->m_bounds.getSize().y) / 2.f;
	else if (hasAlign(align, Align::BOTTOM))
		pos.y = parentBounds.getPosition().y + parentBounds.getSize().y - e->m_bounds.getSize().y;

	e->m_bounds.setPosition(pos);
}

// ---------------------------------------------------------------------------- //
// Scrollable Row Implementation
// ---------------------------------------------------------------------------- //
inline void ScrollableRow::render(sf::RenderTarget& target) {
	// Only ScrollableRow gets this clipped render
	if (getType() == EType::ScrollableRow) {
		// Use scissor test for clipping instead of views to avoid coordinate space issues
		sf::FloatRect clipRect = m_bounds.getGlobalBounds();
		sf::View currentView = target.getView();
		
		// Convert world coordinates to pixel coordinates for scissor rect
		sf::Vector2f topLeft = {clipRect.position.x, clipRect.position.y};
		sf::Vector2f bottomRight = {clipRect.position.x + clipRect.size.x, 
		                             clipRect.position.y + clipRect.size.y};
		
		sf::Vector2i topLeftPixel = target.mapCoordsToPixel(topLeft, currentView);
		sf::Vector2i bottomRightPixel = target.mapCoordsToPixel(bottomRight, currentView);
		
		// SFML's scissor coordinates are from bottom-left, need to flip Y
		sf::Vector2u windowSize = target.getSize();
		int scissorX = topLeftPixel.x;
		int scissorY = windowSize.y - bottomRightPixel.y;  // Flip Y
		int scissorWidth = bottomRightPixel.x - topLeftPixel.x;
		int scissorHeight = bottomRightPixel.y - topLeftPixel.y;
		
		// Save current scissor state
		GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
		GLint oldScissor[4];
		if (scissorWasEnabled) {
			glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
			
			// Intersect with parent's scissor region
			int parentX = oldScissor[0];
			int parentY = oldScissor[1];
			int parentRight = parentX + oldScissor[2];
			int parentTop = parentY + oldScissor[3];
			
			int childRight = scissorX + scissorWidth;
			int childTop = scissorY + scissorHeight;
			
			// Calculate intersection
			scissorX = std::max(scissorX, parentX);
			scissorY = std::max(scissorY, parentY);
			int right = std::min(childRight, parentRight);
			int top = std::min(childTop, parentTop);
			
			scissorWidth = std::max(0, right - scissorX);
			scissorHeight = std::max(0, top - scissorY);
		}
		
		// Enable scissor test with new bounds
		glEnable(GL_SCISSOR_TEST);
		glScissor(scissorX, scissorY, scissorWidth, scissorHeight);

		// Render background with optional rounded corners
		if (m_modifier.getRounded() > 0.f) {
			auto rounded = createRoundedRect(m_bounds.getSize(), m_modifier.getRounded(), m_bounds.getFillColor());
			rounded.setPosition(m_bounds.getPosition());
			target.draw(rounded);
		} else {
			target.draw(m_bounds);
		}

		sf::RenderStates states;
		states.transform.translate(m_bounds.getPosition());

		// Render normal priority elements first (including their custom geometry, clipped)
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority()) {
				e->render(target);
			}

		// Render this row's custom geometry, clipped
		for (auto& d : m_customGeometry) {
			target.draw(*d, states);
		}

		// Then render high priority elements on top (including their custom geometry, clipped)
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority()) {
				e->render(target);
			}

		// Restore previous scissor state
		if (scissorWasEnabled) {
			glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	} else {
		// fallback to Row render (should never hit)
		Row::render(target);
	}
}

inline void ScrollableRow::update(sf::RectangleShape& parentBounds) {
	Row::update(parentBounds);

	if (!m_elements.empty()) {
		// Cache bounds for intersection tests
		const sf::FloatRect containerBounds = m_bounds.getGlobalBounds();
		const float offsetX = m_offset;
		
		// Apply offset and check visibility in single pass
		Element* firstElement = nullptr;
		Element* lastElement = nullptr;
		
		for (auto& e : m_elements) {
			if (e->m_modifier.isVisible()) {
				if (!firstElement) firstElement = e;
				lastElement = e;
				
				// Apply horizontal offset
				const sf::Vector2f currentPos = e->m_bounds.getPosition();
				e->m_bounds.setPosition({currentPos.x + offsetX, currentPos.y});
				
				// Only update visibility flag and children - elements are already properly sized by Row::update
				const sf::FloatRect elementBounds = e->m_bounds.getGlobalBounds();
				const bool isVisible = containerBounds.findIntersection(elementBounds).has_value();
				e->m_doRender = isVisible;
				
				// Update children if element is a container
				e->updateChildren();
			}
		}

		// Boundary checking with cached positions
		if (firstElement && lastElement) {
			const float containerLeft = m_bounds.getPosition().x;
			const float containerRight = m_bounds.getPosition().x + m_bounds.getSize().x;
			const float lastElementRight = lastElement->m_bounds.getPosition().x + lastElement->m_bounds.getSize().x;
			const float firstElementLeft = firstElement->m_bounds.getPosition().x;
			
			// First, ensure left alignment (priority)
			if (firstElementLeft >= containerLeft) {
				m_offset -= firstElementLeft - containerLeft;
			}
			// Then, if there's overflow and we've scrolled past the right, align to right
			else if (lastElementRight <= containerRight) {
				m_offset += containerRight - lastElementRight;
			}
		}
	}
}

inline void ScrollableRow::checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
	if (m_locked) return;
	
	if (m_bounds.getGlobalBounds().contains(pos)) {
		if (horizontalDelta < 0)
			m_offset -= m_scrollSpeed;
		else if (horizontalDelta > 0)
			m_offset += m_scrollSpeed;
		else if (verticalDelta != 0)
			for (auto& e : m_elements)
				e->checkScroll(pos, verticalDelta, horizontalDelta);
	}
}

inline EType ScrollableRow::getType() const {
	return EType::ScrollableRow;
}

// ---------------------------------------------------------------------------- //
// Column Implementation
// ---------------------------------------------------------------------------- //
inline void Column::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	
	// Calculate content height if fitContentHeight is enabled
	if (m_modifier.getFitContentHeight()) {
		float totalHeight = 0.f;
		const float padding = m_modifier.getPadding();
		
		for (const auto& e : m_elements) {
			if (e->m_modifier.isVisible()) {
				const float fixedHeight = e->m_modifier.getFixedHeight();
				const float elemPadding = e->m_modifier.getPadding();
				totalHeight += (fixedHeight > 0.f ? fixedHeight : 0.f) + elemPadding * 2.f;
			}
		}
		
		const float width = m_bounds.getSize().x;
		m_bounds.setSize({width, totalHeight + padding * 2.f});
	}
	
	applyModifiers();
	Element::update(parentBounds);

	const float padding = m_modifier.getPadding();
	const float doublePadding = padding * 2.f;

	// Pre-calculate totals with single pass
	float totalPercent = 0.f, totalFixed = 0.f;
	size_t visibleCount = 0;
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			++visibleCount;
			const float fixedHeight = e->m_modifier.getFixedHeight();
			if (fixedHeight > 0.f) {
				totalFixed += fixedHeight;
			} else {
				totalPercent += e->m_modifier.getHeight();
			}
		}
	}

	if (visibleCount == 0) return; // Early exit if no visible elements

	const float remainingSpace = m_bounds.getSize().y - totalFixed;
	const float percentScale = (totalPercent <= 0.f) ? 1.f : (1.f / totalPercent);
	const sf::Vector2f boundsSize = m_bounds.getSize();

	// Update all elements in single pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const float fixedHeight = e->m_modifier.getFixedHeight();
			const float height = (fixedHeight > 0.f) ? fixedHeight : (e->m_modifier.getHeight() * percentScale * remainingSpace);
			const sf::RectangleShape slot({boundsSize.x, height});
			e->update(const_cast<sf::RectangleShape&>(slot));
		}
	}

	// Categorize elements by alignment in single pass
	std::vector<Element*> top, center, bottom;
	top.reserve(visibleCount);
	center.reserve(visibleCount);
	bottom.reserve(visibleCount);
	
	float topHeight = 0.f, centerHeight = 0.f, bottomHeight = 0.f;

	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			const float elementHeight = e->m_bounds.getSize().y;
			const float elemPadding = e->m_modifier.getPadding();

			if (hasAlign(alignment, Align::BOTTOM)) {
				bottom.push_back(e);
				bottomHeight += elementHeight + elemPadding * 2.f;
			} else if (hasAlign(alignment, Align::CENTER_Y)) {
				center.push_back(e);
				centerHeight += elementHeight + elemPadding * 2.f;
			} else {
				top.push_back(e);
				topHeight += elementHeight + elemPadding * 2.f;
			}
		}
	}

	// Position elements efficiently
	const sf::Vector2f boundsPos = m_bounds.getPosition();
	const float boundsHeight = boundsSize.y;

	// Top aligned elements
	float yPos = boundsPos.y;
	for (const auto& e : top) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos + elemPadding});
		yPos += e->m_bounds.getSize().y + elemPadding * 2.f;
	}

	// Center aligned elements
	yPos = boundsPos.y + (boundsHeight - centerHeight) * 0.5f;
	for (const auto& e : center) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos + elemPadding});
		yPos += e->m_bounds.getSize().y + elemPadding * 2.f;
	}

	// Bottom aligned elements
	yPos = boundsPos.y + boundsHeight - bottomHeight;
	for (const auto& e : bottom) {
		const float elemPadding = e->m_modifier.getPadding();
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos + elemPadding});
		yPos += e->m_bounds.getSize().y + elemPadding * 2.f;
	}

	// Apply horizontal alignment in final pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			const float elemPadding = e->m_modifier.getPadding();
			sf::Vector2f pos = e->m_bounds.getPosition();

			if (hasAlign(alignment, Align::CENTER_X)) {
				pos.x = boundsPos.x + elemPadding + (boundsSize.x - e->m_bounds.getSize().x - elemPadding * 2.f) * 0.5f;
			} else if (hasAlign(alignment, Align::RIGHT)) {
				pos.x = boundsPos.x + boundsSize.x - e->m_bounds.getSize().x - elemPadding;
			} else {
				pos.x = boundsPos.x + elemPadding;
			}

			e->m_bounds.setPosition(pos);
		}
	}
}

inline void Column::render(sf::RenderTarget& target) {
	if (getType() == EType::ScrollableColumn) {
		// Use scissor test for clipping instead of views to avoid coordinate space issues
		sf::FloatRect clipRect = m_bounds.getGlobalBounds();
		sf::View currentView = target.getView();
		
		// Convert world coordinates to pixel coordinates for scissor rect
		sf::Vector2f topLeft = {clipRect.position.x, clipRect.position.y};
		sf::Vector2f bottomRight = {clipRect.position.x + clipRect.size.x, 
		                             clipRect.position.y + clipRect.size.y};
		
		sf::Vector2i topLeftPixel = target.mapCoordsToPixel(topLeft, currentView);
		sf::Vector2i bottomRightPixel = target.mapCoordsToPixel(bottomRight, currentView);
		
		// SFML's scissor coordinates are from bottom-left, need to flip Y
		sf::Vector2u windowSize = target.getSize();
		int scissorX = topLeftPixel.x;
		int scissorY = windowSize.y - bottomRightPixel.y;  // Flip Y
		int scissorWidth = bottomRightPixel.x - topLeftPixel.x;
		int scissorHeight = bottomRightPixel.y - topLeftPixel.y;
		
		// Save current scissor state
		GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
		GLint oldScissor[4];
		if (scissorWasEnabled) {
			glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
			
			// Intersect with parent's scissor region
			int parentX = oldScissor[0];
			int parentY = oldScissor[1];
			int parentRight = parentX + oldScissor[2];
			int parentTop = parentY + oldScissor[3];
			
			int childRight = scissorX + scissorWidth;
			int childTop = scissorY + scissorHeight;
			
			// Calculate intersection
			scissorX = std::max(scissorX, parentX);
			scissorY = std::max(scissorY, parentY);
			int right = std::min(childRight, parentRight);
			int top = std::min(childTop, parentTop);
			
			scissorWidth = std::max(0, right - scissorX);
			scissorHeight = std::max(0, top - scissorY);
		}
		
		// Enable scissor test with new bounds
		glEnable(GL_SCISSOR_TEST);
		glScissor(scissorX, scissorY, scissorWidth, scissorHeight);

		// Render background with optional rounded corners
		if (m_modifier.getRounded() > 0.f) {
			auto rounded = createRoundedRect(m_bounds.getSize(), m_modifier.getRounded(), m_bounds.getFillColor());
			rounded.setPosition(m_bounds.getPosition());
			target.draw(rounded);
		} else {
			target.draw(m_bounds);
		}
		 
		sf::RenderStates states;
		states.transform.translate(m_bounds.getPosition());

		// Render normal priority elements first (including their custom geometry, clipped)
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority()) {
				e->render(target);
			}

		// Render this column's custom geometry, clipped
		for (auto& d : m_customGeometry) {
			target.draw(*d, states);
		}
		 
		// Then render high priority elements on top (including their custom geometry, clipped)
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority()) {
				e->render(target);
			}

		// Restore previous scissor state
		if (scissorWasEnabled) {
			glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}
	else {
		// Render background with optional rounded corners
		if (m_modifier.getRounded() > 0.f) {
			auto rounded = createRoundedRect(m_bounds.getSize(), m_modifier.getRounded(), m_bounds.getFillColor());
			rounded.setPosition(m_bounds.getPosition());
			target.draw(rounded);
		} else {
			target.draw(m_bounds);
		}
		 
		// Render normal priority elements first
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
				e->render(target);

		// Render custom geometry
		sf::RenderStates states;
		states.transform.translate(m_bounds.getPosition());
		for (auto& d : m_customGeometry) {
			target.draw(*d, states);
		}
		 
		// Then render high priority elements on top
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority())
				e->render(target);
	}
}

inline bool Column::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	bool childClicked = false;
	
	// FreeColumns (used for dropdown options) should not use bounds checking
	// to allow clicks to reach their children regardless of positioning
	bool isFreeColumn = (getType() == EType::FreeColumn);
	
	for (auto& e : m_elements)
		if (e) {
			if (e->m_modifier.isVisible() && (isFreeColumn || e->m_bounds.getGlobalBounds().contains(pos)))
				childClicked = e->checkClick(pos, button);
			if (childClicked) return childClicked;
		}
	return Element::checkClick(pos, button);
}

inline void Column::checkHover(const sf::Vector2f& pos) {
	Element::checkHover(pos);
	
	// FreeColumns (used for dropdown options) should not use bounds checking
	bool isFreeColumn = (getType() == EType::FreeColumn);
	
	for (auto& e : m_elements)
		if (e && (isFreeColumn || e->m_bounds.getGlobalBounds().contains(pos))) e->checkHover(pos);
}

inline EType Column::getType() const { return EType::Column; }

inline void Column::applyHorizontalAlignment(Element* e, const sf::RectangleShape& parentBounds) {
	auto align = e->m_modifier.getAlignment();
	auto pos = e->m_bounds.getPosition();

	if (hasAlign(align, Align::CENTER_X))
		pos.x = parentBounds.getPosition().x + (parentBounds.getSize().x - e->m_bounds.getSize().x) / 2.f;
	else if (hasAlign(align, Align::RIGHT))
		pos.x = parentBounds.getPosition().x + parentBounds.getSize().x - e->m_bounds.getSize().x;

	e->m_bounds.setPosition(pos);
}

// ---------------------------------------------------------------------------- //
// Scrollable Column Implementation
// ---------------------------------------------------------------------------- //
inline void ScrollableColumn::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	applyModifiers();
	Element::update(parentBounds);

	// Update layout and positions (but not children yet)
	Column::update(parentBounds);

	if (!m_elements.empty()) {
		// Cache bounds for intersection tests
		const sf::FloatRect containerBounds = m_bounds.getGlobalBounds();
		const float offsetY = m_offset;
		
		// Apply offset and check visibility in single pass
		Element* firstElement = nullptr;
		Element* lastElement = nullptr;
		
		for (auto& e : m_elements) {
			if (e->m_modifier.isVisible()) {
				if (!firstElement) firstElement = e;
				lastElement = e;
				
				// Apply vertical offset
				const sf::Vector2f currentPos = e->m_bounds.getPosition();
				e->m_bounds.setPosition({currentPos.x, currentPos.y + offsetY});
				
				// Only update visibility flag and children - elements are already properly sized by Column::update
				const sf::FloatRect elementBounds = e->m_bounds.getGlobalBounds();
				const bool isVisible = containerBounds.findIntersection(elementBounds).has_value();
				e->m_doRender = isVisible;
				
				// Update children if element is a container
				e->updateChildren();
			}
		}

		// Boundary checking with cached positions
		if (firstElement && lastElement) {
			const float containerTop = m_bounds.getPosition().y;
			const float containerBottom = m_bounds.getPosition().y + m_bounds.getSize().y;
			const float lastElementBottom = lastElement->m_bounds.getPosition().y + lastElement->m_bounds.getSize().y;
			const float firstElementTop = firstElement->m_bounds.getPosition().y;
			
			// First, ensure top alignment (priority)
			if (firstElementTop >= containerTop) {
				m_offset -= firstElementTop - containerTop;
			}
			// Then, if there's overflow and we've scrolled past the bottom, align to bottom
			else if (lastElementBottom <= containerBottom) {
				m_offset += containerBottom - lastElementBottom;
			}
		}
	}
}

inline void ScrollableColumn::checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
	if (m_locked) return;
	
	if (m_bounds.getGlobalBounds().contains(pos)) {
		if (verticalDelta < 0)
			m_offset -= m_scrollSpeed;
		else if (verticalDelta > 0)
			m_offset += m_scrollSpeed;
		else if (horizontalDelta != 0)
			for (auto& e : m_elements)
				e->checkScroll(pos, verticalDelta, horizontalDelta);
	}
}

inline EType ScrollableColumn::getType() const { return EType::ScrollableColumn; }

// ---------------------------------------------------------------------------- //
// Grid Container Implementation
// ---------------------------------------------------------------------------- //
inline Grid::Grid(
	Modifier modifier,
	float cellWidth,
	float cellHeight,
	int columns,
	int rows,
	std::initializer_list<Element*> elements,
	const std::string& name
) : Container(elements, name), m_cellWidth(cellWidth), m_cellHeight(cellHeight), m_columns(columns), m_rows(rows) {
	m_modifier = modifier;
	m_name = name;
	
	if (!m_name.empty() && m_uilo)
		m_uilo->m_grids[m_name] = this;
}

inline void Grid::setUilo(UILO* uilo) {
	Container::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_grids[m_name] = this;
}

inline void Grid::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	applyModifiers();
	Element::update(parentBounds);

	if (m_elements.empty()) return;

	const sf::Vector2f boundsPos = m_bounds.getPosition();
	const sf::FloatRect containerBounds = m_bounds.getGlobalBounds();
	
	int actualColumns = m_columns > 0 ? m_columns : static_cast<int>(std::ceil(std::sqrt(static_cast<float>(m_elements.size()))));
	if (actualColumns == 0) actualColumns = 1;
	
	size_t index = 0;
	for (auto& e : m_elements) {
		if (!e->m_modifier.isVisible()) continue;
		
		int col = static_cast<int>(index % actualColumns);
		int row = static_cast<int>(index / actualColumns);
		
		sf::RectangleShape cellBounds;
		cellBounds.setSize({m_cellWidth, m_cellHeight});
		
		float xPos = boundsPos.x + (col * m_cellWidth) + m_horizontalOffset;
		float yPos = boundsPos.y + (row * m_cellHeight) + m_verticalOffset;
		cellBounds.setPosition({xPos, yPos});
		
		e->update(cellBounds);
		e->m_bounds.setPosition({xPos, yPos});
		
		Align alignment = e->m_modifier.getAlignment();
		sf::Vector2f pos = e->m_bounds.getPosition();
		
		if (hasAlign(alignment, Align::CENTER_X)) {
			pos.x = xPos + (m_cellWidth - e->m_bounds.getSize().x) * 0.5f;
		} else if (hasAlign(alignment, Align::RIGHT)) {
			pos.x = xPos + m_cellWidth - e->m_bounds.getSize().x;
		}
		
		if (hasAlign(alignment, Align::CENTER_Y)) {
			pos.y = yPos + (m_cellHeight - e->m_bounds.getSize().y) * 0.5f;
		} else if (hasAlign(alignment, Align::BOTTOM)) {
			pos.y = yPos + m_cellHeight - e->m_bounds.getSize().y;
		}
		
		e->m_bounds.setPosition(pos);
		
		const sf::FloatRect elementBounds = e->m_bounds.getGlobalBounds();
		const bool isVisible = containerBounds.findIntersection(elementBounds).has_value();
		e->m_doRender = isVisible;
		
		e->updateChildren();
		
		index++;
	}
	
	if (m_columns > 0 || m_rows > 0) {
		size_t visibleCount = 0;
		for (const auto& e : m_elements) {
			if (e->m_modifier.isVisible()) visibleCount++;
		}
		
		if (visibleCount > 0) {
			int actualCols = m_columns > 0 ? m_columns : actualColumns;
			int actualRows = m_rows > 0 ? m_rows : static_cast<int>(std::ceil(static_cast<float>(visibleCount) / actualCols));
			
			float totalWidth = actualCols * m_cellWidth;
			float totalHeight = actualRows * m_cellHeight;
			
			if (totalWidth > m_bounds.getSize().x) {
				float maxOffset = 0.f;
				float minOffset = m_bounds.getSize().x - totalWidth;
				if (m_horizontalOffset > maxOffset) m_horizontalOffset = maxOffset;
				if (m_horizontalOffset < minOffset) m_horizontalOffset = minOffset;
			} else {
				m_horizontalOffset = 0.f;
			}
			
			if (totalHeight > m_bounds.getSize().y) {
				float maxOffset = 0.f;
				float minOffset = m_bounds.getSize().y - totalHeight;
				if (m_verticalOffset > maxOffset) m_verticalOffset = maxOffset;
				if (m_verticalOffset < minOffset) m_verticalOffset = minOffset;
			} else {
				m_verticalOffset = 0.f;
			}
		}
	}
}

inline void Grid::render(sf::RenderTarget& target) {
	sf::View originalView = target.getView();
	sf::FloatRect clipRect = m_bounds.getGlobalBounds();
	sf::View clippingView(clipRect);

	sf::Vector2f worldPos = {clipRect.position.x, clipRect.position.y};
	sf::Vector2i pixelPos = target.mapCoordsToPixel(worldPos, originalView);

	sf::Vector2u windowSize = target.getSize();
	float renderScale = m_uilo ? m_uilo->m_renderScale : 1.0f;
	sf::Vector2f effectiveWindowSize = {
		static_cast<float>(windowSize.x) / renderScale,
		static_cast<float>(windowSize.y) / renderScale
	};
	sf::FloatRect viewport(
		{static_cast<float>(pixelPos.x) / windowSize.x,
		 static_cast<float>(pixelPos.y) / windowSize.y},
		{clipRect.size.x / effectiveWindowSize.x,
		 clipRect.size.y / effectiveWindowSize.y}
	);

	clippingView.setViewport(viewport);
	target.setView(clippingView);

	target.draw(m_bounds);

	sf::RenderStates states;
	states.transform.translate(m_bounds.getPosition());

	for (auto& e : m_elements)
		if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority()) {
			e->render(target);
		}

	for (auto& d : m_customGeometry) {
		target.draw(*d, states);
	}

	for (auto& e : m_elements)
		if (e->m_modifier.isVisible() && e->m_doRender && e->m_modifier.isHighPriority()) {
			e->render(target);
		}

	target.setView(originalView);
}

inline bool Grid::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	bool childClicked = false;
	for (auto& e : m_elements)
		if (e) {
			if (e->m_modifier.isVisible() && e->m_bounds.getGlobalBounds().contains(pos))
				childClicked = e->checkClick(pos, button);
			if (childClicked) return childClicked;
		}
	return Element::checkClick(pos, button);
}

inline void Grid::checkHover(const sf::Vector2f& pos) {
	Element::checkHover(pos);
	for (auto& e : m_elements)
		if (e && e->m_bounds.getGlobalBounds().contains(pos)) e->checkHover(pos);
}

inline void Grid::checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
	if (m_bounds.getGlobalBounds().contains(pos)) {
		if (!m_verticalLocked && verticalDelta != 0) {
			if (verticalDelta < 0)
				m_verticalOffset -= m_scrollSpeed;
			else if (verticalDelta > 0)
				m_verticalOffset += m_scrollSpeed;
		}
		
		if (!m_horizontalLocked && horizontalDelta != 0) {
			if (horizontalDelta < 0)
				m_horizontalOffset -= m_scrollSpeed;
			else if (horizontalDelta > 0)
				m_horizontalOffset += m_scrollSpeed;
		}
		
		if ((m_verticalLocked || verticalDelta == 0) && (m_horizontalLocked || horizontalDelta == 0)) {
			for (auto& e : m_elements)
				e->checkScroll(pos, verticalDelta, horizontalDelta);
		}
	}
}

inline EType Grid::getType() const { return EType::Grid; }

// ---------------------------------------------------------------------------- //
// Free Column Implementation
// ---------------------------------------------------------------------------- //
inline void FreeColumn::update(sf::RectangleShape& parentBounds) {
	sf::RectangleShape newParentBounds({0, 0});
	newParentBounds.setPosition(m_customPosition);
	newParentBounds.setSize(m_bounds.getSize());
	Column::update(newParentBounds);
	m_bounds.setPosition(m_customPosition);
	Element::update(parentBounds);
}

inline bool FreeColumn::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	// First check if click is within this FreeColumn's bounds
	bool withinBounds = getBounds().contains(pos);
	
	if (!withinBounds)
		return false;
	
	// Check children first
	for (auto& e : m_elements) {
		if (e && e->m_modifier.isVisible() && e->m_bounds.getGlobalBounds().contains(pos)) {
			if (e->checkClick(pos, button))
				return true;
		}
	}
	
	// Click is within FreeColumn bounds - call callbacks if any and consume the click
	if (button == sf::Mouse::Button::Left) {
		if (m_modifier.getOnLClick()) m_modifier.getOnLClick()();
	} else if (button == sf::Mouse::Button::Right) {
		if (m_modifier.getOnRClick()) m_modifier.getOnRClick()();
	}
	
	// Always return true if click is within bounds to prevent passthrough
	return true;
}

inline EType FreeColumn::getType() const { return EType::FreeColumn; }

inline void FreeColumn::setPosition(sf::Vector2f pos) { m_customPosition = pos; }

inline sf::Vector2f FreeColumn::getPosition() const { return m_customPosition; }

inline sf::Vector2f FreeColumn::getSize() const { return m_bounds.getSize(); }

inline sf::Vector2f FreeColumn::getCenter() const {
	return
	{
		m_customPosition.x + (m_bounds.getSize().x / 2),
		m_customPosition.y + (m_bounds.getSize().y / 2)
	};
}

inline sf::FloatRect FreeColumn::getBounds() const {
	return sf::FloatRect(m_customPosition, m_bounds.getSize());
}

// ---------------------------------------------------------------------------- //
// Text Implementation
// ---------------------------------------------------------------------------- //
inline Text::Text(Modifier modifier, const std::string& str, sf::Font font, const std::string& name)
: m_string(str), m_font(font){
	m_modifier = modifier;
	m_name = name;

	if (!m_name.empty() && m_uilo)
		m_uilo->m_texts[m_name] = this;
}

inline Text::Text(Modifier modifier, const std::string& str, const std::string& fontPath, const std::string& name)
: m_string(str) {
	m_modifier = modifier;
	m_name = name;
	if (!m_name.empty() && m_uilo)
		m_uilo->m_texts[m_name] = this;

	bool fontLoaded = false;
	if (!fontPath.empty()) {
		fontLoaded = m_font.openFromFile(fontPath);
		if (!fontLoaded) {
			std::cerr << "Text Warning: couldn't load font \"" << fontPath << "\", using default font.\n";
		}
	}
	
	// If fontPath is empty or loading failed, use global default font (auto-loads embedded font)
	if (!fontLoaded) {
		sf::Font* defaultFont = getDefaultFont();
		if (defaultFont) {
			m_font = *defaultFont;
		} else {
			std::cerr << "Text Warning: No default font available when creating Text with string: \"" << str << "\"\n";
		}
	}
}

inline void Text::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);

	m_bounds.setPosition(parentBounds.getPosition());
	float fontSize = m_modifier.getFixedHeight() > 0
		? m_modifier.getFixedHeight()
		: m_bounds.getSize().y * 0.8f;

	m_text.emplace(m_font, sf::String::fromUtf8(m_string.begin(), m_string.end()));
	
	// text supersampling
	const float supersampleFactor = 4.0f;
	m_text->setCharacterSize(static_cast<unsigned>(fontSize * supersampleFactor));
	m_text->setScale({1.0f / supersampleFactor, 1.0f / supersampleFactor});
	
	m_text->setFillColor(m_modifier.getColor());

	sf::FloatRect textBounds = m_text->getLocalBounds();
	float textWidth = textBounds.size.x / supersampleFactor;
	m_bounds.setSize({ textWidth, m_bounds.getSize().y });

	Element::update(parentBounds);
}

inline void Text::render(sf::RenderTarget& target) {
	if (!m_text) return;

	float centerX = m_bounds.getPosition().x + m_bounds.getSize().x / 2.f;
	float centerY = m_bounds.getPosition().y + m_bounds.getSize().y / 2.f;
	
	sf::FloatRect localBounds = m_text->getLocalBounds();
	sf::Text referenceText(m_text->getFont(), "A");
	referenceText.setCharacterSize(m_text->getCharacterSize());
	sf::FloatRect refBounds = referenceText.getLocalBounds();
	
	float verticalOrigin = refBounds.position.y + refBounds.size.y / 2.f;
	
	m_text->setOrigin({
		localBounds.position.x + localBounds.size.x / 2.f,
		verticalOrigin
	});

	m_text->setPosition({centerX, centerY});

	target.draw(*m_text);
}

inline void Text::setString(const std::string& newStr) {
	m_string = newStr;
	if (m_text) {
		// Use sf::String::fromUtf8 to properly decode UTF-8 multi-byte characters
		m_text->setString(sf::String::fromUtf8(newStr.begin(), newStr.end()));
	}
	m_isDirty = true;
}

inline void Text::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_texts[m_name] = this;
}

// ---------------------------------------------------------------------------- //
// Spacer Implementation
// ---------------------------------------------------------------------------- //
inline Spacer::Spacer(Modifier& modifier, const std::string& name) {
	m_modifier = modifier;
	m_name = name;

	if (!m_name.empty() && m_uilo)
		m_uilo->m_spacers[m_name] = this;
}

inline void Spacer::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_spacers[m_name] = this;
}

inline void Spacer::update(sf::RectangleShape& parentBounds) {
	m_bounds.setFillColor(sf::Color::Transparent);
	resize(parentBounds);
	applyModifiers();
	Element::update(parentBounds);
}

// ---------------------------------------------------------------------------- //
// Button Implementation
// ---------------------------------------------------------------------------- //
inline Button::Button(
	Modifier modifier,
	ButtonStyle buttonStyle,
	const std::string& buttonText,
	const std::string& textFont,
	sf::Color textColor,
	const std::string& name
) {
	m_modifier = modifier;
	m_buttonStyle = buttonStyle;

	m_bodyRect.setFillColor(m_modifier.getColor());
	m_leftCircle.setFillColor(m_modifier.getColor());
	m_rightCircle.setFillColor(m_modifier.getColor());

	// Always create text - Text constructor will handle empty fontPath by using embedded font
	m_text = std::make_unique<Text>(
		Modifier()
			.setColor(textColor)
			.align(Align::CENTER_Y | Align::CENTER_X)
			.setHeight(0.5f),
		buttonText,
		textFont,  // Can be empty - Text will use embedded font
		""
	);

	m_textRow = std::make_unique<Row>(
		Modifier().setColor(sf::Color::Transparent).setHeight(1.f).setWidth(1.f),
		std::initializer_list<Element*>{ m_text.get() },
		""
	);

	m_name = name;
	if (!m_name.empty() && m_uilo) {
		m_uilo->m_buttons[m_name] = this;
	}
}

inline Button::Button(
	Modifier modifier,
	ButtonStyle buttonStyle,
	const std::string& buttonText,
	sf::Font& font,
	sf::Color textColor,
	const std::string& name
) {
	m_modifier = modifier;
	m_buttonStyle = buttonStyle;

	m_bodyRect.setFillColor(m_modifier.getColor());
	m_leftCircle.setFillColor(m_modifier.getColor());
	m_rightCircle.setFillColor(m_modifier.getColor());

	m_text = std::make_unique<Text>(
		Modifier()
			.setColor(textColor)
			.align(Align::CENTER_Y | Align::CENTER_X)
			.setHeight(0.5f),
		buttonText,
		font,
		""
	);

	m_textRow = std::make_unique<Row>(
		Modifier().setColor(sf::Color::Transparent).setHeight(1.f).setWidth(1.f),
		std::initializer_list<Element*>{ m_text.get() },
		""
	);

	m_name = name;
	if (!m_name.empty() && m_uilo) {
		m_uilo->m_buttons[m_name] = this;
	}
}

inline void Button::update(sf::RectangleShape& parentBounds) {
	Element::update(parentBounds);
	resize(parentBounds);
	applyModifiers();

	m_bodyRect.setFillColor(m_modifier.getColor());
	m_leftCircle.setFillColor(m_modifier.getColor());
	m_rightCircle.setFillColor(m_modifier.getColor());
	
	// Set up bodyRect for proper text positioning
	if (m_buttonStyle == ButtonStyle::Default || m_buttonStyle == ButtonStyle::Rect) {
		m_bodyRect.setSize(m_bounds.getSize());
		m_bodyRect.setPosition(m_bounds.getPosition());
	} else {
		// For pill style buttons
		m_bodyRect.setSize({
			m_bounds.getSize().x - m_bounds.getSize().y,
			m_bounds.getSize().y
		});
		m_bodyRect.setPosition({
			m_bounds.getPosition().x + (m_bounds.getSize().y / 2),
			m_bounds.getPosition().y
		});
	}
	
	if (m_textRow) {
		// Update ScrollableRow with bodyRect bounds for proper text sizing
		m_textRow->update(m_bounds);
		m_textRow->setPosition(m_bounds.getPosition());
	}
}

inline void Button::render (sf::RenderTarget& target) {
	if (m_buttonStyle == ButtonStyle::Default || m_buttonStyle == ButtonStyle::Rect) {
		target.draw(m_bounds);
	}
	else {
		m_leftCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));
		m_rightCircle.setPointCount(static_cast<unsigned int>(m_bounds.getSize().y * 2));

		m_leftCircle.setRadius(m_bounds.getSize().y / 2);
		m_rightCircle.setRadius(m_bounds.getSize().y / 2);
		
		m_leftCircle.setPosition(m_bounds.getPosition());
		m_rightCircle.setPosition(
		{
			m_bounds.getPosition().x + m_bounds.getSize().x - m_bounds.getSize().y,
			m_bounds.getPosition().y
		});

		target.draw(m_leftCircle);

		if (!(m_leftCircle.getPosition() == m_rightCircle.getPosition())) {
			target.draw(m_rightCircle);
			target.draw(m_bodyRect);
		}
	}

	if (m_textRow) {
		m_textRow->render(target);
	}
}

inline bool Button::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (m_bounds.getGlobalBounds().contains(pos)) {
		if (button == sf::Mouse::Button::Left) {
			m_isClicked = true;
			if (m_modifier.getOnLClick()) {
				m_modifier.getOnLClick()();
			}
			return m_isClicked;
		} else if (button == sf::Mouse::Button::Right) {
			m_isClicked = true;
			if (m_modifier.getOnRClick()) {
				m_modifier.getOnRClick()();
			}
			return m_isClicked;
		}
	}

	return false;
}

inline void Button::checkHover(const sf::Vector2f& pos) {
	m_isHovered = m_bounds.getGlobalBounds().contains(pos);
	Element::checkHover(pos);
}

inline void Button::setText(const std::string& newStr) {
	if (m_text)
		m_text->setString(sf::String::fromUtf8(newStr.begin(), newStr.end()));
}

inline std::string Button::getText() const {
	if (m_text)
		return m_text->getString();
	return "";
}

inline void Button::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_buttons[m_name] = this;
}

// ---------------------------------------------------------------------------- //
// Slider Implementation
// ---------------------------------------------------------------------------- //
inline Slider::Slider(
	Modifier modifier,
	sf::Color knobColor,
	sf::Color barColor,
	SliderOrientation orientation,
	float initialValue,
	const std::string& name
) : m_knobColor(knobColor), m_barColor(barColor), m_orientation(orientation), m_initVal(initialValue), m_curVal(initialValue) {
	m_modifier = modifier;
	m_knobRect.setFillColor(m_knobColor);
	m_barRect.setFillColor(m_barColor);

	m_name = name;
	if (!m_name.empty() && m_uilo) {
		m_uilo->m_sliders[m_name] = this;
	}
}

inline void Slider::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	applyModifiers();

	if (m_orientation == SliderOrientation::Vertical) {
		// Vertical slider (original implementation)
		m_knobRect.setSize(
		{
			m_bounds.getSize().x,
			m_bounds.getSize().x * 0.25f
		});

		m_barRect.setSize(
		{
			4.f,
			m_bounds.getSize().y
		});

		m_barRect.setPosition(
		{
			m_bounds.getPosition().x + (m_bounds.getSize().x / 2) - 2,
			m_bounds.getPosition().y
		});

		m_knobRect.setPosition(
		{
			m_bounds.getPosition().x,
			m_bounds.getPosition().y + m_bounds.getSize().y - (m_bounds.getSize().y * m_curVal)
		});
	} else {
		// Horizontal slider
		m_knobRect.setSize(
		{
			m_bounds.getSize().y * 0.25f,
			m_bounds.getSize().y
		});

		m_barRect.setSize(
		{
			m_bounds.getSize().x,
			4.f
		});

		m_barRect.setPosition(
		{
			m_bounds.getPosition().x,
			m_bounds.getPosition().y + (m_bounds.getSize().y / 2) - 2
		});

		m_knobRect.setPosition(
		{
			m_bounds.getPosition().x + (m_bounds.getSize().x * m_curVal) - (m_knobRect.getSize().x / 2),
			m_bounds.getPosition().y
		});
	}

	Element::update(parentBounds);
}

inline void Slider::render(sf::RenderTarget& target) {
	target.draw(m_barRect);
	target.draw(m_knobRect);
	
	// Render custom geometry
	Element::render(target);
}

inline bool Slider::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (button == sf::Mouse::Button::Left && m_bounds.getGlobalBounds().contains(pos)) {
		if (m_uilo) {
			m_uilo->m_activeDragSlider = this;
		}
		
		m_isDragging = true;

		if (doubleClickTimer.isRunning()) {
			if (doubleClickTimer.getElapsedTime().asMilliseconds() <= 250) {
				m_curVal = m_initVal;
				doubleClickTimer.stop();
				return true;
			}
		}
		doubleClickTimer.restart();
		
		float t, v;
		 
		if (m_orientation == SliderOrientation::Vertical) {
			// Vertical slider (original implementation)
			float relY = pos.y - m_bounds.getPosition().y;
			t = 1.f - (relY / m_bounds.getSize().y);
		} else {
			// Horizontal slider
			float relX = pos.x - m_bounds.getPosition().x;
			t = relX / m_bounds.getSize().x;
		}
		 
		v = m_minVal + t * (m_maxVal - m_minVal);

		if (v < m_minVal) v = m_minVal;
		if (v > m_maxVal) v = m_maxVal;
		
		// Apply quantization if enabled
		if (m_quantizationSteps > 0) {
			v = std::round(v * m_quantizationSteps) / m_quantizationSteps;
		}
		
		m_curVal = v;

		return true;
	}

	return false;
}

inline bool Slider::handleDrag(const sf::Vector2f& pos) {
	float t, v;
	 
	if (m_orientation == SliderOrientation::Vertical) {
		// Vertical slider (original implementation)
		float relY = pos.y - m_bounds.getPosition().y;
		t = 1.f - (relY / m_bounds.getSize().y);
	} else {
		// Horizontal slider
		float relX = pos.x - m_bounds.getPosition().x;
		t = relX / m_bounds.getSize().x;
	}
	 
	v = m_minVal + t * (m_maxVal - m_minVal);

	if (v < m_minVal) v = m_minVal;
	if (v > m_maxVal) v = m_maxVal;
	
	// Apply quantization if enabled
	if (m_quantizationSteps > 0) {
		v = std::round(v * m_quantizationSteps) / m_quantizationSteps;
	}
	
	m_curVal = v;

	return true;
}

inline float Slider::getValue() const { return m_curVal; }

inline void Slider::setValue(float newVal) {
	m_curVal = newVal < m_minVal ? m_minVal : (newVal > m_maxVal ? m_maxVal : newVal);
}

inline void Slider::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_sliders[m_name] = this;
}

// ---------------------------------------------------------------------------- //
// Knob Implementation
// ---------------------------------------------------------------------------- //
inline Knob::Knob(
	Modifier modifier,
	sf::Color knobColor,
	sf::Color trackColor,
	sf::Color arcColor,
	float initialValue,
	const std::string& name
) : m_knobColor(knobColor), m_trackColor(trackColor), m_arcColor(arcColor), m_initVal(initialValue), m_curVal(initialValue) {
	m_modifier = modifier;
	m_name = name;
	if (!m_name.empty() && m_uilo) {
		m_uilo->m_sliders[m_name] = (Slider*)this; // Store as slider for now
	}
}

inline void Knob::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	applyModifiers();

	// Calculate center and radius based on bounds
	float size = std::min(m_bounds.getSize().x, m_bounds.getSize().y);
	m_radius = size * 0.4f;
	m_center = sf::Vector2f(
		m_bounds.getPosition().x + m_bounds.getSize().x / 2.f,
		m_bounds.getPosition().y + m_bounds.getSize().y / 2.f
	);

	Element::update(parentBounds);
}

inline void Knob::render(sf::RenderTarget& target) {
	const int segments = 60;
	// SFML coords: Y-down, so angles are: 0°=right, 90°=down, 180°=left, 270°=up
	// 7 o'clock = 135° (bottom-left), 12 o'clock = 270° (top), 5 o'clock = 45° (bottom-right)
	// To go CLOCKWISE from 135° through 270° to 45°, we add 360° to end: 405°
	const float startAngle = 135.f * 3.14159f / 180.f;   // 7 o'clock (bottom-left) - value 0
	const float endAngle = 405.f * 3.14159f / 180.f;     // 5 o'clock (45° + 360°) - value 1  
	const float angleRange = endAngle - startAngle;      // +270° = clockwise sweep
	
	// Draw inner circle filled with modifier color
	float innerCircleRadius = m_radius - 3.f;
	sf::CircleShape innerCircle(innerCircleRadius);
	innerCircle.setOrigin({innerCircleRadius, innerCircleRadius});
	innerCircle.setPosition(m_center);
	innerCircle.setFillColor(m_modifier.getColor());
	target.draw(innerCircle);
	
	// Draw full track arc from start to end in trackColor
	sf::VertexArray trackArc(sf::PrimitiveType::TriangleStrip);
	for (int i = 0; i <= segments; ++i) {
		float t = float(i) / float(segments);
		float angle = startAngle + angleRange * t;
		
		float innerRadius = m_radius - 3.f;
		float outerRadius = m_radius + 3.f;
		
		sf::Vector2f inner(m_center.x + std::cos(angle) * innerRadius,
		                   m_center.y + std::sin(angle) * innerRadius);
		sf::Vector2f outer(m_center.x + std::cos(angle) * outerRadius,
		                   m_center.y + std::sin(angle) * outerRadius);
		
		trackArc.append({inner, m_trackColor});
		trackArc.append({outer, m_trackColor});
	}
	target.draw(trackArc);
	
	// Draw value arc from start position clockwise to current value position
	sf::VertexArray arc(sf::PrimitiveType::TriangleStrip);
	
	// Calculate the angle span from start (7 o'clock) to current value
	float arcSpan = angleRange * m_curVal;
	
	const int arcSegments = std::max(1, int(segments * m_curVal));
	
	for (int i = 0; i <= arcSegments; ++i) {
		float t = float(i) / float(arcSegments);
		// Interpolate from startAngle clockwise by arcSpan
		float angle = startAngle + arcSpan * t;
		
		float innerRadius = m_radius - 3.f;
		float outerRadius = m_radius + 3.f;
		
		sf::Vector2f inner(m_center.x + std::cos(angle) * innerRadius,
		                   m_center.y + std::sin(angle) * innerRadius);
		sf::Vector2f outer(m_center.x + std::cos(angle) * outerRadius,
		                   m_center.y + std::sin(angle) * outerRadius);
		
		arc.append({inner, m_arcColor});
		arc.append({outer, m_arcColor});
	}
	target.draw(arc);
	
	// Draw indicator dot at current value position (1/6 radius)
	// Position inside the arc with one full dot radius gap between arc inner edge and dot
	float indicatorAngle = startAngle + angleRange * m_curVal;
	float dotRadius = m_radius / 6.f;
	float arcInnerEdge = m_radius - 3.f;
	float dotCenterDistance = arcInnerEdge - dotRadius - dotRadius;
	
	sf::Vector2f dotCenter(
		m_center.x + std::cos(indicatorAngle) * dotCenterDistance,
		m_center.y + std::sin(indicatorAngle) * dotCenterDistance
	);
	
	sf::CircleShape dot(dotRadius);
	dot.setOrigin({dotRadius, dotRadius});
	dot.setPosition(dotCenter);
	dot.setFillColor(m_knobColor);
	target.draw(dot);
	
	// Render custom geometry
	Element::render(target);
}

inline bool Knob::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (button == sf::Mouse::Button::Left && m_bounds.getGlobalBounds().contains(pos)) {
		if (m_uilo) {
			m_uilo->m_activeDragKnob = this;
		}

		if (doubleClickTimer.isRunning()) {
			if (doubleClickTimer.getElapsedTime().asMilliseconds() <= 250) {
				m_curVal = m_initVal;
				m_isDragging = true;  // Mark as dragging so sync knows this is user interaction
				doubleClickTimer.stop();
				return true;
			}
		}
		doubleClickTimer.restart();
		
		// Initialize delta-based dragging
		m_isDragging = true;
		m_lastMousePos = pos;

		return true;
	}

	return false;
}

inline bool Knob::handleDrag(const sf::Vector2f& pos) {
	if (!m_isDragging) {
		return false;
	}
	
	float deltaY = m_lastMousePos.y - pos.y;
	
	// Sensitivity and smoothing
	const float sensitivity = 0.005f;
	const float threshold = 0.5f;
	
	// Only update if movement exceeds threshold
	if (std::abs(deltaY) < threshold)
		return true;
	
	// Apply delta to current value
	float delta = deltaY * sensitivity;
	m_curVal += delta;
	m_curVal = std::max(0.f, std::min(1.f, m_curVal));
	
	// Apply quantization if enabled
	if (m_quantizationSteps > 0) {
		m_curVal = std::round(m_curVal * m_quantizationSteps) / m_quantizationSteps;
	}
	
	m_lastMousePos = pos;

	return true;
}

inline float Knob::getValue() const { return m_curVal; }

inline void Knob::setValue(float newVal) {
	m_curVal = newVal < m_minVal ? m_minVal : (newVal > m_maxVal ? m_maxVal : newVal);
}

inline void Knob::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_sliders[m_name] = (Slider*)this;
}

// ---------------------------------------------------------------------------- //
// Dropdown Implementation
// ---------------------------------------------------------------------------- //
inline Dropdown::Dropdown(
	Modifier modifier,
	const std::string& defaultText,
	const std::vector<std::string>& options,
	const std::string& textFont,
	sf::Color textColor,
	sf::Color optionBackgroundColor,
	const std::string& name
) {
	m_modifier = modifier;
	m_name = name;
	if (!m_name.empty() && m_uilo) {
		m_uilo->m_dropdowns[m_name] = this;
	}
	m_selectedOption = defaultText;

	// Use the original modifier but ensure button fills the dropdown bounds
	Modifier buttonModifier = modifier;
	buttonModifier.setWidth(1.0f).setHeight(1.0f);
	
	m_mainButton = obj<Button>(buttonModifier, ButtonStyle::Rect, m_selectedOption, textFont, textColor, "");
	m_mainButton->m_modifier.onLClick([this]() {
		if (!m_isOpen) {
			// Close any other open dropdown
			if (s_openDropdown && s_openDropdown != this) {
				s_openDropdown->m_isOpen = false;
				if (s_openDropdown->m_optionsColumn)
					s_openDropdown->m_optionsColumn->m_modifier.setVisible(false);
			}
			s_openDropdown = this;
			m_isOpen = true;
			if (m_optionsColumn) m_optionsColumn->m_modifier.setVisible(true);
		} else {
			m_isOpen = false;
			if (m_optionsColumn) m_optionsColumn->m_modifier.setVisible(false);
			if (s_openDropdown == this) s_openDropdown = nullptr;
		}
		m_isDirty = true;
	});

	m_optionsColumn = obj<FreeColumn>(
		Modifier()
			.setVisible(false)
			.setHighPriority(true)
	);

	// Move m_optionsColumn to high_priority_elements for special rendering
	if (m_optionsColumn) {
		auto it = std::find_if(uilo_owned_elements.begin(), uilo_owned_elements.end(),
			[this](const std::unique_ptr<Element>& ptr) { return ptr.get() == m_optionsColumn; });
		if (it != uilo_owned_elements.end()) {
			high_priority_elements.emplace_back(std::move(*it));
			uilo_owned_elements.erase(it);
		}
	}

	for (const auto& optionText : options) {
		Button* optBtn = obj<Button>(
			Modifier()
				.setfixedHeight(modifier.getFixedHeight())
				.setWidth(1.f)
				.setColor(optionBackgroundColor),
			ButtonStyle::Rect,
			optionText,
			textFont,
			textColor
		);
		optBtn->m_modifier.onLClick([this, optionText]() {
			m_selectedOption = optionText;
			if (m_mainButton) m_mainButton->setText(optionText);
			m_isOpen = false;
			if (m_optionsColumn) m_optionsColumn->m_modifier.setVisible(false);
			if (s_openDropdown == this) s_openDropdown = nullptr;
			m_isDirty = true;
		});
		if (m_optionsColumn) m_optionsColumn->addElement(optBtn);
	}
}

inline Dropdown::~Dropdown() {}

inline void Dropdown::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);
	applyModifiers();

	if (m_mainButton) {
		m_mainButton->m_bounds.setSize(m_bounds.getSize());
		m_mainButton->update(m_bounds);
	}

	if (m_optionsColumn) {
		m_optionsColumn->m_modifier.setVisible(m_isOpen);
		if (m_isOpen) {
			sf::Vector2f optionsPos = m_bounds.getPosition();
			optionsPos.y += m_bounds.getSize().y;
			m_optionsColumn->setPosition(optionsPos);
			m_optionsColumn->m_modifier
				.setfixedWidth(m_bounds.getSize().x)
				.setfixedHeight(m_bounds.getSize().y * m_optionsColumn->getElements().size());
			m_optionsColumn->update(parentBounds);
		}
	}

	Element::update(parentBounds);
}

inline void Dropdown::render(sf::RenderTarget& target) {
	if (m_mainButton) {
		m_mainButton->m_bounds.setPosition(m_bounds.getPosition());
		m_mainButton->render(target);
	}

	if (m_isOpen && m_mainButton && m_optionsColumn) {
		sf::Vector2f optionsPos = m_mainButton->getPosition();
		optionsPos.y += m_mainButton->getSize().y;
		m_optionsColumn->setPosition(optionsPos);
	}
}

inline bool Dropdown::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (m_isOpen && m_optionsColumn && !m_optionsColumn->getBounds().contains(pos) && !m_mainButton->m_bounds.getGlobalBounds().contains(pos)) {
		m_isOpen = false;
		m_isDirty = true;
		if (m_optionsColumn) m_optionsColumn->m_modifier.setVisible(false);
		if (s_openDropdown == this) s_openDropdown = nullptr;
		return false;
	}

	if (m_mainButton && m_mainButton->m_bounds.getGlobalBounds().contains(pos)) {
		bool handled = m_mainButton->checkClick(pos, button);
		return handled;
	}

	if (m_isOpen && m_optionsColumn && m_optionsColumn->m_modifier.isVisible()) {
		for (auto& e : m_optionsColumn->getElements()) {
			if (e && e->m_modifier.isVisible() && e->m_bounds.getGlobalBounds().contains(pos)) {
				bool handled = e->checkClick(pos, button);
				if (handled) return true;
			}
		}
	}
	return false;
}

inline void Dropdown::checkHover(const sf::Vector2f& pos) {
	Element::checkHover(pos);
	if (m_mainButton) m_mainButton->checkHover(pos);
	if (m_isOpen && m_optionsColumn) {
		m_optionsColumn->checkHover(pos);
	}
}

inline std::string Dropdown::getSelected() const {
	return m_selectedOption;
}

inline void Dropdown::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_dropdowns[m_name] = this;
}

// ---------------------------------------------------------------------------- //
// Image Implementation
// ---------------------------------------------------------------------------- //
inline TextBox::TextBox(
	Modifier modifier,
	TBStyle style,
	const std::string& fontPath,
	const std::string& defaultText,
	sf::Color textColor,
	sf::Color activeOutlineColor,
	const std::string& name
) : m_style(style), m_defaultText(defaultText), m_textColor(textColor) {
	m_modifier = modifier;
	m_name = name;
	
	if (!m_name.empty() && m_uilo)
		m_uilo->m_textboxes[m_name] = this;

	// Initialize body rectangle
	m_bodyRect.setFillColor(m_modifier.getColor());
	m_bodyRect.setOutlineColor(activeOutlineColor);

	// Initialize circles for pill style
	m_leftCircle.setFillColor(m_modifier.getColor());
	m_leftCircle.setOutlineColor(activeOutlineColor);
	
	m_rightCircle.setFillColor(m_modifier.getColor());
	m_rightCircle.setOutlineColor(activeOutlineColor);

	m_text = std::make_unique<Text>(
		Modifier()
			.setColor(textColor)
			.align(hasStyle(style, TBStyle::CenterText) ? (Align::CENTER_Y | Align::CENTER_X) : Align::CENTER_Y)
			.setHeight(0.8f),
		defaultText,
		fontPath,
		""
	);

	m_textRow = std::make_unique<Row>(
		Modifier().setColor(sf::Color::Transparent),
		std::initializer_list<Element*>{ m_text.get() },
		""
	);

	m_currentText = "";
	// Update text display to show default or current text
	setText(m_currentText);
}

inline TextBox::TextBox(
	Modifier modifier,
	TBStyle style,
	sf::Font& font,
	const std::string& defaultText,
	sf::Color textColor,
	sf::Color activeOutlineColor,
	const std::string& name
) : m_style(style), m_defaultText(defaultText), m_textColor(textColor) {
	m_modifier = modifier;
	m_name = name;
	
	if (!m_name.empty() && m_uilo)
		m_uilo->m_textboxes[m_name] = this;

	// Initialize body rectangle
	m_bodyRect.setFillColor(m_modifier.getColor());
	m_bodyRect.setOutlineColor(activeOutlineColor);

	// Initialize circles for pill style
	m_leftCircle.setFillColor(m_modifier.getColor());
	m_leftCircle.setOutlineColor(activeOutlineColor);
	
	m_rightCircle.setFillColor(m_modifier.getColor());
	m_rightCircle.setOutlineColor(activeOutlineColor);

	m_text = std::make_unique<Text>(
		Modifier()
			.setColor(textColor)
			.align(hasStyle(style, TBStyle::CenterText) ? (Align::CENTER_Y | Align::CENTER_X) : Align::CENTER_Y)
			.setHeight(0.8f),
		defaultText,
		font,
		""
	);

	m_textRow = std::make_unique<Row>(
		Modifier().setColor(sf::Color::Transparent),
		std::initializer_list<Element*>{ m_text.get() },
		""
	);

	m_currentText = "";
	// Update text display to show default or current text
	setText(m_currentText);
}

inline TextBox::~TextBox() {}

inline void TextBox::setUilo(UILO* uilo) {
	Element::setUilo(uilo);
	if (!m_name.empty() && m_uilo)
		m_uilo->m_textboxes[m_name] = this;
}

inline void TextBox::update(sf::RectangleShape& parentBounds) {
	Element::update(parentBounds);
	resize(parentBounds);
	
	// Set up m_bodyRect for proper positioning
	if (hasStyle(m_style, TBStyle::Pill)) {
		m_bodyRect.setSize({
			m_bounds.getSize().x - m_bounds.getSize().y,
			m_bounds.getSize().y
		});
		m_bodyRect.setPosition({
			m_bounds.getPosition().x + (m_bounds.getSize().y / 2),
			m_bounds.getPosition().y
		});
	} else {
		m_bodyRect.setSize(m_bounds.getSize());
		m_bodyRect.setPosition(m_bounds.getPosition());
	}
	
	if (m_textRow) {
		m_textRow->update(m_bodyRect);
		m_textRow->m_bounds.setSize(m_bodyRect.getSize());
		m_textRow->setPosition(m_bodyRect.getPosition());
		
		// Update cursor timing for active textbox
		if (m_isActive && this == s_activeTextBox) {
			float elapsedTime = m_cursorClock.getElapsedTime().asSeconds();
			// Calculate which half of the blink cycle we're in
			float cycleTime = fmod(elapsedTime, CURSOR_BLINK_INTERVAL * 2.0f);
			m_showCursor = (cycleTime < CURSOR_BLINK_INTERVAL);
		} else {
			m_showCursor = false;
		}
		
		// Update text display based on current state
		if (m_text) {
			std::string displayText;
			if (m_currentText.empty()) {
				if (m_isActive && this == s_activeTextBox) {
					// Show empty string when active and empty (cursor will be drawn separately)
					displayText = "";
				} else {
					// Show default text when not active and empty
					displayText = m_defaultText;
				}
			} else {
				if (hasStyle(m_style, TBStyle::Password)) {
					displayText = std::string(m_currentText.length(), '*');
				} else {
					displayText = m_currentText;
				}
			}
			m_text->setString(displayText);
			// Use normal text color when active (even if empty), fade default text when inactive
			sf::Color textColor = (m_currentText.empty() && !(m_isActive && this == s_activeTextBox)) ? 
				sf::Color(m_textColor.r, m_textColor.g, m_textColor.b, 128) : m_textColor;
			m_text->m_modifier.setColor(textColor);
		}
	}
}

inline void TextBox::render(sf::RenderTarget& target) {
	if (!m_modifier.isVisible()) return;
	
	if (hasStyle(m_style, TBStyle::Pill)) {
		m_leftCircle.setRadius(m_modifier.getFixedHeight() / 2);
		m_rightCircle.setRadius(m_modifier.getFixedHeight() / 2);

		m_leftCircle.setPosition(m_bounds.getPosition());
		m_rightCircle.setPosition({
			m_bounds.getPosition().x + m_bounds.getSize().x - (m_rightCircle.getRadius() * 2),
			m_bounds.getPosition().y
		});

		m_bodyRect.setSize({
			m_bounds.getSize().x - m_bounds.getSize().y,
			m_bounds.getSize().y
		});

		m_bodyRect.setPosition({
			m_bounds.getPosition().x + (m_bounds.getSize().y / 2),
			m_bounds.getPosition().y
		});

		m_leftCircle.setOutlineThickness((m_isActive ? m_bounds.getSize().y / 10 : 0));
		m_rightCircle.setOutlineThickness((m_isActive ? m_bounds.getSize().y / 10 : 0));
		m_bodyRect.setOutlineThickness((m_isActive ? m_bounds.getSize().y / 10 : 0));

		target.draw(m_leftCircle);
		target.draw(m_rightCircle);
		target.draw(m_bodyRect);

		if (m_isActive) {
			sf::RectangleShape cleanUpOutline;

			cleanUpOutline.setFillColor(m_modifier.getColor());

			cleanUpOutline.setSize({
				m_bounds.getSize().y / 10,
				m_bounds.getSize().y
			});

			cleanUpOutline.setPosition({
				m_bodyRect.getPosition().x - m_bounds.getSize().y / 10,
				m_bodyRect.getPosition().y
			});

			target.draw(cleanUpOutline);
			
			cleanUpOutline.setPosition({
				m_bodyRect.getPosition().x + m_bodyRect.getSize().x,
				m_bodyRect.getPosition().y
			});

			target.draw(cleanUpOutline);
		}
	}
	else {
		m_bodyRect.setSize(m_bounds.getSize());
		m_bodyRect.setPosition(m_bounds.getPosition());
		m_bodyRect.setOutlineThickness((m_isActive ? m_bounds.getSize().y / 10 : 0));
		target.draw(m_bodyRect);
	}
	
	// NOW setup scissor test for clipping text content to textbox bounds
	sf::FloatRect clipRect = m_bounds.getGlobalBounds();
	sf::View currentView = target.getView();
	
	// Convert world coordinates to pixel coordinates for scissor rect
	sf::Vector2f topLeft = {clipRect.position.x, clipRect.position.y};
	sf::Vector2f bottomRight = {clipRect.position.x + clipRect.size.x, 
	                             clipRect.position.y + clipRect.size.y};
	
	sf::Vector2i topLeftPixel = target.mapCoordsToPixel(topLeft, currentView);
	sf::Vector2i bottomRightPixel = target.mapCoordsToPixel(bottomRight, currentView);
	
	// SFML's scissor coordinates are from bottom-left, need to flip Y
	sf::Vector2u windowSize = target.getSize();
	int scissorX = topLeftPixel.x;
	int scissorY = windowSize.y - bottomRightPixel.y;  // Flip Y
	int scissorWidth = bottomRightPixel.x - topLeftPixel.x;
	int scissorHeight = bottomRightPixel.y - topLeftPixel.y;
	
	// Save current scissor state
	GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
	GLint oldScissor[4];
	if (scissorWasEnabled) {
		glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
		
		// Intersect with parent's scissor region
		int parentX = oldScissor[0];
		int parentY = oldScissor[1];
		int parentRight = parentX + oldScissor[2];
		int parentTop = parentY + oldScissor[3];
		
		int childRight = scissorX + scissorWidth;
		int childTop = scissorY + scissorHeight;
		
		// Calculate intersection
		scissorX = std::max(scissorX, parentX);
		scissorY = std::max(scissorY, parentY);
		int right = std::min(childRight, parentRight);
		int top = std::min(childTop, parentTop);
		
		scissorWidth = std::max(0, right - scissorX);
		scissorHeight = std::max(0, top - scissorY);
	}
	
	// Enable scissor test with new bounds
	glEnable(GL_SCISSOR_TEST);
	glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
	
	// Render text content (clipped)
	m_textRow->render(target);
	
	// Render cursor rectangle if active and should show cursor (clipped)
	if (m_isActive && this == s_activeTextBox && m_showCursor) {
		renderCursor(target);
	}
	
	// Restore previous scissor state
	if (scissorWasEnabled) {
		glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
}

inline bool TextBox::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (m_bounds.getGlobalBounds().contains(pos)) {
		// Deactivate any previously active textbox
		if (s_activeTextBox && s_activeTextBox != this) {
			s_activeTextBox->setActive(false);
		}
		// Activate this textbox
		setActive(true);
		s_activeTextBox = this;
		
		// Calculate cursor position from click coordinates
		if (m_text) {
			if (m_currentText.empty()) {
				// For empty text, cursor goes to position 0
				m_cursorPosition = 0;
			} else {
				// Convert click coordinates to text coordinate space
				float clickX = pos.x;
				size_t bestPosition = 0;
				
				// Get the text element's actual screen position
				sf::Vector2f textElementPos = m_text->m_bounds.getPosition();
				sf::Vector2f textElementSize = m_text->m_bounds.getSize();
				
				// Calculate where the text content actually starts (centered within text element)
				float textWidth = m_text->getTextWidth();
				float textStartX = textElementPos.x + (textElementSize.x - textWidth) / 2.0f;
				
				float minDistance = std::abs(clickX - (textStartX + m_text->getCharacterPosition(0).x));
				
				// Check each possible cursor position (including one past the end)
				for (size_t i = 0; i <= m_currentText.length(); ++i) {
					// Get relative character position and convert to screen coordinates
					sf::Vector2f relativeCharPos = m_text->getCharacterPosition(i);
					float screenCharX = textStartX + relativeCharPos.x;
					float distance = std::abs(clickX - screenCharX);
					
					if (distance < minDistance) {
						minDistance = distance;
						bestPosition = i;
					}
				}
				
				m_cursorPosition = bestPosition;
			}
		}
		
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------- //
// Image Implementation
// ---------------------------------------------------------------------------- //
inline Image::Image(
	Modifier modifier,
	sf::Image& image,
	const bool recolor,
	const std::string& name
) {
	m_modifier = modifier;
	m_image = std::make_unique<sf::Image>(image);

	// recolor all pixels with an opacity value > 0
	if (recolor) {
		for (unsigned int x = 0; x < m_image->getSize().x; x++) {
			for (unsigned int y = 0; y < m_image->getSize().y; y++) {
				sf::Color pixel = m_image->getPixel({x, y});
				if (pixel.a > 0) {
					sf::Color modColor = m_modifier.getColor();
					m_image->setPixel({x, y}, sf::Color(modColor.r, modColor.g, modColor.b, pixel.a));
				}
			}
		}
	}

	m_tex = std::make_unique<sf::Texture>();
	if (m_image->getSize().x > 0 && m_image->getSize().y > 0) {
		if (!m_tex->loadFromImage(*m_image))
			std::cerr << "[UILO] could not create image: " << m_image.get() << "\n";
	}
	m_tex->setSmooth(true);
	m_name = name;
}

inline void Image::setImage(sf::Image& image, bool recolor) {
	m_image = std::make_unique<sf::Image>(image);

	// recolor all pixels with an opacity value > 0
	if (recolor) {
		for (unsigned int x = 0; x < m_image->getSize().x; x++) {
			for (unsigned int y = 0; y < m_image->getSize().y; y++) {
				sf::Color pixel = m_image->getPixel({x, y});
				if (pixel.a > 0) {
					sf::Color modColor = m_modifier.getColor();
					m_image->setPixel({x, y}, sf::Color(modColor.r, modColor.g, modColor.b, pixel.a));
				}
			}
		}
	}

	m_tex = std::make_unique<sf::Texture>();
	if (m_image->getSize().x > 0 && m_image->getSize().y > 0) {
		if (!m_tex->loadFromImage(*m_image))
			std::cerr << "[UILO] could not create image: " << m_image.get() << "\n";
	}
	m_tex->setSmooth(true);
}

inline void Image::update(sf::RectangleShape& parentBounds) {
	Element::update(parentBounds);
	resize(parentBounds);
	applyModifiers();
}

inline void Image::render(sf::RenderTarget& target) {
	if (m_image.get() && m_tex.get() && m_tex->getSize().x > 0 && m_tex->getSize().y > 0) {
		sf::Sprite spr(*m_tex);

		spr.setScale({
			m_bounds.getSize().x / static_cast<float>(m_tex->getSize().x),
			m_bounds.getSize().y / static_cast<float>(m_tex->getSize().y)
		});

		spr.setPosition(m_bounds.getPosition());
		target.draw(spr);
	}
}

// ---------------------------------------------------------------------------- //
// Element Factory
// ---------------------------------------------------------------------------- //
inline Row* row(
	Modifier modifier = default_mod,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<Row>(modifier, elements, name); }

inline ScrollableRow* scrollableRow(
	Modifier modifier = default_mod,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<ScrollableRow>(modifier, elements, name); }

inline Column* column(
	Modifier modifier = default_mod,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<Column>(modifier, elements, name); }

inline ScrollableColumn* scrollableColumn(
	Modifier modifier = default_mod,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<ScrollableColumn>(modifier, elements, name); }

inline Grid* grid(
	Modifier modifier = default_mod,
	float cellWidth = 100.f,
	float cellHeight = 100.f,
	int columns = 0,
	int rows = 0,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<Grid>(modifier, cellWidth, cellHeight, columns, rows, elements, name); }

inline FreeColumn* freeColumn(
	Modifier modifier = default_mod,
	std::initializer_list<Element*> elements = {},
	const std::string& name = ""
) { return obj<FreeColumn>(modifier, elements, name); }

inline Spacer* spacer(
	Modifier modifier = default_mod,
	const std::string& name = ""
) { return obj<Spacer>(modifier, name); }

inline Button* button(
	Modifier modifier = default_mod,
	ButtonStyle style = ButtonStyle::Default,
	const std::string& text = "",
	const std::string& fontPath = "",
	sf::Color textColor = sf::Color::White,
	const std::string& name = ""
) { return obj<Button>(modifier, style, text, fontPath, textColor, name); }

inline Button* button(
	Modifier modifier,
	ButtonStyle style,
	const std::string& text,
	sf::Font& font,
	sf::Color textColor = sf::Color::White,
	const std::string& name = ""
) { return obj<Button>(modifier, style, text, font, textColor, name); }

inline Text* text(
	Modifier modifier = default_mod,
	const std::string& str = "",
	const std::string& fontPath = "",
	const std::string& name = ""
) { return obj<Text>(modifier, str, fontPath, name); }

inline Text* text(
	Modifier modifier,
	const std::string& str,
	sf::Font& font,
	const std::string& name = ""
) { return obj<Text>(modifier, str, font, name); }

inline Slider* slider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	SliderOrientation orientation = SliderOrientation::Vertical,
	float initialValue = 0.75f,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, orientation, initialValue, name); }

inline Slider* verticalSlider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	float initialValue = 0.75f,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, SliderOrientation::Vertical, initialValue, name); }

inline Slider* horizontalSlider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	float initialValue = 0.75f,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, SliderOrientation::Horizontal, initialValue, name); }

inline Knob* knob(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color trackColor = sf::Color(100, 100, 100),
	sf::Color arcColor = sf::Color(0, 150, 255),
	float initialValue = 0.5f,
	const std::string& name = ""
) { return obj<Knob>(modifier, knobColor, trackColor, arcColor, initialValue, name); }

inline Dropdown* dropdown(
	Modifier modifier = default_mod,
	const std::string& defaultText = "",
	const std::vector<std::string>& options = {},
	const std::string& textFont = "",
	sf::Color textColor = sf::Color::White,
	sf::Color optionBackgroundColor = sf::Color(50, 50, 50),
	const std::string& name = ""
) { return obj<Dropdown>(modifier, defaultText, options, textFont, textColor, optionBackgroundColor, name); }

inline TextBox* textBox(
	Modifier modifier = default_mod,
	TBStyle style = TBStyle::Default,
	const std::string& fontPath = "",
	const std::string& defaultText = "",
	sf::Color textColor = sf::Color::Black,
	sf::Color activeOutlineColor = sf::Color::Transparent,
	const std::string& name = ""
) { return obj<TextBox>(modifier, style, fontPath, defaultText, textColor, activeOutlineColor, name); }

inline TextBox* textBox(
	Modifier modifier,
	TBStyle style,
	sf::Font& font,
	const std::string& defaultText = "",
	sf::Color textColor = sf::Color::Black,
	sf::Color activeOutlineColor = sf::Color::Transparent,
	const std::string& name = ""
) { return obj<TextBox>(modifier, style, font, defaultText, textColor, activeOutlineColor, name); }

inline Image* image(
	Modifier modifier = default_mod,
	sf::Image& image = default_image,
	const bool recolor = false,
	const std::string& name = ""
) { return obj<Image>(modifier, image, recolor, name); }

inline static Row* default_row = row();
inline static Column* default_column = column();
inline static Spacer* default_spacer = obj<Spacer>(default_mod, "");
inline static Button* default_button = button();
inline static Text* default_text = text();
inline static Slider* default_slider = slider();


// ---------------------------------------------------------------------------- //
// Page Implementation
// ---------------------------------------------------------------------------- //
inline Page::Page(std::initializer_list<Container*> containers) {
	m_bounds.setFillColor(sf::Color::Transparent);

	for (const auto& c : containers)
		m_containers.push_back(c);
}

inline void Page::update(const sf::RectangleShape& parentBounds) {
	m_bounds = parentBounds;

	for (auto& c : m_containers) {
		if (c->m_modifier.isVisible()) {
			if (!(c->getType() == EType::FreeColumn)) {
				const float padding = c->m_modifier.getPadding();
				c->m_bounds.setPosition(m_bounds.getPosition() + sf::Vector2f(padding, padding));
			}
			c->update(m_bounds);
		}
	}
}

inline void Page::render(sf::RenderTarget& target) {
	target.draw(m_bounds);

	for (auto& c : m_containers) {
		if (c->m_modifier.isVisible())
			c->render(target);
	}

	for (auto& e : high_priority_elements)
		if (e->m_modifier.isVisible())
			e->render(target);
}

inline void Page::handleEvent(const sf::Event& event) {
	for (auto& c : m_containers)
		c->handleEvent(event);
}

inline bool Page::dispatchClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	// First, check all FreeColumns (for context menus, dropdowns, etc.) before bound-checking containers
	for (auto& c : m_containers) {
		if (c->getType() == EType::FreeColumn) {
			FreeColumn* freeCol = dynamic_cast<FreeColumn*>(c);
			if (freeCol && freeCol->m_modifier.isVisible()) {
				if (freeCol->getBounds().contains(pos)) {
					bool handled = freeCol->checkClick(pos, button);
					if (handled) return true;
				}
			}
		}
	}
	
	// Then, handle dropdown logic if any dropdown is open
	if (Dropdown::s_openDropdown) {
		Dropdown* openDropdown = Dropdown::s_openDropdown;
		
		// Check if click is on the dropdown options
		if (openDropdown->m_optionsColumn && openDropdown->m_optionsColumn->m_modifier.isVisible()) {
			if (openDropdown->m_optionsColumn->getBounds().contains(pos)) {
				// Click is on dropdown options - let the options handle it
				bool handled = openDropdown->m_optionsColumn->checkClick(pos, button);
				if (handled) return true;
			}
		}
		
		// Check if click is on the main dropdown button
		if (openDropdown->m_mainButton && openDropdown->m_mainButton->m_bounds.getGlobalBounds().contains(pos)) {
			bool handled = openDropdown->m_mainButton->checkClick(pos, button);
			return handled;
		}
		
		// Click is outside dropdown - close it and don't consume the click
		openDropdown->m_isOpen = false;
		if (openDropdown->m_optionsColumn) openDropdown->m_optionsColumn->m_modifier.setVisible(false);
		Dropdown::s_openDropdown = nullptr;
		openDropdown->m_isDirty = true;
		// Fall through to handle the click normally
	}
	
	// Store current active TextBox to potentially deactivate later
	TextBox* previousActiveTextBox = TextBox::s_activeTextBox;
	bool clickOnActiveTextBox = false;
	bool shouldCheckTextBoxDeactivation = true;
	if (previousActiveTextBox) {
		clickOnActiveTextBox = previousActiveTextBox->m_bounds.getGlobalBounds().contains(pos);
	}
	
	// Check high priority elements (excluding dropdown options which we handled above)
	for (auto& e : high_priority_elements) {
		if (e->m_modifier.isVisible()) {
			// Skip dropdown option columns as we handled them above
			if (e->getType() == EType::FreeColumn) {
				// Check if this is a dropdown options column
				bool isDropdownOptions = false;
				if (m_uilo) {
					for (const auto& [name, dropdown] : m_uilo->m_dropdowns) {
						if (dropdown->m_optionsColumn == e.get()) {
							isDropdownOptions = true;
							break;
						}
					}
				}
				if (isDropdownOptions) continue; // Skip, already handled above
			}
			
			bool handled = e->checkClick(pos, button);
			if (handled) {
				// Check TextBox deactivation before returning
				if (shouldCheckTextBoxDeactivation && previousActiveTextBox && !clickOnActiveTextBox) {
					previousActiveTextBox->setActive(false);
					if (TextBox::s_activeTextBox == previousActiveTextBox) {
						TextBox::s_activeTextBox = nullptr;
					}
				}
				return true;
			}
		}
	}
	
	// Then check regular containers with bounds checking
	for (auto& c : m_containers) {
		if (c->m_modifier.isVisible() && c->m_bounds.getGlobalBounds().contains(pos)) {
			bool handled = c->checkClick(pos, button);
			if (handled) {
				// Check TextBox deactivation before returning
				if (shouldCheckTextBoxDeactivation && previousActiveTextBox && !clickOnActiveTextBox) {
					previousActiveTextBox->setActive(false);
					if (TextBox::s_activeTextBox == previousActiveTextBox) {
						TextBox::s_activeTextBox = nullptr;
					}
				}
				return true;
			}
		}
	}
	
	// Handle TextBox deactivation: if we had an active TextBox and the click wasn't on it,
	// then deactivate it (unless a new TextBox was activated by the click)
	if (previousActiveTextBox && !clickOnActiveTextBox) {
		previousActiveTextBox->setActive(false);
		if (TextBox::s_activeTextBox == previousActiveTextBox) {
			TextBox::s_activeTextBox = nullptr;
		}
	}
	
	return false;
}

inline void Page::dispatchScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) {
	for (auto& c : m_containers)
		c->checkScroll(pos, verticalDelta, horizontalDelta);
}

inline void Page::dispatchHover(const sf::Vector2f& pos) {
	// Check high priority elements first (like dropdown options)
	for (auto& e : high_priority_elements)
		if (e->m_modifier.isVisible())
			e->checkHover(pos);
	
	// Then check regular containers
	for (auto& c : m_containers)
		c->checkHover(pos);
}

inline void Page::clear() {
	for (auto& c : m_containers) {
		c->clear();
		c->m_markedForDeletion = true;
	}
	m_containers.clear();
	cleanupMarkedElements();
}

inline void Page::setUilo(UILO* uilo) {
	m_uilo = uilo;
	for (auto& container : m_containers) {
		container->setUilo(uilo);
	}
}

inline std::unique_ptr<Page> page(std::initializer_list<Container*> containers = {}) {
	return std::make_unique<Page>(containers);
}

// ---------------------------------------------------------------------------- //
// UILO Implementation
// ---------------------------------------------------------------------------- //
inline UILO::UILO() {
	initDefaultView();
	initCursors();
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 4;

	m_window.create(
		m_defScreenRes, m_windowTitle,
		sf::Style::Resize | sf::Style::Close,
		sf::State::Windowed,
		settings
	);

	m_window.setVerticalSyncEnabled(true);
	m_window.setView(m_defaultView);

	if (m_window.isOpen()) m_running = true;

	m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::UILO(const std::string& windowTitle)
: m_windowTitle(windowTitle) {
	initDefaultView();
	initCursors();
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 4;

	m_window.create(
		m_defScreenRes, m_windowTitle,
		sf::Style::Resize | sf::Style::Close,
		sf::State::Windowed,
		settings
	);

	m_window.setVerticalSyncEnabled(true);
	m_window.setView(m_defaultView);

	if (m_window.isOpen()) m_running = true;
	m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::UILO(sf::RenderWindow& userWindow, sf::View& windowView) {
	initCursors();
	m_userWindow = &userWindow;
	m_windowOwned = false;
	
	initDefaultView();
	m_userWindow->setView(m_defaultView);

	if (m_userWindow->isOpen()) {
		m_running = true;
	}
	m_bounds.setFillColor(sf::Color::Transparent);
}

inline UILO::~UILO() {
	// m_ownedPages (vector of unique_ptr) will automatically clean up Pages.
	// Globally-owned elements are also managed by unique_ptr and will be cleaned
	// up at program exit, but we clear them here in case the UILO instance
	// is destroyed before main() exits.
	if (m_fullClean) {
		uilo_owned_elements.clear();
		high_priority_elements.clear();
	}
}

inline void UILO::_internal_update(sf::RenderWindow& target, sf::View& view) {
	pollEvents();

	static bool firstFrame = true;
	if (!firstFrame) {
		for (auto& [name, btn] : m_buttons) {
			btn->setClicked(false);
		}
	}
	firstFrame = false;

	sf::Vector2u currentSize = target.getSize();
	if (currentSize != m_lastWindowSize) {
		m_shouldUpdate = true;
		m_lastWindowSize = currentSize;
	}

	if (!m_shouldUpdate && m_cacheIsInitialized && m_elementBoundsCache.size() == uilo_owned_elements.size()) {
		for (size_t i = 0; i < uilo_owned_elements.size(); ++i) {
			const auto& element = uilo_owned_elements[i];
			const auto& cachedBounds = m_elementBoundsCache[i];
			if (element->m_bounds.getPosition() != cachedBounds.getPosition() ||
				element->m_bounds.getSize() != cachedBounds.getSize() ||
				element->m_isDirty) {
				m_shouldUpdate = true;
				break;
			}
		}
	} else if (!m_cacheIsInitialized || m_elementBoundsCache.size() != uilo_owned_elements.size()) {
		m_shouldUpdate = true;
	}

	if (m_shouldUpdate) {
		// Apply render scale to view size
		sf::Vector2f scaledSize = {
			(float)currentSize.x / m_renderScale,
			(float)currentSize.y / m_renderScale
		};
		view.setSize(scaledSize);
		
		// Ensure view's top-left corner is always at (0, 0)
		view.setCenter({scaledSize.x / 2.f, scaledSize.y / 2.f});
		
		m_bounds.setSize(view.getSize());
		// Bounds position reflects where the view shows (starts at 0,0 since view is centered at size/2)
		m_bounds.setPosition({0.f, 0.f});
		target.setView(view);

		if (m_currentPage) {
			for (int i = 0; i < 12; ++i)
				m_currentPage->update(m_bounds);
		}

		m_elementBoundsCache.clear();
		m_elementBoundsCache.reserve(uilo_owned_elements.size());
		for (const auto& elementPtr : uilo_owned_elements) {
			m_elementBoundsCache.push_back(elementPtr->m_bounds);
			elementPtr->m_isDirty = false;
		}
		m_cacheIsInitialized = true;
	}

	if (m_clickInfo) {
		if (m_currentPage && !m_inputBlocked) {
			m_currentPage->dispatchClick(m_clickInfo->first, m_clickInfo->second);
		}
		m_clickInfo.reset();
	}

	if (m_scrollPosition) {
		if (m_currentPage && !m_inputBlocked) {
			m_currentPage->dispatchScroll(*m_scrollPosition, m_verticalScrollDelta, m_horizontalScrollDelta);
		}
		m_scrollPosition.reset();
	}
}

inline void UILO::update() {
	sf::RenderWindow& target = m_windowOwned ? m_window : *m_userWindow;
	 
	// Update mouse position every frame using real-time mouse position
	m_mousePos = target.mapPixelToCoords(sf::Mouse::getPosition(target));
	 
	_internal_update(target, m_defaultView);
	 
	// Always check hover for the current mouse position on every frame
	if (m_currentPage && !m_inputBlocked) {
		m_currentPage->dispatchHover(m_mousePos);
	}
}

inline void UILO::update(sf::View& windowView) {
	if (m_windowOwned) return;
	 
	// Update mouse position every frame using real-time mouse position
	m_mousePos = m_userWindow->mapPixelToCoords(sf::Mouse::getPosition(*m_userWindow));
	 
	_internal_update(*m_userWindow, windowView);
	 
	// Always check hover for the current mouse position on every frame
	if (m_currentPage && !m_inputBlocked) {
		m_currentPage->dispatchHover(m_mousePos);
	}
}

inline void UILO::render() {
	if (m_shouldUpdate) {
		sf::RenderWindow& target = m_windowOwned ? m_window : *m_userWindow;

		if (m_windowOwned) {
			target.clear(sf::Color::Black);
			if (m_currentPage) m_currentPage->render(target);
			target.display();
		} else {
			if (m_currentPage) m_currentPage->render(target);
		}
	}
	m_shouldUpdate = false;
}

inline void UILO::setTitle(const std::string& newTitle) {
	if (m_windowOwned) m_window.setTitle(newTitle);
}

inline bool UILO::isRunning() const { return m_running; }
inline bool UILO::windowShouldUpdate() const { return m_shouldUpdate; }

inline void UILO::addPage(std::unique_ptr<Page> page, const std::string& name) {
	if (!page) return;
	Page* raw_page_ptr = page.get();
	
	// Set UILO instance on all elements in the page
	raw_page_ptr->setUilo(this);
	
	m_pages[name] = raw_page_ptr;
	m_ownedPages.push_back(std::move(page));
	if (!m_currentPage) {
		m_currentPage = raw_page_ptr;
	}
	m_cacheIsInitialized = false;
}

inline void UILO::switchToPage(const std::string& pageName) {
	auto it = m_pages.find(pageName);
	if (it != m_pages.end()) {
		if (m_currentPage != it->second) {
			m_currentPage = it->second;
			m_shouldUpdate = true;
		}
	} else {
		std::cerr << "[UILO] Page \"" << pageName << "\" not found.\n";
	}
}

inline void UILO::forceUpdate() {
	m_shouldUpdate = true;
	update();
}

inline void UILO::forceUpdate(sf::View& windowView) {
	m_shouldUpdate = true;
	update(windowView);
}

inline void UILO::setScale(float scale) {
	m_renderScale = scale;
	g_renderScale = scale; // Update global scale for viewport calculations
	initDefaultView();
	if (m_windowOwned) {
		m_window.setView(m_defaultView);
	} else if (m_userWindow) {
		// For user windows, we apply the scale but the user needs to use 
		// the parameterless update() method, or handle scaling in their own view
		m_userWindow->setView(m_defaultView);
	}
	// Force update to apply new scaling
	m_shouldUpdate = true;
}

inline float UILO::getScale() const { return m_renderScale; }

inline sf::Vector2f UILO::getMousePosition() const { return m_mousePos; }
inline float UILO::getVerticalScrollDelta() const { return m_verticalScrollDelta; }
inline float UILO::getHorizontalScrollDelta() const { return m_horizontalScrollDelta; }
inline void UILO::resetScrollDeltas() { 
	m_verticalScrollDelta = 0.f; 
	m_horizontalScrollDelta = 0.f; 
}

inline void UILO::setInputBlocked(bool blocked) { m_inputBlocked = blocked; }
inline bool UILO::isInputBlocked() const { return m_inputBlocked; }
inline bool UILO::isMouseDragging() const { return m_mouseDragging;}

// Element getters
inline Row* UILO::getRow(const std::string& name) {
	if (m_containers.count(name))
		return dynamic_cast<Row*>(m_containers[name]);
	else {
		std::cerr << "[UILO] Error: Row element \"" << name << "\" not found.\n";
		return default_row;
	}
}

inline Column* UILO::getColumn(const std::string& name) {
	if (m_containers.count(name))
		return dynamic_cast<Column*>(m_containers[name]);
	else {
		std::cerr << "[UILO] Error: Column element \"" << name << "\" not found.\n";
		return default_column;
	}
}

inline Spacer* UILO::getSpacer(const std::string& name) {
	if (m_spacers.count(name))
		return m_spacers[name];
	else {
		std::cerr << "[UILO] Error: Spacer element \"" << name << "\" not found.\n";
		return default_spacer;
	}
}

inline Button* UILO::getButton(const std::string& name) {
	if (m_buttons.count(name))
		return m_buttons[name];
	else {
		std::cerr << "[UILO] Error: Button element \"" << name << "\" not found.\n";
		return default_button;
	}
}

inline Text* UILO::getText(const std::string& name) {
	if (m_texts.count(name))
		return m_texts[name];
	else {
		std::cerr << "[UILO] Error: Text element \"" << name << "\" not found.\n";
		return default_text;
	}
}

inline Slider* UILO::getSlider(const std::string& name) {
	if (m_sliders.count(name))
		return m_sliders[name];
	else {
		std::cerr << "[UILO] Error: Slider element \"" << name << "\" not found.\n";
		return default_slider;
	}
}

inline Dropdown* UILO::getDropdown(const std::string& name) {
	if (m_dropdowns.count(name))
		return m_dropdowns[name];
	else {
		std::cerr << "[UILO] Error: Dropdown element \"" << name << "\" not found.\n";
		return nullptr;
	}
}

inline Grid* UILO::getGrid(const std::string& name) {
	if (m_grids.count(name))
		return m_grids[name];
	else {
		std::cerr << "[UILO] Error: Grid element \"" << name << "\" not found.\n";
		return nullptr;
	}
}

inline TextBox* UILO::getTextBox(const std::string& name) {
	if (m_textboxes.count(name))
		return m_textboxes[name];
	else {
		std::cerr << "[UILO] Error: TextBox element \"" << name << "\" not found.\n";
		return nullptr;
	}
}

inline void UILO::pollEvents() {
	sf::RenderWindow* activeWindow = m_windowOwned ? &m_window : m_userWindow;
	if (!activeWindow) return;

	while (const auto event = activeWindow->pollEvent()) {
		if (event->is<sf::Event::Closed>()) {
			activeWindow->close();
			m_running = false;
		}

		if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
			sf::Mouse::Button button = mousePressed->button;
			if (button == sf::Mouse::Button::Left || button == sf::Mouse::Button::Right) {
				m_clickInfo = { activeWindow->mapPixelToCoords(mousePressed->position), button };
				m_shouldUpdate = true;
				if (button == sf::Mouse::Button::Left) {
					m_mouseDragging = true;
				}
			}
		}

		if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
			m_mousePos = activeWindow->mapPixelToCoords(mouseMoved->position);
			
			// Handle slider dragging
			if (m_mouseDragging && m_activeDragSlider) {
				m_activeDragSlider->handleDrag(m_mousePos);
				m_shouldUpdate = true;
			}
			
			// Handle knob dragging
			if (m_mouseDragging && m_activeDragKnob) {
				m_activeDragKnob->handleDrag(m_mousePos);
				m_shouldUpdate = true;
			}
		}

		if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
			m_scrollPosition = activeWindow->mapPixelToCoords(mouseScrolled->position);
			
			// Check if shift is held for horizontal scrolling
			bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || 
							sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
			
			if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
				if (shiftHeld) {
					// Convert vertical scroll to horizontal when shift is held
					m_horizontalScrollDelta = mouseScrolled->delta;
					m_verticalScrollDelta = 0.f;
				} else {
					m_verticalScrollDelta = mouseScrolled->delta;
					m_horizontalScrollDelta = 0.f;
				}
			}
			else if (mouseScrolled->wheel == sf::Mouse::Wheel::Horizontal) {
				m_horizontalScrollDelta = mouseScrolled->delta;
				m_verticalScrollDelta = 0.f;
			}
			m_shouldUpdate = true;
		}

		if (event->getIf<sf::Event::MouseButtonReleased>()) {
			m_mouseDragging = false;
			if (m_activeDragSlider) {
				m_activeDragSlider->m_isDragging = false;
				m_activeDragSlider = nullptr;
			}
			if (m_activeDragKnob) {
				m_activeDragKnob->m_isDragging = false;
				m_activeDragKnob = nullptr;
			}
		}

		// Handle text input for active TextBox
		if (TextBox::s_activeTextBox && TextBox::s_activeTextBox->isActive()) {
			if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
				char inputChar = static_cast<char>(textEntered->unicode);
				// Only accept printable characters (ASCII 32-126)
				if (inputChar >= 32 && inputChar < 127) {
					TextBox::s_activeTextBox->insertAtCursor(inputChar);
					m_shouldUpdate = true;
				}
			}
			
			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
				if (keyPressed->code == sf::Keyboard::Key::Backspace) {
					TextBox::s_activeTextBox->backspaceAtCursor();
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::Delete) {
					TextBox::s_activeTextBox->deleteAtCursor();
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::Left) {
					TextBox::s_activeTextBox->moveCursorLeft();
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::Right) {
					TextBox::s_activeTextBox->moveCursorRight();
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::Home) {
					TextBox::s_activeTextBox->setCursorPosition(0);
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::End) {
					TextBox::s_activeTextBox->setCursorPosition(TextBox::s_activeTextBox->getText().length());
					m_shouldUpdate = true;
				}
				else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Escape) {
					// Deactivate textbox on Enter or Escape
					TextBox::s_activeTextBox->setActive(false);
					TextBox::s_activeTextBox = nullptr;
					m_shouldUpdate = true;
				}
			}
		}
	}
}

inline void UILO::initDefaultView() {
	m_defScreenRes = sf::VideoMode::getDesktopMode();
	m_defScreenRes.size.x /= m_renderScale;
	m_defScreenRes.size.y /= m_renderScale;

	m_defaultView.setSize({
		(float)m_defScreenRes.size.x,
		(float)m_defScreenRes.size.y
	});
	
	// Ensure view's top-left corner is at (0, 0)
	m_defaultView.setCenter({
		(float)m_defScreenRes.size.x / 2.f,
		(float)m_defScreenRes.size.y / 2.f
	});
}

inline void UILO::setView(const sf::View& view) {
	m_defaultView = view;
	
	// Normalize the view to ensure its top-left corner is at (0, 0)
	sf::Vector2f viewSize = m_defaultView.getSize();
	m_defaultView.setCenter({viewSize.x / 2.f, viewSize.y / 2.f});
	
	if (!m_windowOwned)
		m_userWindow->setView(m_defaultView);
}

// ---------------------------------------------------------------------------- //
// Cursor Management Implementation
// ---------------------------------------------------------------------------- //
inline void UILO::initCursors() {
	// SFML cursors must be created using loadFromSystem
	// We'll create them on-demand in setCursor instead
}

inline void UILO::setCursor(CursorType cursorType) {
	if (m_currentCursorType == cursorType) return; // Already set
	
	m_currentCursorType = cursorType;
	sf::RenderWindow* activeWindow = m_windowOwned ? &m_window : m_userWindow;
	if (!activeWindow) return;
	
	auto ensureCursor = [](std::optional<sf::Cursor>& cursor, sf::Cursor::Type type) -> sf::Cursor* {
		if (!cursor.has_value())
			cursor = sf::Cursor::createFromSystem(type);
		return cursor.has_value() ? &cursor.value() : nullptr;
	};
	
	sf::Cursor* cursorPtr = nullptr;
	switch (cursorType) {
		case CursorType::Arrow:
			cursorPtr = ensureCursor(m_arrowCursor, sf::Cursor::Type::Arrow);
			break;
		case CursorType::Hand:
			cursorPtr = ensureCursor(m_handCursor, sf::Cursor::Type::Hand);
			break;
		case CursorType::IBeam:
			cursorPtr = ensureCursor(m_iBeamCursor, sf::Cursor::Type::Text);
			break;
		case CursorType::SizeHorizontal:
			cursorPtr = ensureCursor(m_sizeHorizontalCursor, sf::Cursor::Type::SizeHorizontal);
			break;
		case CursorType::SizeVertical:
			cursorPtr = ensureCursor(m_sizeVerticalCursor, sf::Cursor::Type::SizeVertical);
			break;
		case CursorType::SizeNWSE:
			cursorPtr = ensureCursor(m_sizeNWSECursor, sf::Cursor::Type::SizeTopLeftBottomRight);
			break;
		case CursorType::SizeNESW:
			cursorPtr = ensureCursor(m_sizeNESWCursor, sf::Cursor::Type::SizeBottomLeftTopRight);
			break;
		case CursorType::SizeAll:
			cursorPtr = ensureCursor(m_sizeAllCursor, sf::Cursor::Type::SizeAll);
			break;
		case CursorType::Cross:
			cursorPtr = ensureCursor(m_crossCursor, sf::Cursor::Type::Cross);
			break;
		case CursorType::NotAllowed:
			cursorPtr = ensureCursor(m_notAllowedCursor, sf::Cursor::Type::NotAllowed);
			break;
	}
	
	if (cursorPtr) {
		activeWindow->setMouseCursor(*cursorPtr);
	}
}

inline CursorType UILO::getCurrentCursor() const {
	return m_currentCursorType;
}

inline void UILO::resetCursor() {
	setCursor(CursorType::Arrow);
}

// ---------------------------------------------------------------------------- //
// Alignment Helpers
// ---------------------------------------------------------------------------- //
inline Align operator|(Align lhs, Align rhs) {
	return static_cast<Align>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline Align operator&(Align lhs, Align rhs) {
	return static_cast<Align>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasAlign(Align value, Align flag) {
	return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

// ---------------------------------------------------------------------------- //
// TextBox Style Helpers
// ---------------------------------------------------------------------------- //
inline TBStyle operator|(TBStyle lhs, TBStyle rhs) {
	return static_cast<TBStyle>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline TBStyle operator&(TBStyle lhs, TBStyle rhs) {
	return static_cast<TBStyle>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool hasStyle(TBStyle value, TBStyle flag) {
	return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

// ---------------------------------------------------------------------------- //
// Helper Functions for UI Element Creation
// ---------------------------------------------------------------------------- //

using contains = std::initializer_list<uilo::Element*>;

} // !namespace uilo

#endif // !UILO_HPP