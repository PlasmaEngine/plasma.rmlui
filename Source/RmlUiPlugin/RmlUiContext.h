#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInput.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plBlackboard;

namespace plRmlUiInternal
{
  class RenderInterface;
  class EventListener;
} // namespace plRmlUiInternal

class PL_RMLUIPLUGIN_DLL plRmlUiContext final : public Rml::Context
{
public:
  plRmlUiContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler);
  ~plRmlUiContext();

public:
  plResult LoadDocumentFromResource(const plRmlUiResourceHandle& hResource);
  plResult LoadDocumentFromString(const plStringView& sContent);

  void UnloadDocument();
  plResult ReloadDocumentFromResource(const plRmlUiResourceHandle& hResource);

  bool HasDocument() { return GetNumDocuments() > 0; }

  void ShowDocument();
  void HideDocument();

  /// \brief Returns true if the input was consumed
  bool UpdateInput(const plVec2& vMousePos, const plRmlUiInputProvider& input);
  bool WantsInput() const { return m_bWantsInput; }

  void SetSize(const plVec2U32& vSize);
  void SetDpiScale(float fScale);

  using EventHandler = plDelegate<void(Rml::Event&)>;

  /// \brief Registers an event handler for a RmlUI event (such as 'onclick')
  ///
  /// There can only be one event handler for each event type.
  /// If called multiple times, the existing event handler is overridden.
  void RegisterEventHandler(const char* szIdentifier, EventHandler handler);

  /// \brief Removes a previously registered RmlUI event handler.
  void DeregisterEventHandler(const char* szIdentifier);

  using FallbackEventHandler = plDelegate<void(const plHashedString&, Rml::Event&)>;

  /// \brief Registers a fallback event handler for RmlUI events which is called when no specific event handler is registered for the event's type.
  void RegisterFallbackEventHandler(FallbackEventHandler handler);

  /// \brief Removes the previously registered fallback event handler.
  void DeregisterFallbackEventHandler();

  void Update();

private:
  friend class plRmlUi;
  void ExtractRenderData(plRmlUiInternal::RenderInterface& renderInterface, plGALTextureHandle hTexture);

  friend class plRmlUiInternal::EventListener;
  void ProcessEvent(const plHashedString& sIdentifier, Rml::Event& event);

  plHashTable<plHashedString, EventHandler> m_EventHandler;
  FallbackEventHandler m_FallbackEventHandler;

  plUInt64 m_uiUpdatedFrame = plUInt64(-1);
  plUInt64 m_uiExtractedFrame = plUInt64(-1);

  bool m_bWantsInput = false;
};

namespace plRmlUiInternal
{
  class ContextInstancer : public Rml::ContextInstancer
  {
  public:
    virtual Rml::ContextPtr InstanceContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler) override;
    virtual void ReleaseContext(Rml::Context* pContext) override;

  private:
    virtual void Release() override;
  };
} // namespace plRmlUiInternal
