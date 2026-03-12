#pragma once

#include <string>

namespace HudShaders {

inline constexpr auto VERT = R"glsl(
#version 320 es
precision highp float;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texcoord;

out vec2 v_texcoord;

uniform mat3 proj;
uniform vec4 topLeft;
uniform vec4 fullSize;

void main() {
    vec2 scaledPos = pos * fullSize.xy + topLeft.xy;
    gl_Position = vec4((proj * vec3(scaledPos, 1.0)).xy, 0.0, 1.0);
    v_texcoord = pos;
}
)glsl";

inline constexpr auto FRAG = R"glsl(
#version 320 es
precision highp float;

in vec2 v_texcoord;
out vec4 fragColor;

// Geometry
uniform vec2 u_resolution;      // decoration box size in px
uniform vec2 u_windowSize;      // inner window size in px
uniform float u_borderWidth;
uniform float u_glowRadius;
uniform float u_notchSize;      // corner notch/clip size

// Color
uniform vec4 u_borderColor;
uniform vec4 u_glowColor;

// Animation
uniform float u_time;
uniform float u_pulseSpeed;
uniform float u_pulseIntensity;
uniform float u_activeFade;

// Features
uniform int u_showEdgeDecor;
uniform int u_showHashMarks;
uniform int u_showCornerNotch;

