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
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <optional>
#include <memory> // Added for std::unique_ptr
#include <fstream> // For std::ifstream

namespace uilo {

// ---------------------------------------------------------------------------- //
// Forward Declarations
// ---------------------------------------------------------------------------- //
class Element;
class Container;
class Row;
class Column;
class Page;
class UILO;
class Slider;
class Text;
class Spacer;
class Button;
class Dropdown;

// ---------------------------------------------------------------------------- //
// Global Ownership
// ---------------------------------------------------------------------------- //
inline static std::vector<std::unique_ptr<Element>> uilo_owned_elements;
inline static std::vector<std::unique_ptr<Element>> high_priority_elements;

inline static std::unordered_map<std::string, Slider*> sliders;
inline static std::unordered_map<std::string, Container*> containers;
inline static std::unordered_map<std::string, Text*> texts;
inline static std::unordered_map<std::string, Spacer*> spacers;
inline static std::unordered_map<std::string, Button*> buttons;
inline static std::unordered_map<std::string, Dropdown*> dropdowns;

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
};

inline TBStyle operator|(TBStyle lhs, TBStyle rhs);
inline TBStyle operator&(TBStyle lhs, TBStyle rhs);
inline bool hasStyle(TBStyle value, TBStyle flag);

// ---------------------------------------------------------------------------- //
// Modifier
// ---------------------------------------------------------------------------- //
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

private:
	float m_widthPct = 1.f;
	float m_heightPct = 1.f;
	float m_fixedWidth = 0.f;
	float m_fixedHeight = 0.f;
	bool m_isVisible = true;
	bool m_highPriority = false;
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

	std::string m_name = "";

	Element();
	virtual ~Element();

	virtual void update(sf::RectangleShape& parentBounds);
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
	const std::vector<Element*>& getElements() const;
	virtual void checkScroll(const sf::Vector2f& pos, const float verticalDelta, const float horizontalDelta) override {};
	void clear();

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
// Free Column Container
// ---------------------------------------------------------------------------- //
class FreeColumn : public Column {
public:
	sf::Vector2f m_customPosition;

	using Column::Column;
	using Column::render;
	using Column::checkClick;
	using Column::checkHover;

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

	void update(sf::RectangleShape& parentBounds) override;
	void render (sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;
	void checkHover(const sf::Vector2f& pos) override;

	void setText(const std::string& newStr);
	std::string getText() const;

	bool isClicked() const { return m_isClicked; }
	bool isHovered() const { return m_isHovered; }

	void setClicked(bool clicked) { m_isClicked = clicked; }

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
		const std::string& name = ""
	);

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;

	float getValue() const;
	void setValue(float newVal);

private:
	float m_minVal = 0.f;
	float m_maxVal = 1.f;
	float m_curVal = 0.75f;

	sf::Color m_knobColor = sf::Color::White;
	sf::Color m_barColor = sf::Color::Black;
	SliderOrientation m_orientation = SliderOrientation::Vertical;

	sf::RectangleShape m_knobRect;
	sf::RectangleShape m_barRect;
};

// ---------------------------------------------------------------------------- //
// Dropdown Element
// ---------------------------------------------------------------------------- //
class Dropdown : public Element {
public:
	Dropdown(
		Modifier modifier = default_mod,
		const std::string& defaultText = "",
		std::initializer_list<std::string> options = {},
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

	std::string getSelected() const;

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
	~TextBox();

	void update(sf::RectangleShape& parentBounds) override;
	void render(sf::RenderTarget& target) override;
	bool checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) override;

	inline bool isActive() const { return m_isActive; }
	inline void setActive(bool active) { m_isActive = active; }

private:
	TBStyle m_style = TBStyle::Default;
	sf::RectangleShape m_bodyRect;

	// if hasStyle(TBStyle::Pill)
	sf::CircleShape m_leftCircle;
	sf::CircleShape m_rightCircle;

	std::unique_ptr<Text> m_text;
	std::unique_ptr<Row> m_textRow;

	// if hasStyle(TBStyle::Wrap)
	std::unique_ptr<ScrollableColumn> m_textColumn;

	std::string m_defaultText = "Default Text";
	std::string m_currentText = "";
	sf::Color m_textColor = sf::Color::Black;

