#include "HudFrameDecoration.hpp"
#include "HudFramePassElement.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/pass/RectPassElement.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

using namespace Hyprutils::Signal;

static std::chrono::steady_clock::time_point s_lastFrameTime = std::chrono::steady_clock::now();

CHudFrameDecoration::CHudFrameDecoration(PHLWINDOW pWindow, HANDLE pluginHandle)
    : IHyprWindowDecoration(pWindow), m_pWindow(pWindow), m_pluginHandle(pluginHandle) {

    auto cfg = CHudConfig::readConfig(m_pluginHandle);
    float totalExtent = static_cast<float>(cfg.glowRadius + cfg.borderWidth + cfg.padding + 2);
    m_extents = {{totalExtent, totalExtent}, {totalExtent, totalExtent}};
}

CHudFrameDecoration::~CHudFrameDecoration() {}

SDecorationPositioningInfo CHudFrameDecoration::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy         = DECORATION_POSITION_ABSOLUTE;
    info.desiredExtents = m_extents;
    info.priority       = 2000;
    info.edges          = DECORATION_EDGE_TOP | DECORATION_EDGE_BOTTOM |
                          DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT;
    return info;
}

void CHudFrameDecoration::onPositioningReply(const SDecorationPositioningReply& /*reply*/) {
}

bool CHudFrameDecoration::isWindowActive() const {
    auto window = m_pWindow.lock();
    if (!window)
        return false;

    auto workspace = window->m_workspace;
    if (!workspace)
        return false;

    auto lastFocused = workspace->m_lastFocusedWindow.lock();
    return lastFocused == window;
}

extern bool g_exiting;

