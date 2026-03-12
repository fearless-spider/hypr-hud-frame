#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <string>

struct SHudFrameConfig {
    int   borderWidth    = 2;
    int   glowRadius     = 10;
    int   notchSize      = 8;
    int   padding        = 0;        // gap between window edge and border       // corner notch/clip size (0 = sharp 90°)
    float pulseSpeed     = 2.0f;
    float pulseIntensity = 0.12f;
    bool  showEdgeDecor  = true;    // dots + accent lines
    bool  showHashMarks  = true;    // diagonal hash marks at corners
    bool  showCornerNotch = true;   // angular corner clips
    bool  enabled        = true;

    // Colors stored as RGBA floats
    float activeBorderR = 0.380f, activeBorderG = 0.886f, activeBorderB = 1.0f, activeBorderA = 0.8f;
    float inactiveBorderR = 0.420f, inactiveBorderG = 0.373f, inactiveBorderB = 0.482f, inactiveBorderA = 0.4f;
    float glowR = 0.380f, glowG = 0.886f, glowB = 1.0f, glowA = 0.267f;
    float glowInactiveR = 0.420f, glowInactiveG = 0.373f, glowInactiveB = 0.482f, glowInactiveA = 0.067f;

    std::string excludeClasses;
};

class CHudConfig {
  public:
    static void            registerConfig(HANDLE handle);
    static SHudFrameConfig readConfig(HANDLE handle);
    static bool            isExcluded(const SHudFrameConfig& cfg, const std::string& windowClass);
};