	bool m_isActive = false;

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

private:
	std::vector<Container*> m_containers;
	sf::RectangleShape m_bounds;
};

// ---------------------------------------------------------------------------- //
// UILO Application Core
// ---------------------------------------------------------------------------- //
class UILO {
public:
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
	sf::Vector2f getMousePosition() const;
	float getVerticalScrollDelta() const;
	float getHorizontalScrollDelta() const;
	void resetScrollDeltas();

	void setInputBlocked(bool blocked);
	bool isInputBlocked() const;
	bool isMouseDragging() const;
	inline void setFullClean(bool doFullClean) { m_fullClean = doFullClean; }

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
	Page* m_currentPage = nullptr;

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

	void pollEvents();
	void initDefaultView();
	void setView(const sf::View& view);
	void _internal_update(sf::RenderWindow& target, sf::View& view);
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
	if (m_modifier.getFixedWidth() != 0) {
		m_bounds.setSize({
			m_modifier.getFixedWidth(),
			m_bounds.getSize().y
		});
	}
	else {
		if (inSlot)
			m_bounds.setSize({
				parent.getSize().x,
				m_bounds.getSize().y
			});
		else
			m_bounds.setSize({
				m_modifier.getWidth() * parent.getSize().x,
				m_bounds.getSize().y
			});
	}

	if (m_modifier.getFixedHeight() != 0) {
		m_bounds.setSize({
			m_bounds.getSize().x,
			m_modifier.getFixedHeight()
		});
	}
	else {
		if (inSlot)
			m_bounds.setSize({
				m_bounds.getSize().x,
				parent.getSize().y
			});
		else
			m_bounds.setSize({
				m_bounds.getSize().x,
				m_modifier.getHeight() * parent.getSize().y
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

	if (!m_name.empty())
		containers[m_name] = this;
}

inline Container::Container(Modifier modifier, std::initializer_list<Element*> elements, const std::string& name) {
	m_modifier = modifier;

	for (auto& e : elements)
		m_elements.push_back(e);

	m_name = name;

	if (!m_name.empty())
		containers[m_name] = this;
}

inline Container::~Container() {}

inline void Container::addElement(Element* element) { m_elements.push_back(element); }

inline void Container::addElements(std::initializer_list<Element*> elements) {
	for (auto& e : elements)
		m_elements.push_back(e);
}

inline void Container::handleEvent(const sf::Event& event) {
	for (auto& e : m_elements)
		e->handleEvent(event);
	Element::handleEvent(event);
}

inline const std::vector<Element*>& Container::getElements() const { return m_elements; }

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

inline void cleanupMarkedElements() {
	auto it = uilo_owned_elements.begin();
	while (it != uilo_owned_elements.end()) {
		Element* element = it->get();
		if (element->m_markedForDeletion) {
			if (!element->m_name.empty()) {
				auto buttonIt = buttons.find(element->m_name);
				if (buttonIt != buttons.end() && buttonIt->second == element)
					buttons.erase(buttonIt);

				auto sliderIt = sliders.find(element->m_name);
				if (sliderIt != sliders.end() && sliderIt->second == element)
					sliders.erase(sliderIt);

				auto textIt = texts.find(element->m_name);
				if (textIt != texts.end() && textIt->second == element)
					texts.erase(textIt);

				auto spacerIt = spacers.find(element->m_name);
				if (spacerIt != spacers.end() && spacerIt->second == element)
					spacers.erase(spacerIt);

				auto containerIt = containers.find(element->m_name);
				if (containerIt != containers.end() && containerIt->second == element)
					containers.erase(containerIt);
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
	applyModifiers();
	Element::update(parentBounds);

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
			
			if (hasAlign(alignment, Align::RIGHT)) {
				right.push_back(e);
				rightWidth += elementWidth;
			} else if (hasAlign(alignment, Align::CENTER_X)) {
				center.push_back(e);
				centerWidth += elementWidth;
			} else {
				left.push_back(e);
				leftWidth += elementWidth;
			}
		}
	}

	// Position elements efficiently
	const sf::Vector2f boundsPos = m_bounds.getPosition();
	const float boundsWidth = boundsSize.x;
	
	// Left aligned elements
	float xPos = boundsPos.x;
	for (const auto& e : left) {
		e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x;
	}

	// Center aligned elements
	xPos = boundsPos.x + (boundsWidth - centerWidth) * 0.5f;
	for (const auto& e : center) {
		e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x;
	}

	// Right aligned elements
	xPos = boundsPos.x + boundsWidth - rightWidth;
	for (const auto& e : right) {
		e->m_bounds.setPosition({xPos, e->m_bounds.getPosition().y});
		xPos += e->m_bounds.getSize().x;
	}

	// Apply vertical alignment in final pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			sf::Vector2f pos = e->m_bounds.getPosition();

			if (hasAlign(alignment, Align::CENTER_Y)) {
				pos.y = boundsPos.y + (boundsSize.y - e->m_bounds.getSize().y) * 0.5f;
			} else if (hasAlign(alignment, Align::BOTTOM)) {
				pos.y = boundsPos.y + boundsSize.y - e->m_bounds.getSize().y;
			} else {
				pos.y = boundsPos.y;
			}

			e->m_bounds.setPosition(pos);
		}
	}
}

inline void Row::render(sf::RenderTarget& target) {
	target.draw(m_bounds);

	// Render normal priority elements first
	for (auto& e : m_elements)
		if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
			e->render(target);

	// Render custom geometry (clipped by parent view)
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
		sf::View originalView = target.getView();
		sf::FloatRect clipRect = m_bounds.getGlobalBounds();
		sf::View clippingView(clipRect);

		sf::Vector2f worldPos = {clipRect.position.x, clipRect.position.y};
		sf::Vector2i pixelPos = target.mapCoordsToPixel(worldPos, originalView);

		sf::Vector2u windowSize = target.getSize();
		sf::FloatRect viewport(
			{static_cast<float>(pixelPos.x) / windowSize.x,
			 static_cast<float>(pixelPos.y) / windowSize.y},
			{clipRect.size.x / windowSize.x,
			 clipRect.size.y / windowSize.y}
		);

		clippingView.setViewport(viewport);
		target.setView(clippingView);

		target.draw(m_bounds);

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

		target.setView(originalView);
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
				
				// Only update if visible in container - optimized intersection check
				const sf::FloatRect elementBounds = e->m_bounds.getGlobalBounds();
				if (containerBounds.findIntersection(elementBounds).has_value()) {
					e->update(e->m_bounds);
				}
			}
		}

