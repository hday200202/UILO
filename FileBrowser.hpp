#pragma once

#include "UILO.hpp"
#include "FileTree.hpp"
#include "assets/EmbeddedAssets.hpp"
#include <algorithm>
#include <future>
#include <atomic>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <regex>
#include <fstream>

using namespace uilo;

enum class BrowserMode {
    BROWSE,
    SELECT_FILE,
    SELECT_DIRECTORY
};

struct BrowserTheme {
    sf::Color backgroundColor = sf::Color(40, 40, 40);
    sf::Color controlsBarColor = sf::Color(60, 60, 60);
    sf::Color gridBackgroundColor = sf::Color(50, 50, 50);
    sf::Color buttonColor = sf::Color(77, 105, 153);
    sf::Color searchBarColor = sf::Color(50, 50, 50);
    sf::Color searchBarTextColor = sf::Color::White;
    sf::Color searchBarOutlineColor = sf::Color(77, 105, 153);
    sf::Color entryBackgroundColor = sf::Color::Transparent;
    sf::Color entrySelectedColor = sf::Color(128, 128, 128, 100);
    sf::Color folderIconColor = sf::Color(77, 105, 153);
    sf::Color fileIconColor = sf::Color(77, 105, 153);
    sf::Color textColor = sf::Color::White;
};

class FileBrowser {
public:
    FileBrowser(const std::filesystem::path& path, BrowserMode mode = BrowserMode::BROWSE, 
                const std::vector<std::string>& filters = {});

    void update();
    bool isOpen() const { return m_window.isOpen(); }
    std::filesystem::path getSelectedPath() const { return m_selectedPath; }
    bool hasSelection() const { return !m_selectedPath.empty(); }
    
    void setTheme(const BrowserTheme& theme) { m_theme = theme; }
    BrowserTheme& getTheme() { return m_theme; }
    
    // Icon customization (optional - embedded icons used by default)
    void setFolderIcon(const sf::Image& icon) { m_folderIcon = icon; }
    void setFileIcon(const sf::Image& icon) { m_fileIcon = icon; }
    void setFolderIconPath(const std::string& path) { [[maybe_unused]] bool loaded = m_folderIcon.loadFromFile(path); }
    void setFileIconPath(const std::string& path) { [[maybe_unused]] bool loaded = m_fileIcon.loadFromFile(path); }

private:
    sf::RenderWindow m_window;
    sf::VideoMode m_screenRes;
    sf::View m_windowView;

    std::unique_ptr<UILO> m_ui;
    Row* m_controlsBar;
    Button* m_backButton;
    Button* m_selectButton;
    TextBox* m_searchBox;
    Grid* m_fileGrid;

    FileTree fileTree;
    std::vector<Entry*> m_searchResults;
    std::string m_lastSearchText;
    std::filesystem::path m_currentDirectory;
    
    sf::Image m_folderIcon;
    sf::Image m_fileIcon;
    BrowserTheme m_theme;
    
    BrowserMode m_mode;
    std::vector<std::string> m_fileFilters;
    std::vector<std::regex> m_filterRegexes;
    
    float m_minCellWidth = 256.f;
    float m_maxCellWidth = 256.f;
    float m_cellHeight = 256.f;
    sf::Vector2u m_lastWindowSize;
    bool m_isNavigating = false;
    
    std::future<void> m_loadingFuture;
    std::atomic<bool> m_isLoading{false};
    
    std::vector<std::filesystem::path> m_allEntryPaths;
    std::mutex m_pathsMutex;
    size_t m_lastRenderedStart = 0;
    size_t m_lastRenderedEnd = 0;
    float m_lastScrollOffset = 0.f;
    
    std::filesystem::path m_selectedPath;
    std::filesystem::path m_previousSelectedPath;
    std::chrono::steady_clock::time_point m_lastClickTime;
    static constexpr int DOUBLE_CLICK_MS = 250;
    
    std::unordered_map<std::string, Container*> m_entryContainers;

    void initWindow();
    void buildUI();
    void loadIcons();
    void compileFilters();
    bool matchesFilter(const std::filesystem::path& path) const;
    
    // Helper functions for creating UI elements with default font
    Button* createButton(Modifier modifier, ButtonStyle style, const std::string& text, 
                         sf::Color textColor, const std::string& name) {
        return new Button(modifier, style, text, "", textColor, name);
    }
    
    Text* createText(Modifier modifier, const std::string& str, const std::string& name) {
        return new Text(modifier, str, "", name);
    }
    
