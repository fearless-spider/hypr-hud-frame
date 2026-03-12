#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <GLES3/gl32.h>
#include "HudAnimationState.hpp"
#include "HudConfig.hpp"

struct SHudFrameData {
    CBox            decoBox;       // full decoration box in monitor-local coords
    Vector2D        windowSize;    // inner window size
    SHudFrameConfig config;
    CHudAnimationState animState;
    float           alpha = 1.0f;
    float           scale = 1.0f;
};

class CHudFramePassElement : public IPassElement {
  public:
    explicit CHudFramePassElement(const SHudFrameData& data);
    virtual ~CHudFramePassElement() = default;

    virtual void                draw(const CRegion& damage) override;
    virtual bool                needsLiveBlur() override;
    virtual bool                needsPrecomputeBlur() override;
    virtual const char*         passName() override;
    virtual std::optional<CBox> boundingBox() override;
    virtual bool                disableSimplification() override;

    static bool initShader();
    static void destroyShader();

  private:
    SHudFrameData m_data;

    // Raw OpenGL resources — no reliance on CShader internals
    static inline GLuint s_program          = 0;
    static inline GLuint s_vao              = 0;
    static inline GLuint s_vbo              = 0;
    static inline bool   s_initialized      = false;

    // All uniform locations via glGetUniformLocation
    static inline GLint s_locProj           = -1;
    static inline GLint s_locTopLeft        = -1;
    static inline GLint s_locFullSize       = -1;
    static inline GLint s_locResolution     = -1;
    static inline GLint s_locWindowSize     = -1;
    static inline GLint s_locBorderWidth    = -1;
    static inline GLint s_locGlowRadius     = -1;
    static inline GLint s_locNotchSize      = -1;
    static inline GLint s_locBorderColor    = -1;
    static inline GLint s_locGlowColor      = -1;
    static inline GLint s_locTime           = -1;
    static inline GLint s_locPulseSpeed     = -1;
    static inline GLint s_locPulseIntensity = -1;
    static inline GLint s_locActiveFade     = -1;
    static inline GLint s_locShowEdgeDecor  = -1;
    static inline GLint s_locShowHashMarks  = -1;
    static inline GLint s_locShowCornerNotch = -1;

    static GLuint compileShader(GLenum type, const char* src);
};