		// Boundary checking with cached positions
		if (firstElement && lastElement) {
			const float containerLeft = m_bounds.getPosition().x;
			const float lastElementRight = lastElement->m_bounds.getPosition().x;
			const float firstElementLeft = firstElement->m_bounds.getPosition().x;
			
			if (lastElementRight <= containerLeft) {
				m_offset += containerLeft - lastElementRight;
			} else if (firstElementLeft >= containerLeft) {
				m_offset -= firstElementLeft - containerLeft;
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
	applyModifiers();
	Element::update(parentBounds);

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

			if (hasAlign(alignment, Align::BOTTOM)) {
				bottom.push_back(e);
				bottomHeight += elementHeight;
			} else if (hasAlign(alignment, Align::CENTER_Y)) {
				center.push_back(e);
				centerHeight += elementHeight;
			} else {
				top.push_back(e);
				topHeight += elementHeight;
			}
		}
	}

	// Position elements efficiently
	const sf::Vector2f boundsPos = m_bounds.getPosition();
	const float boundsHeight = boundsSize.y;

	// Top aligned elements
	float yPos = boundsPos.y;
	for (const auto& e : top) {
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
		yPos += e->m_bounds.getSize().y;
	}

	// Center aligned elements
	yPos = boundsPos.y + (boundsHeight - centerHeight) * 0.5f;
	for (const auto& e : center) {
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
		yPos += e->m_bounds.getSize().y;
	}

	// Bottom aligned elements
	yPos = boundsPos.y + boundsHeight - bottomHeight;
	for (const auto& e : bottom) {
		e->m_bounds.setPosition({e->m_bounds.getPosition().x, yPos});
		yPos += e->m_bounds.getSize().y;
	}

