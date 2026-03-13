#pragma once

#include <functional>
#include "../utils/Color.hpp"
#include "../utils/Alignment.hpp"

using funcPtr = std::function<void()>;

namespace uilo {

class Modifier {
public:
	Modifier() = default;   
	Modifier& setWidth(float pct);
	Modifier& setHeight(float pct);
	Modifier& setFixedWidth(float px);
	Modifier& setFixedHeight(float px);
	Modifier& align(Align alignment);
	Modifier& setColor(const Color& color);
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
	Color getColor() const;
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
	Color m_color = {0, 0, 0, 0}; // Transparent
	std::function<void()> m_onLClick = nullptr;
	std::function<void()> m_onRClick = nullptr;
};

}