    void updateGridLayout();
    void updateCurrentDirectoryDisplay();
    void navigateToDirectory(const std::filesystem::path& path);
    void navigateToParent();
    void performSearch(const std::string& searchText);
    void updateVisibleEntries(bool forceUpdate = false);
    Container* buildEntryUI(const std::filesystem::path& path);
    void handleEntryAction(const std::filesystem::path& path, bool isDirectory);
    void handleSelectButton();
};

inline FileBrowser::FileBrowser(const std::filesystem::path& path, BrowserMode mode,
                                 const std::vector<std::string>& filters) 
    : m_mode(mode), m_currentDirectory(path), m_fileFilters(filters) {
    compileFilters();
    loadIcons();
    initWindow();
    buildUI();
    navigateToDirectory(path);
}

inline void FileBrowser::update() {
    m_isNavigating = false;
    
    if (m_selectedPath != m_previousSelectedPath) {
        if (!m_previousSelectedPath.empty()) {
            auto it = m_entryContainers.find(m_previousSelectedPath.string());
            if (it != m_entryContainers.end() && it->second) {
                it->second->m_modifier.setColor(m_theme.entryBackgroundColor);
                it->second->m_isDirty = true;
            }
        }
        
        if (!m_selectedPath.empty()) {
            auto it = m_entryContainers.find(m_selectedPath.string());
            if (it != m_entryContainers.end() && it->second) {
                it->second->m_modifier.setColor(m_theme.entrySelectedColor);
                it->second->m_isDirty = true;
            }
        }
        
        // Update select button color based on valid selection
        if (m_selectButton && m_mode != BrowserMode::BROWSE) {
            bool hasValidSelection = false;
            
            if (!m_selectedPath.empty()) {
                if (m_mode == BrowserMode::SELECT_FILE && std::filesystem::is_regular_file(m_selectedPath)) {
                    hasValidSelection = true;
                } else if (m_mode == BrowserMode::SELECT_DIRECTORY && std::filesystem::is_directory(m_selectedPath)) {
                    hasValidSelection = true;
                }
            }
            
            m_selectButton->m_modifier.setColor(hasValidSelection ? m_theme.buttonColor : m_theme.backgroundColor);
            m_selectButton->m_isDirty = true;
        }
        
        m_previousSelectedPath = m_selectedPath;
    }
    
    // Update visible entries based on scroll position
    if (m_fileGrid) {
        float currentScrollOffset = m_fileGrid->getVerticalOffset();
        if (std::abs(currentScrollOffset - m_lastScrollOffset) > m_cellHeight * 2.0f) {
            m_lastScrollOffset = currentScrollOffset;
            updateVisibleEntries();
        }
    }
    
    // Check async loading completion
    if (m_isLoading && m_loadingFuture.valid()) {
        if (m_loadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            m_loadingFuture.get();
            m_isLoading = false;
            updateVisibleEntries();
        }
    }
    
    // Handle search text changes
    if (m_searchBox && !m_isLoading) {
        std::string currentSearchText = m_searchBox->getText();
        if (currentSearchText != m_lastSearchText) {
            m_lastSearchText = currentSearchText;
            performSearch(currentSearchText);
        }
    }
    
    // Handle window resize
    sf::Vector2u currentSize = m_window.getSize();
    if (currentSize != m_lastWindowSize) {
        m_lastWindowSize = currentSize;
        updateGridLayout();
    }
    
    m_ui->forceUpdate();

    if (m_ui->windowShouldUpdate()) {
        m_window.clear();
        m_ui->render();
        m_window.display();
    }

    m_ui->resetScrollDeltas();
}

void FileBrowser::loadIcons() {
    // Load icons from files or use embedded versions
    if (!m_folderIcon.loadFromFile("assets/icons/folder.png")) {
        [[maybe_unused]] bool loaded = m_folderIcon.loadFromMemory(EMBEDDED_FOLDER_ICON.data(), EMBEDDED_FOLDER_ICON.size());
    }
    
    if (!m_fileIcon.loadFromFile("assets/icons/file.png")) {
        [[maybe_unused]] bool loaded = m_fileIcon.loadFromMemory(EMBEDDED_FILE_ICON.data(), EMBEDDED_FILE_ICON.size());
    }
}

void FileBrowser::compileFilters() {
    m_filterRegexes.clear();
    for (const auto& filter : m_fileFilters) {
        std::string regexPattern;
        
        for (size_t i = 0; i < filter.length(); ++i) {
            char c = filter[i];
            if (c == '*') {
                regexPattern += ".*";
            } else if (c == '?') {
                regexPattern += ".";
            } else if (c == '.' || c == '^' || c == '$' || c == '|' || c == '(' || c == ')' || 
                       c == '[' || c == ']' || c == '{' || c == '}' || c == '+' || c == '\\') {
                regexPattern += '\\';
                regexPattern += c;
            } else {
                regexPattern += c;
            }
        }
        
        regexPattern = "^" + regexPattern + "$";
        m_filterRegexes.emplace_back(regexPattern, std::regex::icase);
    }
}

