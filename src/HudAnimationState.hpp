#pragma once

struct CHudAnimationState {
    float pulsePhase  = 0.0f;
    float scanlineY   = 0.0f;
    float activeFade  = 0.0f;  // 0 = inactive, 1 = active
    float time        = 0.0f;  // monotonic time in seconds

    void tick(float dt, bool isActive, float fadeSpeed = 5.0f);
};