void CHudFrameDecoration::draw(PHLMONITOR pMonitor, float const& a) {
    if (g_exiting)
        return;

    auto window = m_pWindow.lock();
    if (!window || !window->m_isMapped)
        return;

    auto cfg = CHudConfig::readConfig(m_pluginHandle);

    if (!cfg.enabled)
        return;

    if (CHudConfig::isExcluded(cfg, window->m_class))
        return;

    if (window->m_fullscreenState.internal != FSMODE_NONE)
        return;

    // Window geometry in monitor-local coords
    Vector2D windowPos  = window->m_realPosition->value();
    Vector2D windowSize = window->m_realSize->value();
    Vector2D monPos     = pMonitor->m_position;

    float bw = static_cast<float>(cfg.borderWidth);

    // Active/inactive color switch
    bool active = g_pCompositor->isWindowActive(window);
    float cr = active ? cfg.activeBorderR : cfg.inactiveBorderR;
    float cg = active ? cfg.activeBorderG : cfg.inactiveBorderG;
    float cb = active ? cfg.activeBorderB : cfg.inactiveBorderB;
    float ca = (active ? cfg.activeBorderA : cfg.inactiveBorderA) * a;

    // Pulse animation on active window
    float pulse = 1.0f;
    if (active && cfg.pulseIntensity > 0.001f) {
        float time = std::chrono::duration<float>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        pulse = 1.0f + std::sin(time * cfg.pulseSpeed) * cfg.pulseIntensity;
    }

    CHyprColor borderCol(cr, cg, cb, std::min(ca * pulse, 1.0f));

    auto addRect = [&](CBox box, const CHyprColor& col) {
        box.scale(pMonitor->m_scale);
        CRectPassElement::SRectData data;
        data.box   = box;
        data.color = col;
        data.round = 0;
        g_pHyprRenderer->m_renderPass.add(makeUnique<CRectPassElement>(data));
    };

    float pad = static_cast<float>(cfg.padding);
    double lx = windowPos.x - monPos.x - pad;
    double ly = windowPos.y - monPos.y - pad;
    double w  = windowSize.x + pad * 2.0;
    double h  = windowSize.y + pad * 2.0;

    float notch = static_cast<float>(cfg.notchSize);
    float glowR = static_cast<float>(cfg.glowRadius);
    float halfBw = bw * 0.5f;

    // ── GLOW ──
    if (glowR > 0) {
        float gr = active ? cfg.glowR : cfg.glowInactiveR;
        float gg = active ? cfg.glowG : cfg.glowInactiveG;
        float gb = active ? cfg.glowB : cfg.glowInactiveB;
        float ga = (active ? cfg.glowA : cfg.glowInactiveA) * a * pulse;

        int layers = std::max(1, static_cast<int>(glowR / 2.0f));
        for (int i = 1; i <= layers; i++) {
            float t = static_cast<float>(i) / static_cast<float>(layers);
            float offset = bw + t * glowR;
            float layerAlpha = ga * (1.0f - t) * (1.0f - t);
            CHyprColor glowCol(gr, gg, gb, layerAlpha);
            float lw = 2.0f;
            addRect({lx - offset + notch, ly - offset, w + (offset - notch) * 2.0, lw}, glowCol);
            addRect({lx - offset + notch, ly + h + offset - lw, w + (offset - notch) * 2.0, lw}, glowCol);
            addRect({lx - offset, ly - offset + notch, lw, h + (offset - notch) * 2.0}, glowCol);
            addRect({lx + w + offset - lw, ly - offset + notch, lw, h + (offset - notch) * 2.0}, glowCol);
        }
    }

    // ── CORNER FILLS (dark background behind cut corners) ──
    {
        CHyprColor fillCol(0.08f, 0.06f, 0.12f, 0.9f * a);
        int steps = std::max(8, static_cast<int>(notch));
        float stepX = notch / static_cast<float>(steps);
        float stepY = notch / static_cast<float>(steps);
        // Top-left fill
        for (int i = 0; i < steps; i++) {
            float fi = static_cast<float>(i);
            float rowX = lx - halfBw;
            float rowY = ly - halfBw + stepY * fi;
            float rowW = notch - stepX * fi;
            addRect({rowX, rowY, rowW, stepY + 0.5}, fillCol);
        }
        // Bottom-right fill
        for (int i = 0; i < steps; i++) {
            float fi = static_cast<float>(i);
            float rowX = lx + w + halfBw - notch + stepX * fi;
            float rowY = ly + h + halfBw - stepY * (i + 1);
            float rowW = notch - stepX * fi;
            addRect({rowX, rowY, rowW, stepY + 0.5}, fillCol);
        }
    }

    // ── BORDER with angled corners ──
    addRect({lx + notch, ly - halfBw, w - notch, bw}, borderCol);
    addRect({lx - halfBw, ly + h - halfBw, w - notch + halfBw, bw}, borderCol);
    addRect({lx - halfBw, ly + notch, bw, h - notch + halfBw}, borderCol);
    addRect({lx + w - halfBw, ly - halfBw, bw, h - notch + halfBw}, borderCol);

    // Top-left diagonal
    {
        int steps = std::max(8, static_cast<int>(notch));
        float stepX = notch / static_cast<float>(steps);
        float stepY = notch / static_cast<float>(steps);
        for (int i = 0; i < steps; i++) {
            float fi = static_cast<float>(i);
            addRect({lx - halfBw + stepX * fi, ly + notch - stepY * fi - stepY - halfBw, stepX + 0.5, bw}, borderCol);
        }
    }
    // Bottom-right diagonal
    {
        int steps = std::max(8, static_cast<int>(notch));
        float stepX = notch / static_cast<float>(steps);
        float stepY = notch / static_cast<float>(steps);
        for (int i = 0; i < steps; i++) {
            float fi = static_cast<float>(i);
            addRect({lx + w + halfBw - stepX * fi - stepX, ly + h - notch + stepY * fi - halfBw, stepX + 0.5, bw}, borderCol);
        }
    }

    // ── HASH MARKS ──
    if (cfg.showHashMarks) {
        CHyprColor hashCol(cr, cg, cb, ca * (active ? 0.7f : 0.3f));
        float markLen = 10.0f, markThick = bw * 0.7f, markGap = 4.0f, markOffset = 3.0f;
        for (int i = 0; i < 4; i++) {
            float fi = static_cast<float>(i);
            addRect({lx + w + bw + 3.0 + fi * markGap, ly - bw + 2.0 + fi * markOffset, markLen, markThick}, hashCol);
            addRect({lx - bw - 3.0 - markLen - fi * markGap, ly + h + bw - 2.0 - markThick - fi * markOffset, markLen, markThick}, hashCol);
        }
        if (active) {
            for (int i = 0; i < 3; i++) {
                float fi = static_cast<float>(i);
                addRect({lx - bw - 3.0 - 8.0 - fi * markGap, ly - bw + 2.0 + fi * markOffset, 8.0, markThick}, hashCol);
                addRect({lx + w + bw + 3.0 + fi * markGap, ly + h + bw - 2.0 - markThick - fi * markOffset, 8.0, markThick}, hashCol);
            }
        }
    }

    // ── ACCENT BARS & DOTS ──
    if (active && cfg.showEdgeDecor) {
        CHyprColor barCol(cr, cg, cb, ca * 0.5f);
        CHyprColor dotCol(cr, cg, cb, ca * 0.6f);
        float barW = w * 0.15f, barH = bw * 1.5f;
        addRect({lx + w * 0.5 - barW * 0.5, ly - bw - barH - 1.0, barW, barH}, barCol);
        double dotsStart = lx + w * 0.5 - 12.5;
        for (int i = 0; i < 5; i++)
            addRect({dotsStart + static_cast<float>(i) * 5.0, ly - bw - 1.0 - 2.5, 2.5, 2.5}, dotCol);
        double centerX = lx + w * 0.5;
        for (int i = -1; i <= 1; i++)
            addRect({centerX + static_cast<float>(i) * 8.0 - 1.5, ly + h + bw + 2.0, 3.0, 3.0}, dotCol);
        float sideBarH = h * 0.1f;
        addRect({lx + w + bw + 1.0, ly + 10.0, bw, sideBarH}, barCol);
        addRect({lx - bw - bw - 1.0, ly + h - 10.0 - sideBarH, bw, sideBarH}, barCol);
    }

    // ── INNER ACCENT LINES ──
    if (active) {
        float inset = bw + 4.0f, thin = 1.0f;
        CHyprColor accentCol(cr, cg, cb, ca * 0.25f);
        addRect({lx + notch + 6.0, ly + inset, w * 0.6, thin}, accentCol);
        addRect({lx + inset, ly + notch + 6.0, thin, h * 0.6}, accentCol);
        addRect({lx + 6.0, ly + h - inset, w * 0.5, thin}, accentCol);
        addRect({lx + w - inset, ly + 6.0, thin, h * 0.5}, accentCol);
    }

    // ── PULSE REDRAW ──
    if (active && cfg.pulseIntensity > 0.001f) {
        float totalR = glowR + bw + pad;
        CBox dmg = {windowPos.x - totalR, windowPos.y - totalR,
                    windowSize.x + totalR * 2.0, windowSize.y + totalR * 2.0};
        g_pHyprRenderer->damageBox(dmg);
        g_pCompositor->scheduleFrameForMonitor(pMonitor);
    }
}

