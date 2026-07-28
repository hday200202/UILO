#include "Engine.hpp"

#include <thread>
#include <chrono>

Engine::~Engine() {
    // Scene destructor runs exit() (deletes its GameObjects), guarded so the
    // explicit exit-on-switch isn't double-run.
    for (Scene* scene : m_scenes) delete scene;
    m_scenes.clear();

    physics.shutdown();   // after scenes (they may hold bodies) and before exit
}

bool Engine::init(uint32_t width, uint32_t height, const std::string& title) {
    if (!m_renderer.init(width, height, title)) {
        utils::log::error("ENGINE", "Renderer failed to initialize");
        return false;
    }

    m_dtClock.restart();
    m_avgFpsClock.restart();
    m_frameClock.restart();
    m_shaderReloadClock.restart();

    // UILO attaches to the engine's window + bgfx context; views start at 16
    // (above POSTPROCESS=4). The engine drives its update/render/present.
    m_uiRenderer.attach(m_renderer.getWindow(), 16);
    m_ui.setRenderer(m_uiRenderer);

    physics.init();

    m_running = true;
    return m_running;
}

void Engine::update() {
    m_dt = m_dtClock.restart();

    m_fpsCount++;

    // Average FPS = frames / elapsed time over the window. (Averaging 1/dt per
    // frame instead is skewed by outliers -- a single near-zero-dt frame spikes
    // the reported FPS into the thousands even at a locked 60.)
    const float window = m_avgFpsClock.getElapsed();
    if (window >= m_avgFpsDelay) {
        m_avgFps = static_cast<float>(m_fpsCount) / window;
        m_fpsCount = 0;
        m_avgFpsClock.restart();

        utils::log::info("ENGINE", "FPS: {}", m_avgFps);
    }

    // Dev hot reload: poll shader sources a few times a second (no-op in builds
    // without the live source tree present).
    if (m_shaderReloadClock.getElapsed() >= 0.25f) {
        m_renderer.reloadShaders();
        m_shaderReloadClock.restart();
    }

    pollEvents();

    if (m_mousebinds.getRelativeMode(m_renderer.getWindow()))
        m_keybinds.update();
    m_mousebinds.update(m_renderer.getWindow());

    physics.step(m_dt);   // fixed-step before the scene reads transforms

    if (m_activeScene) {
        m_activeScene->update(m_dt);

        // Drive UILO for the active scene's cached UI page (if any).
        if (m_activeUIPage) {
            m_ui.setActivePage(m_activeUIPage);
            m_ui.update();
        }
    }
}

void Engine::render() {
    m_renderer.clear(m_clearColor);
    m_renderer.flush();

    // UILO overlay: composited over the scene. No present here -- display() does
    // the single bgfx::frame() for both the scene and the UI.
    if (m_activeUIPage) {
        m_uiRenderer.beginFrame();
        m_ui.render();
        m_uiRenderer.endFrame();
    }

    m_renderer.display();
    limitFramerate();
}


void Engine::limitFramerate() {
    if (m_minFrameTime <= 0.f) return;

    constexpr float spinMargin = 0.002f;

    float remaining = m_minFrameTime - m_frameClock.getElapsed();
    if (remaining > spinMargin) {
        std::this_thread::sleep_for(
            std::chrono::duration<float>(remaining - spinMargin)
        );
    }
    while (m_frameClock.getElapsed() < m_minFrameTime) {
        // busy-wait the remainder
    }

    m_frameClock.restart();
}


void Engine::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        m_ui.handleEvent(event); // forward to UILO (buttons / sliders / textboxes)

        if (event.type == SDL_EVENT_QUIT)
            m_running = false;

        if (event.type == SDL_EVENT_WINDOW_RESIZED)
            m_renderer.handleResize(
                event.window.data1,
                event.window.data2
            );
    }
}

GameObject* Engine::castRay(const Vec3f& origin, const Vec3f& direction) {
    return m_activeScene ? m_activeScene->castRay(origin, direction) : nullptr;
}