// ═══════════════════════════════════════════════════
// SDF: Sharp rectangle (no rounding, no chamfer)
// ═══════════════════════════════════════════════════
float sdRect(vec2 p, vec2 halfSize) {
    vec2 d = abs(p) - halfSize;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// ═══════════════════════════════════════════════════
// SDF: Line segment
// ═══════════════════════════════════════════════════
float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

// ═══════════════════════════════════════════════════
// SDF: Circle
// ═══════════════════════════════════════════════════
float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

// ═══════════════════════════════════════════════════
// Corner notch — small angular clip at each corner
// Like the reference: tiny 45° cut at corners
// ═══════════════════════════════════════════════════
float sdCornerNotches(vec2 p, vec2 halfSize, float notch) {
    // Each corner gets a small diagonal plane cut
    float d = -1e10;
    for (int ix = -1; ix <= 1; ix += 2) {
        for (int iy = -1; iy <= 1; iy += 2) {
            vec2 corner = vec2(float(ix), float(iy)) * halfSize;
            vec2 toCorner = p - corner;
            // Diagonal plane: x + y = notch (in corner-local coords)
            float planeDist = (float(-ix) * toCorner.x + float(-iy) * toCorner.y) - notch;
            d = max(d, -planeDist);
        }
    }
    return d;
}

// ═══════════════════════════════════════════════════
// Diagonal hash marks at corners (like reference)
// Short parallel diagonal lines near each corner
// ═══════════════════════════════════════════════════
float sdCornerHashMarks(vec2 p, vec2 halfSize) {
    float d = 1e10;
    float markLen = 10.0;
    float spacing = 5.0;

    for (int ix = -1; ix <= 1; ix += 2) {
        for (int iy = -1; iy <= 1; iy += 2) {
            vec2 corner = vec2(float(ix), float(iy)) * halfSize;
            vec2 dir45 = normalize(vec2(float(-ix), float(-iy)));
            vec2 perp = vec2(dir45.y, -dir45.x);

            // 3 parallel diagonal marks near each corner
            for (int i = 0; i < 3; i++) {
                vec2 offset = perp * (float(i) * spacing - spacing);
                vec2 a = corner + offset;
                vec2 b = corner + offset + dir45 * markLen;
                d = min(d, sdSegment(p, a, b));
            }
        }
    }
    return d;
}

// ═══════════════════════════════════════════════════
// Edge dots — small dots along top/bottom edges
// Like the reference: row of dots near border
// ═══════════════════════════════════════════════════
float sdEdgeDots(vec2 p, vec2 halfSize) {
    float d = 1e10;
    float dotR = 1.2;
    float spacing = 8.0;
    float inset = 6.0; // how far inside the border

    // Dots along top-left area (like reference)
    float startX = -halfSize.x + 15.0;
    float endX = startX + 50.0;
    for (float x = startX; x < endX; x += spacing) {
        d = min(d, sdCircle(p - vec2(x, -halfSize.y + inset), dotR));
    }

    // Dots along bottom-left area
    for (float x = startX; x < endX; x += spacing) {
        d = min(d, sdCircle(p - vec2(x, halfSize.y - inset), dotR));
    }

    return d;
}

// ═══════════════════════════════════════════════════
// Edge accent lines — short horizontal lines at edges
// Like the reference: small line accents
// ═══════════════════════════════════════════════════
float sdEdgeAccents(vec2 p, vec2 halfSize) {
    float d = 1e10;
    float lineLen = 30.0;

    // Top-right: short horizontal line just inside border
    float tx = halfSize.x - 15.0;
    d = min(d, sdSegment(p, vec2(tx - lineLen, -halfSize.y + 5.0), vec2(tx, -halfSize.y + 5.0)));

    // Bottom-right: short horizontal line
    d = min(d, sdSegment(p, vec2(tx - lineLen, halfSize.y - 5.0), vec2(tx, halfSize.y - 5.0)));

    // Small chevron arrows on right edge (mid-height)
    float cy = 0.0;
    float chevSize = 4.0;
    float rx = halfSize.x - 5.0;
    d = min(d, sdSegment(p, vec2(rx, cy - chevSize), vec2(rx + chevSize, cy)));
    d = min(d, sdSegment(p, vec2(rx + chevSize, cy), vec2(rx, cy + chevSize)));

    return d;
}

void main() {
    vec2 pixelPos = (v_texcoord - 0.5) * u_resolution;
    vec2 windowHalf = u_windowSize * 0.5;

    // ──── Sharp Rectangle SDF ────
    float rectDist = sdRect(pixelPos, windowHalf);

    // ──── Corner notch modification ────
    float shapeDist = rectDist;
    if (u_showCornerNotch == 1 && u_notchSize > 0.5) {
        float notchDist = sdCornerNotches(pixelPos, windowHalf, u_notchSize);
        // Union: the shape is rect with corner cuts removed
        shapeDist = max(rectDist, notchDist);
    }

    // ──── Layer 1: Outer Glow ────
    float glowSigma = u_glowRadius * 0.4;
    float glowDist = max(shapeDist, 0.0);
    float glowFactor = exp(-(glowDist * glowDist) / (glowSigma * glowSigma * 2.0));
    glowFactor *= step(0.0, shapeDist);
    glowFactor *= u_activeFade;

    // Pulse
    float pulse = 1.0 + sin(u_time * u_pulseSpeed * 6.28318) * u_pulseIntensity * u_activeFade;

    vec4 color = u_glowColor * glowFactor * pulse;

    // ──── Layer 2: Main Border (outer line) ────
    float outerEdge = smoothstep(0.5, -0.5, shapeDist);
    float innerEdge = smoothstep(0.5, -0.5, shapeDist + u_borderWidth);
    float borderMask = outerEdge - innerEdge;

    vec4 borderContrib = u_borderColor * borderMask * pulse;
    color = color + borderContrib * (1.0 - color.a);

    // ──── Layer 3: Inner accent line (thinner, lower opacity) ────
    float accentGap = u_borderWidth + 3.0;
    float accentDist;
    if (u_showCornerNotch == 1 && u_notchSize > 0.5) {
        float innerRect = sdRect(pixelPos, windowHalf - accentGap);
        float innerNotch = sdCornerNotches(pixelPos, windowHalf - accentGap, u_notchSize * 0.6);
        accentDist = max(innerRect, innerNotch);
    } else {
        accentDist = sdRect(pixelPos, windowHalf - accentGap);
    }
    float accentOuter = smoothstep(0.5, -0.5, accentDist);
    float accentInner = smoothstep(0.5, -0.5, accentDist + 1.0);
    float accentMask = (accentOuter - accentInner) * 0.35 * u_activeFade;

    vec4 accentColor = vec4(u_borderColor.rgb, u_borderColor.a * accentMask);
    color = color + accentColor * (1.0 - color.a);

    // ──── Layer 4: Corner hash marks (diagonal lines) ────
    if (u_showHashMarks == 1) {
        float hashDist = sdCornerHashMarks(pixelPos, windowHalf - u_borderWidth * 0.5);
        float hashMask = smoothstep(1.2, 0.3, hashDist) * 0.7 * u_activeFade;
        vec4 hashColor = vec4(u_borderColor.rgb, u_borderColor.a * hashMask);
        color = color + hashColor * (1.0 - color.a);
    }

    // ──── Layer 5: Edge decorations (dots + accent lines) ────
    if (u_showEdgeDecor == 1) {
        // Dots
        float dotDist = sdEdgeDots(pixelPos, windowHalf);
        float dotMask = smoothstep(1.0, 0.0, dotDist) * 0.7 * u_activeFade;
        vec4 dotColor = vec4(u_borderColor.rgb, u_borderColor.a * dotMask);
        color = color + dotColor * (1.0 - color.a);

        // Accent lines + chevrons
        float accentLineDist = sdEdgeAccents(pixelPos, windowHalf);
        float accentLineMask = smoothstep(1.2, 0.3, accentLineDist) * 0.6 * u_activeFade;
        vec4 accentLineColor = vec4(u_borderColor.rgb, u_borderColor.a * accentLineMask);
        color = color + accentLineColor * (1.0 - color.a);
    }

    // ──── Discard transparent ────
    if (color.a < 0.002)
        discard;

    fragColor = color;
}
)glsl";

} // namespace HudShaders
