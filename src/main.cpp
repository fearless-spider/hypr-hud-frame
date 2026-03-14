#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

#include "HudFrameDecoration.hpp"
#include "HudFramePassElement.hpp"
#include "HudConfig.hpp"

#include <unordered_map>
#include <vector>

using namespace Hyprutils::Signal;

static HANDLE              g_pluginHandle = nullptr;
static CHyprSignalListener g_windowOpenListener;
static CHyprSignalListener g_windowDestroyListener;
static CHyprSignalListener g_windowActiveListener;
static CHyprSignalListener g_configReloadListener;

// Track decorations per window so we can manage them
static std::unordered_map<PHLWINDOW, CHudFrameDecoration*> g_decorations;
bool g_exiting = false;

static void onWindowOpen(PHLWINDOW pWindow) {
    if (!pWindow || !g_pluginHandle)
        return;

    auto cfg = CHudConfig::readConfig(g_pluginHandle);
    if (!cfg.enabled)
        return;

    if (CHudConfig::isExcluded(cfg, pWindow->m_class))
        return;

    // Already decorated?
    if (g_decorations.count(pWindow))
        return;

    auto deco = makeUnique<CHudFrameDecoration>(pWindow, g_pluginHandle);
    auto* rawPtr = deco.get();

    if (HyprlandAPI::addWindowDecoration(g_pluginHandle, pWindow, std::move(deco))) {
        g_decorations[pWindow] = rawPtr;
        rawPtr->damageEntire();
    }
}

static void onWindowDestroy(PHLWINDOW pWindow) {
    if (!pWindow)
        return;

    auto it = g_decorations.find(pWindow);
    if (it != g_decorations.end()) {
        HyprlandAPI::removeWindowDecoration(g_pluginHandle, it->second);
        g_decorations.erase(it);
    }
}

static void onWindowActive(PHLWINDOW /*pWindow*/, Desktop::eFocusReason /*reason*/) {
    // Damage all decorations so they update active/inactive state
    for (auto& [win, deco] : g_decorations) {
        if (deco)
            deco->damageEntire();
    }
}

static void onConfigReload() {
    // Config changed — update all decoration extents
    for (auto& [win, deco] : g_decorations) {
        auto window = win;
        if (window && deco)
            deco->updateWindow(window);
    }
}

static void addDecoToAllWindows() {
    for (auto& w : g_pCompositor->m_windows) {
        if (w && w->m_isMapped && !w->m_fadingOut) {
            if (g_decorations.find(w) == g_decorations.end())
                onWindowOpen(w);
        }
    }
}

static void removeDecoFromAllWindows() {
    // Copy keys to avoid modifying map during iteration
    std::vector<PHLWINDOW> windows;
    for (auto& [w, _] : g_decorations)
        windows.push_back(w);

    for (auto& w : windows)
        onWindowDestroy(w);
}

// Toggle dispatcher
static SDispatchResult toggleHudFrame(std::string /*args*/) {
    auto cfg = CHudConfig::readConfig(g_pluginHandle);
    bool wasEnabled = cfg.enabled;

    // Toggle via hyprctl
    std::string newVal = wasEnabled ? "0" : "1";
    HyprlandAPI::invokeHyprctlCommand("keyword", "plugin:hypr-hud-frame:enabled " + newVal);

    if (wasEnabled) {
        removeDecoFromAllWindows();
        HyprlandAPI::addNotification(g_pluginHandle, "[HUD Frame] Disabled", CHyprColor(1.0f, 0.5f, 0.0f, 1.0f), 3000);
    } else {
        addDecoToAllWindows();
        HyprlandAPI::addNotification(g_pluginHandle, "[HUD Frame] Enabled", CHyprColor(0.38f, 0.89f, 1.0f, 1.0f), 3000);
    }

    return {};
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    g_pluginHandle = handle;

    // Register configuration
    CHudConfig::registerConfig(handle);

    g_exiting = false;
    // NOTE: shader init is deferred to first draw() call (needs GL context)

    // Subscribe to events using the modern event bus
    g_windowOpenListener = Event::bus()->m_events.window.open.listen(
        [](PHLWINDOW w) { onWindowOpen(w); });

    g_windowDestroyListener = Event::bus()->m_events.window.destroy.listen(
        [](PHLWINDOW w) { onWindowDestroy(w); });

    g_windowActiveListener = Event::bus()->m_events.window.active.listen(
        [](PHLWINDOW w, Desktop::eFocusReason r) { onWindowActive(w, r); });

    g_configReloadListener = Event::bus()->m_events.config.reloaded.listen(
        []() { onConfigReload(); });

    // Register toggle dispatcher
    HyprlandAPI::addDispatcherV2(handle, "hypr-hud-frame:toggle", toggleHudFrame);

    // Add decorations to existing windows
    int winTotal = 0, winMapped = 0, winExcluded = 0;
    auto cfg2 = CHudConfig::readConfig(handle);
    std::string skippedClasses;
    for (auto& w : g_pCompositor->m_windows) {
        winTotal++;
        if (!w || !w->m_isMapped || w->m_fadingOut) continue;
        winMapped++;
        if (CHudConfig::isExcluded(cfg2, w->m_class)) {
            winExcluded++;
            skippedClasses += w->m_class + ",";
        }
    }
    addDecoToAllWindows();
    int decoCount = static_cast<int>(g_decorations.size());

    return {"hypr-hud-frame", "Cyberpunk HUD window decorations", "Spider's LAB", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // Stop all rendering first
    g_exiting = true;

    // Must remove decorations from Hyprland before unloading
    for (auto& [w, deco] : g_decorations) {
        if (deco)
            HyprlandAPI::removeWindowDecoration(g_pluginHandle, deco);
    }
    g_decorations.clear();

    // Listeners are automatically cleaned up when the shared pointers go out of scope
    g_windowOpenListener.reset();
    g_windowDestroyListener.reset();
    g_windowActiveListener.reset();
    g_configReloadListener.reset();

    g_pluginHandle = nullptr;
}