	// Apply horizontal alignment in final pass
	for (const auto& e : m_elements) {
		if (e->m_modifier.isVisible()) {
			const Align alignment = e->m_modifier.getAlignment();
			sf::Vector2f pos = e->m_bounds.getPosition();

			if (hasAlign(alignment, Align::CENTER_X)) {
				pos.x = boundsPos.x + (boundsSize.x - e->m_bounds.getSize().x) * 0.5f;
			} else if (hasAlign(alignment, Align::RIGHT)) {
				pos.x = boundsPos.x + boundsSize.x - e->m_bounds.getSize().x;
			} else {
				pos.x = boundsPos.x;
			}

			e->m_bounds.setPosition(pos);
		}
	}
}

inline void Column::render(sf::RenderTarget& target) {
	if (getType() == EType::ScrollableColumn) {
		sf::View originalView = target.getView();
		sf::FloatRect clipRect = m_bounds.getGlobalBounds();
		sf::View clippingView(clipRect);

		sf::Vector2f worldPos = {clipRect.position.x, clipRect.position.y};
		sf::Vector2i pixelPos = target.mapCoordsToPixel(worldPos, originalView);

		sf::Vector2u windowSize = target.getSize();
		sf::FloatRect viewport(
			{static_cast<float>(pixelPos.x) / windowSize.x,
			 static_cast<float>(pixelPos.y) / windowSize.y},
			{clipRect.size.x / windowSize.x,
			 clipRect.size.y / windowSize.y}
		);

		clippingView.setViewport(viewport);
		target.setView(clippingView);

		target.draw(m_bounds);
		 
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

		target.setView(originalView);
	}
	else {
		target.draw(m_bounds);
		 
		// Render normal priority elements first
		for (auto& e : m_elements)
			if (e->m_modifier.isVisible() && e->m_doRender && !e->m_modifier.isHighPriority())
				e->render(target);

		// Render custom geometry (not clipped)
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
				
				// Only update if visible in container - optimized intersection check
				const sf::FloatRect elementBounds = e->m_bounds.getGlobalBounds();
				const bool isVisible = containerBounds.findIntersection(elementBounds).has_value();
				e->m_doRender = isVisible;
				
				if (isVisible) {
					e->update(e->m_bounds);
				}
			}
		}

		// Boundary checking with cached positions
		if (firstElement && lastElement) {
			const float containerTop = m_bounds.getPosition().y;
			const float lastElementBottom = lastElement->m_bounds.getPosition().y;
			const float firstElementTop = firstElement->m_bounds.getPosition().y;
			
			if (lastElementBottom <= containerTop) {
				m_offset += containerTop - lastElementBottom;
			} else if (firstElementTop >= containerTop) {
				m_offset -= firstElementTop - containerTop;
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

	if (!m_name.empty())
		texts[m_name] = this;
}

inline Text::Text(Modifier modifier, const std::string& str, const std::string& fontPath, const std::string& name)
: m_string(str) {
	m_modifier = modifier;
	m_name = name;
	if (!m_name.empty())
		texts[m_name] = this;

	if (!fontPath.empty())
		if (!m_font.openFromFile(fontPath))
			std::cerr << "Text Error: couldn't load font \"" << fontPath << "\".\n";
}

inline void Text::update(sf::RectangleShape& parentBounds) {
	resize(parentBounds);

	m_bounds.setPosition(parentBounds.getPosition());
	float fontSize = m_modifier.getFixedHeight() > 0
		? m_modifier.getFixedHeight()
		: m_bounds.getSize().y * 0.8f; // Use 80% of height for better fit

	m_text.emplace(m_font, m_string);
	m_text->setCharacterSize(static_cast<unsigned>(fontSize));
	m_text->setFillColor(m_modifier.getColor());

	// Get proper text bounds for width calculation
	sf::FloatRect textBounds = m_text->getLocalBounds();
	// Use just the width, ignoring the left bearing position
	float textWidth = textBounds.size.x;
	m_bounds.setSize({ textWidth, m_bounds.getSize().y });

	Element::update(parentBounds);
}

inline void Text::render(sf::RenderTarget& target) {
	if (!m_text) return;

	// Calculate proper center position
	float centerX = m_bounds.getPosition().x + m_bounds.getSize().x / 2.f;
	float centerY = m_bounds.getPosition().y + m_bounds.getSize().y / 2.f;
	
	// Get text bounds for centering
	sf::FloatRect localBounds = m_text->getLocalBounds();

	// Use the actual text bounds for proper centering
	m_text->setOrigin({
		localBounds.position.x + localBounds.size.x / 2.f,
		localBounds.position.y + localBounds.size.y / 2.f
	});

	// Position the text at the calculated center
	m_text->setPosition({centerX, centerY});

	target.draw(*m_text);
}

inline void Text::setString(const std::string& newStr) {
	m_string = newStr;
	if (m_text)
		m_text->setString(newStr);
	m_isDirty = true;
}

// ---------------------------------------------------------------------------- //
// Spacer Implementation
// ---------------------------------------------------------------------------- //
inline Spacer::Spacer(Modifier& modifier, const std::string& name) {
	m_modifier = modifier;
	m_name = name;

	if (!m_name.empty())
		spacers[m_name] = this;
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

	if (!textFont.empty()) {
		m_text = std::make_unique<Text>(
			Modifier()
				.setColor(textColor)
				.align(Align::CENTER_Y | Align::CENTER_X)
				.setHeight(0.8f),
			buttonText,
			textFont,
			""
		);

		m_textRow = std::make_unique<Row>(
			Modifier().setColor(sf::Color::Transparent).setHeight(1.f).setWidth(1.f),
			std::initializer_list<Element*>{ m_text.get() },
			""
		);
	}

	m_name = name;
	if (!m_name.empty()) {
		buttons[m_name] = this;
	}
}

inline void Button::update(sf::RectangleShape& parentBounds) {
	Element::update(parentBounds);
	resize(parentBounds);
	
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
		m_text->setString(newStr);
}

inline std::string Button::getText() const {
	if (m_text)
		return m_text->getString();
	return "";
}

// ---------------------------------------------------------------------------- //
// Slider Implementation
// ---------------------------------------------------------------------------- //
inline Slider::Slider(
	Modifier modifier,
	sf::Color knobColor,
	sf::Color barColor,
	SliderOrientation orientation,
	const std::string& name
) : m_knobColor(knobColor), m_barColor(barColor), m_orientation(orientation) {
	m_modifier = modifier;
	m_knobRect.setFillColor(m_knobColor);
	m_barRect.setFillColor(m_barColor);

	m_name = name;
	if (!m_name.empty()) {
		sliders[m_name] = this;
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
}

inline bool Slider::checkClick(const sf::Vector2f& pos, sf::Mouse::Button button) {
	if (button == sf::Mouse::Button::Left && m_bounds.getGlobalBounds().contains(pos)) {
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
		m_curVal = v;

		return true;
	}

	return false;
}

inline float Slider::getValue() const { return m_curVal; }

inline void Slider::setValue(float newVal) {
	m_curVal = newVal < m_minVal ? m_minVal : (newVal > m_maxVal ? m_maxVal : newVal);
}

// ---------------------------------------------------------------------------- //
// Dropdown Implementation
// ---------------------------------------------------------------------------- //
inline Dropdown::Dropdown(
	Modifier modifier,
	const std::string& defaultText,
	std::initializer_list<std::string> options,
	const std::string& textFont,
	sf::Color textColor,
	sf::Color optionBackgroundColor,
	const std::string& name
) {
	m_modifier = modifier;
	m_name = name;
	if (!m_name.empty()) {
		dropdowns[m_name] = this;
	}
	m_selectedOption = defaultText;

	m_mainButton = obj<Button>(modifier, ButtonStyle::Rect, m_selectedOption, textFont, textColor, "");
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
			.align(Align::CENTER_Y)
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
}

inline TextBox::~TextBox() {}

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
		
		m_textRow->render(target);
	}
	else {
		m_bodyRect.setSize(m_bounds.getSize());
		m_bodyRect.setPosition(m_bounds.getPosition());
		m_bodyRect.setOutlineThickness((m_isActive ? m_bounds.getSize().y / 10 : 0));
		target.draw(m_bodyRect);
		m_textRow->render(target);
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

inline Text* text(
	Modifier modifier = default_mod,
	const std::string& str = "",
	const std::string& fontPath = "",
	const std::string& name = ""
) { return obj<Text>(modifier, str, fontPath, name); }

inline Slider* slider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	SliderOrientation orientation = SliderOrientation::Vertical,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, orientation, name); }

