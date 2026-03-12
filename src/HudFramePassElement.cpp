#include "HudFramePassElement.hpp"
#include "Shaders.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <cstdio>

// ─── Shader compilation ────────────────────────────────────────────────

GLuint CHudFramePassElement::compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "[HUD Frame] Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool CHudFramePassElement::initShader() {
    if (s_initialized)
        return true;

    // Compile vertex shader
    GLuint vert = compileShader(GL_VERTEX_SHADER, HudShaders::VERT);
    if (!vert) return false;

    // Compile fragment shader
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, HudShaders::FRAG);
    if (!frag) { glDeleteShader(vert); return false; }

    // Link program
    s_program = glCreateProgram();
    glAttachShader(s_program, vert);
    glAttachShader(s_program, frag);
    glLinkProgram(s_program);

    GLint ok = 0;
    glGetProgramiv(s_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(s_program, sizeof(log), nullptr, log);
        fprintf(stderr, "[HUD Frame] Program link error: %s\n", log);
        glDeleteProgram(s_program);
        glDeleteShader(vert);
        glDeleteShader(frag);
        s_program = 0;
        return false;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    // Get all uniform locations
    s_locProj            = glGetUniformLocation(s_program, "proj");
    s_locTopLeft         = glGetUniformLocation(s_program, "topLeft");
    s_locFullSize        = glGetUniformLocation(s_program, "fullSize");
    s_locResolution      = glGetUniformLocation(s_program, "u_resolution");
    s_locWindowSize      = glGetUniformLocation(s_program, "u_windowSize");
    s_locBorderWidth     = glGetUniformLocation(s_program, "u_borderWidth");
    s_locGlowRadius      = glGetUniformLocation(s_program, "u_glowRadius");
    s_locNotchSize       = glGetUniformLocation(s_program, "u_notchSize");
    s_locBorderColor     = glGetUniformLocation(s_program, "u_borderColor");
    s_locGlowColor       = glGetUniformLocation(s_program, "u_glowColor");
    s_locTime            = glGetUniformLocation(s_program, "u_time");
    s_locPulseSpeed      = glGetUniformLocation(s_program, "u_pulseSpeed");
    s_locPulseIntensity  = glGetUniformLocation(s_program, "u_pulseIntensity");
    s_locActiveFade      = glGetUniformLocation(s_program, "u_activeFade");
    s_locShowEdgeDecor   = glGetUniformLocation(s_program, "u_showEdgeDecor");
    s_locShowHashMarks   = glGetUniformLocation(s_program, "u_showHashMarks");
    s_locShowCornerNotch = glGetUniformLocation(s_program, "u_showCornerNotch");

    // Create VAO + VBO with unit quad
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    // Unit quad: pos (0-1) and texcoord (0-1)
    GLfloat vertices[] = {
        // pos       texcoord
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 1.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // layout(location = 0) in vec2 pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location = 1) in vec2 texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    s_initialized = true;
    fprintf(stderr, "[HUD Frame] Shader initialized OK (program=%u, vao=%u)\n", s_program, s_vao);
    return true;
}

void CHudFramePassElement::destroyShader() {
    if (s_vao) { glDeleteVertexArrays(1, &s_vao); s_vao = 0; }
    if (s_vbo) { glDeleteBuffers(1, &s_vbo); s_vbo = 0; }
    if (s_program) { glDeleteProgram(s_program); s_program = 0; }
    s_initialized = false;
}

// ─── Pass Element ──────────────────────────────────────────────────────

CHudFramePassElement::CHudFramePassElement(const SHudFrameData& data) : m_data(data) {}

void CHudFramePassElement::draw(const CRegion& damage) {
    // Lazy init — need GL context which isn't available at PLUGIN_INIT time
    if (!s_initialized) {
        if (!initShader())
            return;
    }

    if (!s_program)
        return;

    if (damage.empty())
        return;

    const auto& box   = m_data.decoBox;
    const auto& cfg   = m_data.config;
    const auto& anim  = m_data.animState;
    float       scale = m_data.scale;

    // Save current GL program
    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    glUseProgram(s_program);

    // Projection matrix from Hyprland's render state
    auto projMatrix = g_pHyprOpenGL->m_renderData.projection.getMatrix();
    glUniformMatrix3fv(s_locProj, 1, GL_FALSE, projMatrix.data());

    // Vertex shader: map unit quad to decoration box
    glUniform4f(s_locTopLeft, static_cast<float>(box.x), static_cast<float>(box.y), 0.0f, 0.0f);
    glUniform4f(s_locFullSize, static_cast<float>(box.w), static_cast<float>(box.h), 0.0f, 0.0f);

    // Fragment shader geometry
    glUniform2f(s_locResolution, static_cast<float>(box.w) * scale, static_cast<float>(box.h) * scale);
    glUniform2f(s_locWindowSize, static_cast<float>(m_data.windowSize.x) * scale, static_cast<float>(m_data.windowSize.y) * scale);
    glUniform1f(s_locBorderWidth, static_cast<float>(cfg.borderWidth) * scale);
    glUniform1f(s_locGlowRadius, static_cast<float>(cfg.glowRadius) * scale);
    glUniform1f(s_locNotchSize, static_cast<float>(cfg.notchSize) * scale);

    // Interpolate colors
    float fade = anim.activeFade;
    float br = cfg.inactiveBorderR + (cfg.activeBorderR - cfg.inactiveBorderR) * fade;
    float bg = cfg.inactiveBorderG + (cfg.activeBorderG - cfg.inactiveBorderG) * fade;
    float bb = cfg.inactiveBorderB + (cfg.activeBorderB - cfg.inactiveBorderB) * fade;
    float ba = cfg.inactiveBorderA + (cfg.activeBorderA - cfg.inactiveBorderA) * fade;
    ba *= m_data.alpha;

    float glr = cfg.glowInactiveR + (cfg.glowR - cfg.glowInactiveR) * fade;
    float glg = cfg.glowInactiveG + (cfg.glowG - cfg.glowInactiveG) * fade;
    float glb = cfg.glowInactiveB + (cfg.glowB - cfg.glowInactiveB) * fade;
    float gla = cfg.glowInactiveA + (cfg.glowA - cfg.glowInactiveA) * fade;
    gla *= m_data.alpha;

    glUniform4f(s_locBorderColor, br, bg, bb, ba);
    glUniform4f(s_locGlowColor, glr, glg, glb, gla);

    // Animation
    glUniform1f(s_locTime, anim.time);
    glUniform1f(s_locPulseSpeed, cfg.pulseSpeed);
    glUniform1f(s_locPulseIntensity, cfg.pulseIntensity);
    glUniform1f(s_locActiveFade, fade);

    // Features
    glUniform1i(s_locShowEdgeDecor, cfg.showEdgeDecor ? 1 : 0);
    glUniform1i(s_locShowHashMarks, cfg.showHashMarks ? 1 : 0);
    glUniform1i(s_locShowCornerNotch, cfg.showCornerNotch ? 1 : 0);

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw with our VAO
    glBindVertexArray(s_vao);

    auto rects = damage.getRects();
    for (const auto& rect : rects) {
        CBox scissorBox = {static_cast<double>(rect.x1), static_cast<double>(rect.y1),
                          static_cast<double>(rect.x2 - rect.x1), static_cast<double>(rect.y2 - rect.y1)};
        g_pHyprOpenGL->scissor(scissorBox);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glBindVertexArray(0);
    g_pHyprOpenGL->scissor(CBox{});

    // Restore previous program
    glUseProgram(prevProgram);
}

bool CHudFramePassElement::needsLiveBlur() {
    return false;
}

bool CHudFramePassElement::needsPrecomputeBlur() {
    return false;
}

const char* CHudFramePassElement::passName() {
    return "CHudFramePassElement";
}

std::optional<CBox> CHudFramePassElement::boundingBox() {
    return m_data.decoBox;
}

bool CHudFramePassElement::disableSimplification() {
    return true;
}