bool FileBrowser::matchesFilter(const std::filesystem::path& path) const {
    if (std::filesystem::is_directory(path)) {
        return true;
    }
    
    if (m_filterRegexes.empty()) {
        return true;
    }
    
    std::string filename = path.filename().string();
    for (const auto& regex : m_filterRegexes) {
        if (std::regex_match(filename, regex)) {
            return true;
        }
    }
    
    return false;
}

void FileBrowser::handleSelectButton() {
    if (m_selectedPath.empty()) return;
    
    if (m_mode == BrowserMode::SELECT_FILE && std::filesystem::is_regular_file(m_selectedPath)) {
        m_window.close();
    } else if (m_mode == BrowserMode::SELECT_DIRECTORY && std::filesystem::is_directory(m_selectedPath)) {
        m_window.close();
    }
}

void FileBrowser::initWindow() {
    m_screenRes = sf::VideoMode::getDesktopMode();
    m_screenRes.size.x /= 2.f;
    m_screenRes.size.y /= 2.f;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;
    m_window.create(m_screenRes, "File Browser", sf::Style::Titlebar | sf::Style::Default, sf::State::Windowed, settings);
    m_window.requestFocus();
}

void FileBrowser::buildUI() {
    m_lastWindowSize = m_window.getSize();
    
    float windowWidth = static_cast<float>(m_lastWindowSize.x);
    float controlsHeight = 64.f;
    
    Column* baseColumn = column(
        Modifier()
            .setColor(m_theme.backgroundColor),
        {},
        "base_column"
    );
    
    m_controlsBar = row(
        Modifier()
            .setfixedHeight(controlsHeight)
            .setColor(m_theme.controlsBarColor),
        {},
        "controls_bar"
    );
    
    m_backButton = button(
        Modifier()
            .setfixedWidth(96)
            .setfixedHeight(48)
            .setColor(m_theme.buttonColor)
            .align(Align::LEFT | Align::CENTER_Y)
            .onLClick([this]() {
                navigateToParent();
            }),
        ButtonStyle::Pill,
        "Back",
        "",
        m_theme.textColor,
        "back_button"
    );
    
    m_searchBox = textBox(
        Modifier()
            .setWidth(1.f)
            .setfixedHeight(48)
            .setColor(m_theme.searchBarColor)
            .align(Align::CENTER_Y | Align::LEFT),
        TBStyle::Pill,
        "",
        m_currentDirectory.string(),
        m_theme.searchBarTextColor,
        m_theme.searchBarOutlineColor,
        "search_box"
    );
    
    m_controlsBar->addElement(spacer(Modifier().setfixedWidth(32)));
    m_controlsBar->addElement(m_backButton);
    m_controlsBar->addElement(spacer(Modifier().setfixedWidth(32)));
    m_controlsBar->addElement(m_searchBox);
    m_controlsBar->addElement(spacer(Modifier().setfixedWidth(32)));
    
    if (m_mode != BrowserMode::BROWSE) {
        m_selectButton = button(
            Modifier()
                .setfixedWidth(96)
                .setfixedHeight(48)
                .setColor(m_theme.backgroundColor)
                .align(Align::RIGHT | Align::CENTER_Y)
                .onLClick([this]() {
                    handleSelectButton();
                }),
            ButtonStyle::Pill,
            "Select",
            "",
            m_theme.textColor,
            "select_button"
        );
        m_controlsBar->addElement(m_selectButton);
        m_controlsBar->addElement(spacer(Modifier().setfixedWidth(32).align(Align::RIGHT)));
    }
    
    baseColumn->addElement(m_controlsBar);
    
    int columns = std::max(1, static_cast<int>(windowWidth / m_minCellWidth));
    float cellWidth = windowWidth / columns;
    
    if (cellWidth > m_maxCellWidth) {
        columns = std::max(1, static_cast<int>(windowWidth / m_maxCellWidth));
        cellWidth = windowWidth / columns;
    }
    
    m_fileGrid = grid(
        Modifier()
            .setColor(m_theme.gridBackgroundColor)
            .align(Align::CENTER_X | Align::BOTTOM)
            .onLClick([this]() {
                m_selectedPath.clear();
            }),
        cellWidth,
        m_cellHeight,
        columns,
        0,
        {},
        "file_grid"
    );
    m_fileGrid->setScrollSpeed(50.f);
    
    baseColumn->addElement(m_fileGrid);

    m_ui = std::make_unique<UILO>(m_window, m_windowView);
    m_ui->addPage(page({baseColumn}), "main");
}

