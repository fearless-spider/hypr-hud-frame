#include "HudAnimationState.hpp"
#include <algorithm>

void CHudAnimationState::tick(float dt, bool isActive, float fadeSpeed) {
    time += dt;

    // Smooth fade between active/inactive
    float target = isActive ? 1.0f : 0.0f;
    activeFade += (target - activeFade) * std::min(1.0f, dt * fadeSpeed);
    activeFade = std::clamp(activeFade, 0.0f, 1.0f);
}