eDecorationType CHudFrameDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHudFrameDecoration::updateWindow(PHLWINDOW /*pWindow*/) {
    auto window = m_pWindow.lock();
    if (!window)
        return;

    auto cfg = CHudConfig::readConfig(m_pluginHandle);
    float totalExtent = static_cast<float>(cfg.glowRadius + cfg.borderWidth + cfg.padding + 2);
    m_extents = {{totalExtent, totalExtent}, {totalExtent, totalExtent}};

    bool active = isWindowActive();
    if (active != m_lastFocusState) {
        m_lastFocusState = active;
        damageEntire();
    }
}

void CHudFrameDecoration::damageEntire() {
    auto window = m_pWindow.lock();
    if (!window)
        return;

    auto cfg = CHudConfig::readConfig(m_pluginHandle);
    float totalExtent = static_cast<float>(cfg.glowRadius + cfg.borderWidth + cfg.padding + 2);

    Vector2D windowPos  = window->m_realPosition->value();
    Vector2D windowSize = window->m_realSize->value();

    CBox dmgBox = {
        windowPos.x - totalExtent,
        windowPos.y - totalExtent,
        windowSize.x + totalExtent * 2.0,
        windowSize.y + totalExtent * 2.0
    };

    g_pHyprRenderer->damageBox(dmgBox);
}

eDecorationLayer CHudFrameDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

uint64_t CHudFrameDecoration::getDecorationFlags() {
    return DECORATION_NON_SOLID;
}

std::string CHudFrameDecoration::getDisplayName() {
    return "HUD Frame";
}

bool CHudFrameDecoration::onInputOnDeco(const eInputType, const Vector2D&, std::any) {
    return false;
}