void FileBrowser::updateGridLayout() {
    if (!m_fileGrid) return;
    
    float windowWidth = static_cast<float>(m_lastWindowSize.x);
    int columns = std::max(1, static_cast<int>(windowWidth / m_minCellWidth));
    float cellWidth = windowWidth / columns;
    
    if (cellWidth > m_maxCellWidth) {
        columns = std::max(1, static_cast<int>(windowWidth / m_maxCellWidth));
        cellWidth = windowWidth / columns;
    }
    
    m_fileGrid->setCellSize(cellWidth, m_cellHeight);
    m_fileGrid->setGridDimensions(columns, 0);
}

void FileBrowser::updateCurrentDirectoryDisplay() {
    if (m_searchBox && fileTree.getRootDir()) {
        m_currentDirectory = fileTree.getRootDir()->getPath();
        m_searchBox->setPlaceholder(m_currentDirectory.string());
    }
}

void FileBrowser::navigateToDirectory(const std::filesystem::path& path) {
    if (m_isLoading) return;
    
    std::filesystem::path resolvedPath = path;
    try {
        if (std::filesystem::exists(path)) {
            resolvedPath = std::filesystem::canonical(path);
        }
    } catch (const std::filesystem::filesystem_error&) {
        return;
    }
    
    if (m_fileGrid) {
        m_fileGrid->clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(m_pathsMutex);
        m_allEntryPaths.clear();
    }
    
    m_selectedPath.clear();
    m_searchResults.clear();
    m_lastSearchText = "";
    m_currentDirectory = resolvedPath;
    
    if (m_searchBox) {
        m_searchBox->setPlaceholder(m_currentDirectory.string());
        m_searchBox->clearText();
        m_searchBox->setActive(false);
    }
    
    m_isLoading = true;
    
    m_loadingFuture = std::async(std::launch::async, [this, resolvedPath]() {
        fileTree.setRootDir(resolvedPath);
        
        if (fileTree.getRootDir()) {
            std::vector<Entry*> entries;
            for (const auto& entry : fileTree.getRootDir()->getEntries()) {
                entries.push_back(entry.get());
            }
            
            std::sort(entries.begin(), entries.end(), [](Entry* a, Entry* b) {
                bool aIsDir = a->isDirectory();
                bool bIsDir = b->isDirectory();
                if (aIsDir != bIsDir) return aIsDir;
                return a->getName() < b->getName();
            });
            
            // Store all paths
            {
                std::lock_guard<std::mutex> lock(m_pathsMutex);
                for (Entry* entry : entries) {
                    if (matchesFilter(entry->getPath())) {
                        m_allEntryPaths.push_back(entry->getPath());
                    }
                }
            }
        }
    });
}

void FileBrowser::navigateToParent() {
    if (m_isNavigating) return;
    
    if (fileTree.getRootDir()) {
        std::filesystem::path currentPath = fileTree.getRootDir()->getPath();
        std::filesystem::path parentPath = currentPath.parent_path();
        
        if (parentPath != currentPath) {
            m_isNavigating = true;
            navigateToDirectory(parentPath);
        }
    }
}

void FileBrowser::performSearch(const std::string& searchText) {
    if (m_isLoading) return;
    
    m_searchResults.clear();
    
    if (!searchText.empty() && fileTree.getRootDir()) {
        if (m_fileGrid) {
            m_fileGrid->clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(m_pathsMutex);
            m_allEntryPaths.clear();
        }
        
        m_isLoading = true;
        
        m_loadingFuture = std::async(std::launch::async, [this, searchText]() {
            std::vector<Entry*> results;
            fileTree.find(searchText, results);
            
            std::sort(results.begin(), results.end(), [](Entry* a, Entry* b) {
                bool aIsDir = a->isDirectory();
                bool bIsDir = b->isDirectory();
                if (aIsDir != bIsDir) return aIsDir;
                return a->getName() < b->getName();
            });
            
            {
                std::lock_guard<std::mutex> lock(m_pathsMutex);
                for (Entry* entry : results) {
                    if (matchesFilter(entry->getPath())) {
                        m_allEntryPaths.push_back(entry->getPath());
                    }
                }
            }
            
            m_searchResults = std::move(results);
        });
    } else {
        if (fileTree.getRootDir()) {
            navigateToDirectory(fileTree.getRootDir()->getPath());
        }
    }
}

