#include "UILO.hpp"
#include "elements/interactible/Interactible.hpp"
#include <algorithm>

namespace uilo {

UILO::UILO(sf::RenderWindow& window, Page* page) {
    m_window = &window; 
    addPage(page);
    setPage(page->m_name);
}

void UILO::addPage(Page* page) {
    page->setUILO(*this);
    m_pages[page->m_name] = std::unique_ptr<Page>(page);
}

void UILO::setPage(const std::string& pageName) {
    auto it = m_pages.find(pageName);
    if (it != m_pages.end()) {
        setCurrInteractible(nullptr);
        m_activePage = it->second.get();
    }
}

void UILO::setCurrInteractible(Interactible* i) {
    m_interactibleActivatedThisFrame = true;
    if (m_currInteractible == i) return;
    if (m_currInteractible) m_currInteractible->onDeactivate();
    m_currInteractible = i;
}

void UILO::setScale(float scale) { if (scale > 0.f) m_scale = scale; }

void UILO::update() {
    m_deltaTime = m_timer.restart();

    if (!m_activePage) return;

    sf::FloatRect logicalBounds = {
        { 0.f, 0.f },
        { static_cast<float>(m_window->getSize().x), static_cast<float>(m_window->getSize().y) }
    };

    m_activePage->update(logicalBounds, m_deltaTime);

    m_elementPool.erase(
        std::remove_if(
            m_elementPool.begin(), m_elementPool.end(),
            [&](const std::unique_ptr<Element>& e) {
                if (e->m_markedForDeletion) {
                    if (!e->m_name.empty()) m_elements.erase(e->m_name);
                    return true;
                }
                return false;
            }
        ), m_elementPool.end()
    );

    sf::Vector2i mouseRaw = sf::Mouse::getPosition(*m_window);
    sf::Vector2f mouse = { static_cast<float>(mouseRaw.x), static_cast<float>(mouseRaw.y) };

    bool leftDown  = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    auto* root = m_activePage->m_rootContainer;
    root->checkHover(mouse);

    if (leftDown && !m_prevLeftMouse) {
        m_interactibleActivatedThisFrame = false;
        root->checkLeftClick(mouse);
        if (!m_interactibleActivatedThisFrame)
            setCurrInteractible(nullptr);
    }
    if (rightDown && !m_prevRightMouse) {
        m_interactibleActivatedThisFrame = false;
        root->checkRightClick(mouse);
        if (!m_interactibleActivatedThisFrame)
            setCurrInteractible(nullptr);
    }

    m_prevLeftMouse  = leftDown;
    m_prevRightMouse = rightDown;
}

void UILO::render() { m_activePage->render(*m_window); }

void UILO::handleEvent(const sf::Event& event) {
    if (!m_activePage) return;

    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        sf::Vector2f mouse = {
            static_cast<float>(scroll->position.x),
            static_cast<float>(scroll->position.y)
        };
        m_activePage->m_rootContainer->checkScroll(mouse, scroll->delta);
    }
}

}