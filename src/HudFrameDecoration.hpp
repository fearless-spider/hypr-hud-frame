#pragma once

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include "HudAnimationState.hpp"
#include "HudConfig.hpp"

class CHudFrameDecoration : public IHyprWindowDecoration {
  public:
    CHudFrameDecoration(PHLWINDOW pWindow, HANDLE pluginHandle);
    virtual ~CHudFrameDecoration();

    virtual SDecorationPositioningInfo getPositioningInfo() override;
    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply) override;
    virtual void                       draw(PHLMONITOR, float const& a) override;
    virtual eDecorationType            getDecorationType() override;
    virtual void                       updateWindow(PHLWINDOW) override;
    virtual void                       damageEntire() override;
    virtual bool                       onInputOnDeco(const eInputType, const Vector2D&, std::any = {}) override;
    virtual eDecorationLayer           getDecorationLayer() override;
    virtual uint64_t                   getDecorationFlags() override;
    virtual std::string                getDisplayName() override;

  private:
    PHLWINDOWREF       m_pWindow;
    HANDLE             m_pluginHandle;
    CHudAnimationState m_animState;
    SBoxExtents        m_extents;
    bool               m_lastFocusState = false;

    bool isWindowActive() const;
};
