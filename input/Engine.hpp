#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <type_traits>

#include "../renderer/Renderer.hpp"
#include "../utils/Timer.hpp"

#include "EngineContext.hpp"
#include "scene/Scene.hpp"
#include "input/Keybinds.hpp"
#include "input/Mousebinds.hpp"
#include "physics/PhysicsWorld.hpp"

#include "UILO.hpp"

class Engine {
public:
    Engine() = default;
    ~Engine();

    bool init(uint32_t width, uint32_t height, const std::string& title);

    void update();
    void render();

    // Physics simulation, reached as ctx.engine.physics (Jolt-free interface).
    PhysicsWorld physics;

    template <typename T>
    T* addScene(const std::string& name) {
        static_assert(std::is_base_of_v<Scene, T>, "T must derive from Scene");

        EngineContext ctx{
            *this,
            m_renderer,
            *m_renderer.getShaderManager(),
            *m_renderer.getTextureManager(),
            *m_renderer.getMaterialManager(),
            m_keybinds,
            m_mousebinds,
            physics
        };

        T* scene = new T(ctx);
        scene->setName(name);
        m_scenes.push_back(scene);

        if (m_activeScene == nullptr) gotoScene(scene);
        return scene;
    }

    void loadScene(const std::string& name) {
        for (Scene* scene : m_scenes)
            if (scene->getName() == name) { gotoScene(scene); return; }
        utils::log::warn("ENGINE", "loadScene: no scene named '{}'", name);
    }

    Scene* getActiveScene() const { return m_activeScene; }

    bool isRunning() const { return m_running; }
    float getDeltaTime() const { return m_dt; }
    float getFrameRate() const { return 1.f / m_dt; }
    float getAvgFrameRate() const { return m_avgFps; }

    Renderer* getRenderer() { return &m_renderer; }

    Keybinds* getKeybinds() { return &m_keybinds; }
    Mousebinds* getMousebinds() { return &m_mousebinds; }

    GameObject* castRay(const Vec3f& origin, const Vec3f& direction);

    void setMaxFramerate(uint32_t fps) {
        m_minFrameTime = (fps > 0) ? 1.f / static_cast<float>(fps) : 0.f;
    }

    void setClearColor(uint32_t clearColor) { m_clearColor = clearColor; }
    uint32_t getClearColor() const { return m_clearColor; }

private:
    Renderer m_renderer;
    uilo::Renderer m_uiRenderer; // UILO's renderer, attached to the engine's bgfx context
    uilo::UILO     m_ui;
    bool m_running = false;

    uint32_t m_clearColor = 0x000000FF;

    utils::Timer m_dtClock;
    float m_dt = 0.1f;

    utils::Timer m_avgFpsClock;
    int m_fpsCount = 0;
    float m_avgFpsDelay = 3.f;
    float m_avgFps = 0.f;

    utils::Timer m_frameClock;
    float m_minFrameTime = 0.f;

    utils::Timer m_shaderReloadClock; // throttles dev shader hot reload

    Keybinds m_keybinds;
    Mousebinds m_mousebinds;

    std::vector<Scene*> m_scenes;
    Scene* m_activeScene = nullptr;
    uilo::Page* m_activeUIPage = nullptr; // cached from scene->getUI() on activation

    void gotoScene(Scene* scene) {
        if (m_activeScene == scene) return;
        if (m_activeScene) m_activeScene->exit();
        m_activeScene = scene;
        if (m_activeScene) m_activeScene->start();
        // Cache the scene's UI page once (getUI() may allocate; don't call it
        // every frame). Null if the scene doesn't override getUI().
        m_activeUIPage = m_activeScene ? m_activeScene->getUI() : nullptr;
    }

    void pollEvents();
    void limitFramerate();
};