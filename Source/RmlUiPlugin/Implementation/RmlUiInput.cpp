
#include <RmlUiPlugin/RmlUiInput.h>
#include <Core/Input/InputManager.h>

// Define storage for static constexpr arrays
constexpr plRmlUiInputButtons::MouseButtonMapping plRmlUiInputButtons::s_MouseButtonMappings[];
constexpr plRmlUiInputButtons::KeyMapping plRmlUiInputButtons::s_KeyMappings[];

plRmlUiInputSnapshot plRmlUiInputSnapshot::MakeFromCurrentInput()
{
  plRmlUiInputSnapshot snapshot = plRmlUiInputSnapshot::MakeEmpty();

  const bool bCtrlPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftCtrl) >= plKeyState::Pressed ||
    plInputManager::GetInputSlotState(plInputSlot_KeyRightCtrl) >= plKeyState::Pressed;
  const bool bShiftPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftShift) >= plKeyState::Pressed ||
    plInputManager::GetInputSlotState(plInputSlot_KeyRightShift) >= plKeyState::Pressed;
  const bool bAltPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftAlt) >= plKeyState::Pressed ||
    plInputManager::GetInputSlotState(plInputSlot_KeyRightAlt) >= plKeyState::Pressed;

  if (bCtrlPressed)
    snapshot.m_Buttons |= plRmlUiInputButtons::Ctrl;
  if (bShiftPressed)
    snapshot.m_Buttons |= plRmlUiInputButtons::Shift;
  if (bAltPressed)
    snapshot.m_Buttons |= plRmlUiInputButtons::Alt;

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(plRmlUiInputButtons::s_MouseButtonMappings); ++i)
  {
    plRmlUiInputButtons::MouseButtonMapping mbm = plRmlUiInputButtons::s_MouseButtonMappings[i];
    if (plInputManager::GetInputSlotState(mbm.szEzButton) >= plKeyState::Pressed)
      snapshot.m_Buttons |= mbm.uiEzButton;
  }

  if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelDown) == plKeyState::Pressed)
  {
    snapshot.m_Buttons |= plRmlUiInputButtons::MouseWheelDown;
  }
  if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelUp) == plKeyState::Pressed)
  {
    snapshot.m_Buttons |= plRmlUiInputButtons::MouseWheelUp;
  }

  snapshot.m_uiLastCharacter = plInputManager::RetrieveLastCharacter(false);

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(plRmlUiInputButtons::s_KeyMappings); ++i)
  {
    plRmlUiInputButtons::KeyMapping km = plRmlUiInputButtons::s_KeyMappings[i];
    if (plInputManager::GetInputSlotState(km.szEzKey) >= plKeyState::Pressed)
      snapshot.m_Buttons |= km.uiEzKey;
  }

  return snapshot;
}


bool plRmlUiInputProvider::Update(plRmlUiInputSnapshot input)
{
  bool bHasChanged = m_Buttons != input.m_Buttons || m_uiLastCharacter != input.m_uiLastCharacter;
  m_PrevButtons = m_Buttons;
  m_Buttons = input.m_Buttons;
  m_uiLastCharacter = input.m_uiLastCharacter;
  return bHasChanged;
}

plKeyState::Enum plRmlUiInputProvider::GetButtonState(plRmlUiInputButtons::Enum button) const
{
  if (m_Buttons.IsSet(button))
  {
    if (!m_PrevButtons.IsSet(button))
      return plKeyState::Pressed;
    return plKeyState::Down;
  }
  else
  {
    if (m_PrevButtons.IsSet(button))
      return plKeyState::Released;
    return plKeyState::Up;
  }
}
