#include "HudConfig.hpp"
#include <hyprlang.hpp>
#include <sstream>

static const std::string CFG_PREFIX = "plugin:hypr-hud-frame:";

void CHudConfig::registerConfig(HANDLE handle) {
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "border_width",      Hyprlang::INT{2});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "glow_radius",       Hyprlang::INT{10});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "notch_size",        Hyprlang::INT{8});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "padding",           Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "pulse_speed",       Hyprlang::FLOAT{2.0f});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "pulse_intensity",   Hyprlang::FLOAT{0.12f});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "show_edge_decor",   Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "show_hash_marks",   Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "show_corner_notch", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "enabled",           Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "exclude_classes",   Hyprlang::STRING{""});

    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "col.active_border",   Hyprlang::INT{0x61E2FFCC});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "col.inactive_border", Hyprlang::INT{0x6B5F7B66});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "col.glow",            Hyprlang::INT{0x61E2FF44});
    HyprlandAPI::addConfigValue(handle, CFG_PREFIX + "col.glow_inactive",   Hyprlang::INT{0x6B5F7B11});
}

static void unpackColor(int64_t hex, float& r, float& g, float& b, float& a) {
    uint32_t c = static_cast<uint32_t>(hex);
    r = static_cast<float>((c >> 24) & 0xFF) / 255.0f;
    g = static_cast<float>((c >> 16) & 0xFF) / 255.0f;
    b = static_cast<float>((c >> 8)  & 0xFF) / 255.0f;
    a = static_cast<float>((c >> 0)  & 0xFF) / 255.0f;
}

SHudFrameConfig CHudConfig::readConfig(HANDLE handle) {
    SHudFrameConfig cfg;

    auto getInt = [&](const std::string& key) -> int64_t {
        auto* val = HyprlandAPI::getConfigValue(handle, CFG_PREFIX + key);
        if (!val) return 0;
        return std::any_cast<Hyprlang::INT>(val->getValue());
    };

    auto getFloat = [&](const std::string& key) -> float {
        auto* val = HyprlandAPI::getConfigValue(handle, CFG_PREFIX + key);
        if (!val) return 0.0f;
        return std::any_cast<Hyprlang::FLOAT>(val->getValue());
    };

    auto getString = [&](const std::string& key) -> std::string {
        auto* val = HyprlandAPI::getConfigValue(handle, CFG_PREFIX + key);
        if (!val) return "";
        return std::any_cast<Hyprlang::STRING>(val->getValue());
    };

    cfg.borderWidth     = static_cast<int>(getInt("border_width"));
    cfg.glowRadius      = static_cast<int>(getInt("glow_radius"));
    cfg.notchSize        = static_cast<int>(getInt("notch_size"));
    cfg.padding          = static_cast<int>(getInt("padding"));
    cfg.pulseSpeed      = getFloat("pulse_speed");
    cfg.pulseIntensity  = getFloat("pulse_intensity");
    cfg.showEdgeDecor   = getInt("show_edge_decor") != 0;
    cfg.showHashMarks   = getInt("show_hash_marks") != 0;
    cfg.showCornerNotch = getInt("show_corner_notch") != 0;
    cfg.enabled         = getInt("enabled") != 0;
    cfg.excludeClasses  = getString("exclude_classes");

    unpackColor(getInt("col.active_border"),   cfg.activeBorderR, cfg.activeBorderG, cfg.activeBorderB, cfg.activeBorderA);
    unpackColor(getInt("col.inactive_border"), cfg.inactiveBorderR, cfg.inactiveBorderG, cfg.inactiveBorderB, cfg.inactiveBorderA);
    unpackColor(getInt("col.glow"),            cfg.glowR, cfg.glowG, cfg.glowB, cfg.glowA);
    unpackColor(getInt("col.glow_inactive"),   cfg.glowInactiveR, cfg.glowInactiveG, cfg.glowInactiveB, cfg.glowInactiveA);

    return cfg;
}

bool CHudConfig::isExcluded(const SHudFrameConfig& cfg, const std::string& windowClass) {
    if (cfg.excludeClasses.empty())
        return false;

    std::istringstream ss(cfg.excludeClasses);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t start = token.find_first_not_of(" \t");
        size_t end   = token.find_last_not_of(" \t");
        if (start == std::string::npos)
            continue;
        token = token.substr(start, end - start + 1);
        if (token == windowClass)
            return true;
    }
    return false;
}
