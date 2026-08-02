#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>


plRmlUiContext::plRmlUiContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
  : Rml::Context(sName, pRenderManager, pTextInputHandler)
{
}

plRmlUiContext::~plRmlUiContext() = default;

plResult plRmlUiContext::LoadDocumentFromResource(const plRmlUiResourceHandle& hResource)
{
  return plRmlUi::GetSingleton()->LoadDocumentFromResource(*this, hResource);
}

plResult plRmlUiContext::LoadDocumentFromString(const plStringView& sContent)
{
  return plRmlUi::GetSingleton()->LoadDocumentFromString(*this, sContent);
}

void plRmlUiContext::UnloadDocument()
{
  plRmlUi::GetSingleton()->UnloadDocument(*this);
}

plResult plRmlUiContext::ReloadDocumentFromResource(const plRmlUiResourceHandle& hResource)
{
  plRmlUi::GetSingleton()->ClearCaches();

  PL_SUCCEED_OR_RETURN(LoadDocumentFromResource(hResource));

  RequestNextUpdate(0.0);

  return PL_SUCCESS;
}

void plRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    PL_LOCK(plRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Show();
  }
}

void plRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    PL_LOCK(plRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Hide();
  }

  m_bWantsInput = false;
}

bool plRmlUiContext::UpdateInput(const plVec2& vMousePos, const plRmlUiInputProvider& input)
{
  bool bMouseInputConsumed = false;
  bool bKeyboardInputConsumed = false;

  int modifierState = 0;
  modifierState |= input.IsButtonDown(plRmlUiInputButtons::Alt) ? Rml::Input::KM_ALT : 0;
  modifierState |= input.IsButtonDown(plRmlUiInputButtons::Ctrl) ? Rml::Input::KM_CTRL : 0;
  modifierState |= input.IsButtonDown(plRmlUiInputButtons::Shift) ? Rml::Input::KM_SHIFT : 0;

  // Mouse
  {
    bMouseInputConsumed |= !ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(plRmlUiInputButtons::s_MouseButtonMappings); ++i)
    {
      plRmlUiInputButtons::MouseButtonMapping mbm = plRmlUiInputButtons::s_MouseButtonMappings[i];
      plKeyState::Enum state = input.GetButtonState(mbm.uiEzButton);
      if (state == plKeyState::Pressed)
      {
        bMouseInputConsumed |= !ProcessMouseButtonDown(mbm.uiRmlButton, modifierState);
      }
      else if (state == plKeyState::Released)
      {
        bMouseInputConsumed |= !ProcessMouseButtonUp(mbm.uiRmlButton, modifierState);
      }
    }

    if (input.IsButtonDown(plRmlUiInputButtons::MouseWheelDown))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (input.IsButtonDown(plRmlUiInputButtons::MouseWheelUp))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    plUInt32 uiLastChar = input.m_uiLastCharacter;
    if (uiLastChar >= 32 || uiLastChar == '\n') // >= space (+ enter/return)
    {
      char szUtf8[8] = "";
      char* pChar = szUtf8;
      plUnicodeUtils::EncodeUtf32ToUtf8(uiLastChar, pChar);
      if (!plStringUtils::IsNullOrEmpty(szUtf8))
      {
        bKeyboardInputConsumed |= !ProcessTextInput(szUtf8);
      }
    }

    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(plRmlUiInputButtons::s_KeyMappings); ++i)
    {
      plRmlUiInputButtons::KeyMapping km = plRmlUiInputButtons::s_KeyMappings[i];
      plKeyState::Enum state = input.GetButtonState(km.uiEzKey);
      if (state == plKeyState::Pressed)
      {
        bKeyboardInputConsumed |= !ProcessKeyDown(km.uiRmlKey, modifierState);
      }
      else if (state == plKeyState::Released)
      {
        bKeyboardInputConsumed |= !ProcessKeyUp(km.uiRmlKey, modifierState);
      }
    }
  }

  m_bWantsInput = bMouseInputConsumed || bKeyboardInputConsumed;

  return bMouseInputConsumed || bKeyboardInputConsumed;
}

void plRmlUiContext::SetSize(const plVec2U32& vSize)
{
  SetDimensions(Rml::Vector2i(vSize.x, vSize.y));
}

void plRmlUiContext::SetDpiScale(float fScale)
{
  SetDensityIndependentPixelRatio(fScale);
}

void plRmlUiContext::RegisterEventHandler(const char* szIdentifier, EventHandler handler)
{
  plHashedString sIdentifier;
  sIdentifier.Assign(szIdentifier);

  m_EventHandler.Insert(sIdentifier, std::move(handler));
}

void plRmlUiContext::DeregisterEventHandler(const char* szIdentifier)
{
  m_EventHandler.Remove(plTempHashedString(szIdentifier));
}

void plRmlUiContext::RegisterFallbackEventHandler(FallbackEventHandler handler)
{
  m_FallbackEventHandler = std::move(handler);
}

void plRmlUiContext::DeregisterFallbackEventHandler()
{
  m_FallbackEventHandler.Invalidate();
}

void plRmlUiContext::Update()
{
  PL_LOCK(plRmlUi::GetSingleton()->GetContextMutex());

  Rml::Context::Update();

  m_uiUpdatedFrame = plRenderWorld::GetFrameCounter();
}

void plRmlUiContext::ExtractRenderData(plRmlUiInternal::RenderInterface& renderInterface, plGALTextureHandle hTexture)
{
  if (m_uiExtractedFrame != m_uiUpdatedFrame)
  {
    plHashedString sName;
    sName.Assign(plRmlUiConversionUtils::ToStringView(GetName()));

    renderInterface.BeginExtraction(sName, hTexture);

    Render();

    renderInterface.EndExtraction();

    m_uiExtractedFrame = m_uiUpdatedFrame;
  }
}

void plRmlUiContext::ProcessEvent(const plHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
  else if (m_FallbackEventHandler.IsValid())
  {
    m_FallbackEventHandler(sIdentifier, event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr plRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
{
  return Rml::ContextPtr(PL_DEFAULT_NEW(plRmlUiContext, sName, pRenderManager, pTextInputHandler));
}

void plRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

void plRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