inline Slider* verticalSlider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, SliderOrientation::Vertical, name); }

inline Slider* horizontalSlider(
	Modifier modifier = default_mod,
	sf::Color knobColor = sf::Color::White,
	sf::Color barColor = sf::Color::Black,
	const std::string& name = ""
) { return obj<Slider>(modifier, knobColor, barColor, SliderOrientation::Horizontal, name); }

inline Dropdown* dropdown(
	Modifier modifier = default_mod,
	const std::string& defaultText = "",
	std::initializer_list<std::string> options = {},
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
// Global Element Getters
// ---------------------------------------------------------------------------- //
inline Row* getRow(const std::string& name) {
	if (containers.count(name))
		return dynamic_cast<Row*>(containers[name]);
	else {
		std::cerr << "[UILO] Error: Row element \"" << name << "\" not found.\n";
		return default_row;
	}
}

inline Column* getColumn(const std::string& name) {
	if (containers.count(name))
		return dynamic_cast<Column*>(containers[name]);
	else {
		std::cerr << "[UILO] Error: Column element \"" << name << "\" not found.\n";
		return default_column;
	}
}

inline Spacer* getSpacer(const std::string& name) {
	if (spacers.count(name))
		return spacers[name];
	else {
		std::cerr << "[UILO] Error: Spacer element \"" << name << "\" not found.\n";
		return default_spacer;
	}
}

inline Button* getButton(const std::string& name) {
	if (buttons.count(name))
		return buttons[name];
	else {
		std::cerr << "[UILO] Error: Button element \"" << name << "\" not found.\n";
		return default_button;
	}
}

inline Text* getText(const std::string& name) {
	if (texts.count(name))
		return texts[name];
	else {
		std::cerr << "[UILO] Error: Text element \"" << name << "\" not found.\n";
		return default_text;
	}
}

inline Slider* getSlider(const std::string& name) {
	if (sliders.count(name))
		return sliders[name];
	else {
		std::cerr << "[UILO] Error: Slider element \"" << name << "\" not found.\n";
		return default_slider;
	}
}

inline Dropdown* getDropdown(const std::string& name) {
	if (dropdowns.count(name))
		return dropdowns[name];
	else {
		std::cerr << "[UILO] Error: Dropdown element \"" << name << "\" not found.\n";
		return nullptr; // Or a default dropdown if you create one
	}
}

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
			if (!(c->getType() == EType::FreeColumn))
				c->m_bounds.setPosition(m_bounds.getPosition());
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
	// First, handle dropdown logic if any dropdown is open
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
				for (const auto& [name, dropdown] : dropdowns) {
					if (dropdown->m_optionsColumn == e.get()) {
						isDropdownOptions = true;
						break;
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

inline std::unique_ptr<Page> page(std::initializer_list<Container*> containers = {}) {
	return std::make_unique<Page>(containers);
}

// ---------------------------------------------------------------------------- //
// UILO Implementation
// ---------------------------------------------------------------------------- //
inline UILO::UILO() {
	initDefaultView();
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
		for (auto& [name, btn] : uilo::buttons) {
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
		m_bounds.setSize(view.getSize());
		m_bounds.setPosition({
			view.getCenter().x - view.getSize().x * 0.5f,
			view.getCenter().y - view.getSize().y * 0.5f
		});
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
		}

		if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
			m_scrollPosition = activeWindow->mapPixelToCoords(mouseScrolled->position);
			if (mouseScrolled->wheel == sf::Mouse::Wheel::Vertical) {
				m_verticalScrollDelta = mouseScrolled->delta;
				m_horizontalScrollDelta = 0.f;
			}
			else if (mouseScrolled->wheel == sf::Mouse::Wheel::Horizontal) {
				m_horizontalScrollDelta = mouseScrolled->delta;
				m_verticalScrollDelta = 0.f;
			}
			m_shouldUpdate = true;
		}

		if (event->getIf<sf::Event::MouseButtonReleased>()) {
			m_mouseDragging = false;
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
}

inline void UILO::setView(const sf::View& view) {
	m_defaultView = view;
	if (!m_windowOwned)
		m_userWindow->setView(m_defaultView);
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

using contains = std::initializer_list<uilo::Element*>;

} // !namespace uilo

#endif // !UILO_HPP