void FileBrowser::updateVisibleEntries(bool forceUpdate) {
    if (!m_fileGrid) return;
    
    std::lock_guard<std::mutex> lock(m_pathsMutex);
    if (m_allEntryPaths.empty()) return;
    
    float viewportHeight = m_window.getSize().y - 60.f;
    float scrollOffset = std::abs(m_fileGrid->getVerticalOffset());
    
    int columns = static_cast<int>(m_window.getSize().x / m_minCellWidth);
    if (columns == 0) columns = 1;
    
    int firstVisibleRow = static_cast<int>(scrollOffset / m_cellHeight) - 25;
    int lastVisibleRow = static_cast<int>((scrollOffset + viewportHeight) / m_cellHeight) + 25;
    
    if (firstVisibleRow < 0) firstVisibleRow = 0;
    
    size_t firstVisibleIndex = firstVisibleRow * columns;
    size_t lastVisibleIndex = (lastVisibleRow + 1) * columns;
    
    if (lastVisibleIndex > m_allEntryPaths.size()) {
        lastVisibleIndex = m_allEntryPaths.size();
    }
    
    if (!forceUpdate && firstVisibleIndex == m_lastRenderedStart && lastVisibleIndex == m_lastRenderedEnd) {
        return;
    }
    
    m_lastRenderedStart = firstVisibleIndex;
    m_lastRenderedEnd = lastVisibleIndex;
    
    m_entryContainers.clear();
    
    // Clear and repopulate grid with visible entries
    m_fileGrid->clear();
    
    for (size_t i = firstVisibleIndex; i < lastVisibleIndex && i < m_allEntryPaths.size(); ++i) {
        if (std::filesystem::exists(m_allEntryPaths[i])) {
            auto entryUI = buildEntryUI(m_allEntryPaths[i]);
            if (entryUI) {
                m_fileGrid->addElement(entryUI);
            }
        }
    }
}

Container* FileBrowser::buildEntryUI(const std::filesystem::path& path) {
    bool isDirectory = std::filesystem::is_directory(path);
    std::string fileName = path.filename().string();
    
    float boxWidth = m_minCellWidth * 0.85f;
    float boxHeight = m_cellHeight * 0.85f;
    
    sf::Image& iconImage = isDirectory ? m_folderIcon : m_fileIcon;
    sf::Color iconColor = isDirectory ? m_theme.folderIconColor : m_theme.fileIconColor;
    bool isSelected = (path == m_selectedPath);
    sf::Color bgColor = isSelected ? m_theme.entrySelectedColor : m_theme.entryBackgroundColor;
    
    auto clickHandler = [this, path, isDirectory]() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastClickTime).count();
        
        if (path == m_selectedPath && elapsed < DOUBLE_CLICK_MS) {
            handleEntryAction(path, isDirectory);
        } else {
            m_selectedPath = path;
            m_lastClickTime = now;
        }
    };
    
    Container* entryColumn = column(
        Modifier()
            .setfixedWidth(boxWidth)
            .setfixedHeight(boxHeight)
            .setColor(bgColor)
            .align(Align::CENTER_X | Align::CENTER_Y)
            .onLClick(clickHandler),
        contains{
            image(
                Modifier()
                    .setfixedWidth(180.f)
                    .setfixedHeight(180.f)
                    .setColor(iconColor)
                    .align(Align::CENTER_X | Align::TOP)
                    .onLClick(clickHandler),
                iconImage,
                true
            ),
            spacer(
                Modifier()
                    .setfixedHeight(10.f)
            ),
            text(
                Modifier()
                    .setColor(m_theme.textColor)
                    .setfixedHeight(20.f)
                    .align(Align::CENTER_X)
                    .onLClick(clickHandler),
                fileName.length() > 15 ? fileName.substr(0, 12) + "..." : fileName,
                ""
            )
        },
        fileName + "_entry"
    );
    
    m_entryContainers[path.string()] = entryColumn;
    
    return entryColumn;
}

void FileBrowser::handleEntryAction(const std::filesystem::path& path, bool isDirectory) {
    if (m_isNavigating) return;
    
    switch (m_mode) {
        case BrowserMode::BROWSE:
            if (isDirectory) {
                m_isNavigating = true;
                m_selectedPath.clear();
                navigateToDirectory(path);
            }
            break;
            
        case BrowserMode::SELECT_FILE:
            if (isDirectory) {
                m_isNavigating = true;
                m_selectedPath.clear();
                navigateToDirectory(path);
            } else {
                m_window.close();
            }
            break;
            
        case BrowserMode::SELECT_DIRECTORY:
            if (isDirectory) {
                m_window.close();
            }
            break;
    }
}