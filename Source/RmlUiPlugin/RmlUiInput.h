#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>
#include <Core/Input/Declarations.h>

struct plRmlUiInputButtons
{
  using StorageType = plUInt32;

  enum Enum
  {
    None             = 0,

    Mouse0           = PL_BIT(0),
    Mouse1           = PL_BIT(1),
    Mouse2           = PL_BIT(2),
    MouseWheelUp     = PL_BIT(3),
    MouseWheelDown   = PL_BIT(4),
    Tab              = PL_BIT(5),
    Left             = PL_BIT(6),
    Up               = PL_BIT(7),
    Right            = PL_BIT(8),
    Down             = PL_BIT(9),
    PageUp           = PL_BIT(10),
    PageDown         = PL_BIT(11),
    Home             = PL_BIT(12),
    End              = PL_BIT(13),
    Delete           = PL_BIT(14),
    Backspace        = PL_BIT(15),
    Return           = PL_BIT(16),
    NumpadEnter      = PL_BIT(17),
    Escape           = PL_BIT(18),
    Alt              = PL_BIT(19),
    Ctrl             = PL_BIT(20),
    Shift            = PL_BIT(21),

    Default          = None,
  };

  struct Bits
  {
    StorageType Mouse0          : 1;
    StorageType Mouse1          : 1;
    StorageType Mouse2          : 1;
    StorageType MouseWheelUp    : 1;
    StorageType MouseWheelDown  : 1;
    StorageType Tab             : 1;
    StorageType Left            : 1;
    StorageType Up              : 1;
    StorageType Right           : 1;
    StorageType Down            : 1;
    StorageType PageUp          : 1;
    StorageType PageDown        : 1;
    StorageType Home            : 1;
    StorageType End             : 1;
    StorageType Delete          : 1;
    StorageType Backspace       : 1;
    StorageType Return          : 1;
    StorageType NumpadEnter     : 1;
    StorageType Escape          : 1;
    StorageType Alt             : 1;
    StorageType Ctrl            : 1;
    StorageType Shift           : 1;
  };

  struct MouseButtonMapping
  {
    Enum uiEzButton;
    plUInt32 uiRmlButton;
    const char* szEzButton;
  };

  struct KeyMapping
  {
    Enum uiEzKey;
    Rml::Input::KeyIdentifier uiRmlKey;
    const char* szEzKey;
  };

  constexpr static MouseButtonMapping s_MouseButtonMappings[] = {
    { plRmlUiInputButtons::Mouse0, 0, plInputSlot_MouseButton0 },
    { plRmlUiInputButtons::Mouse1, 1, plInputSlot_MouseButton1 },
    { plRmlUiInputButtons::Mouse2, 2, plInputSlot_MouseButton2 },
  };

  constexpr static KeyMapping s_KeyMappings[] = {
    { plRmlUiInputButtons::Tab, Rml::Input::KI_TAB, plInputSlot_KeyTab },
    { plRmlUiInputButtons::Left, Rml::Input::KI_LEFT, plInputSlot_KeyLeft },
    { plRmlUiInputButtons::Up, Rml::Input::KI_UP, plInputSlot_KeyUp },
    { plRmlUiInputButtons::Right, Rml::Input::KI_RIGHT, plInputSlot_KeyRight },
    { plRmlUiInputButtons::Down, Rml::Input::KI_DOWN, plInputSlot_KeyDown },
    { plRmlUiInputButtons::PageUp, Rml::Input::KI_PRIOR, plInputSlot_KeyPageUp },
    { plRmlUiInputButtons::PageDown, Rml::Input::KI_NEXT, plInputSlot_KeyPageDown },
    { plRmlUiInputButtons::Home, Rml::Input::KI_HOME, plInputSlot_KeyHome },
    { plRmlUiInputButtons::End, Rml::Input::KI_END, plInputSlot_KeyEnd },
    { plRmlUiInputButtons::Delete, Rml::Input::KI_DELETE, plInputSlot_KeyDelete },
    { plRmlUiInputButtons::Backspace, Rml::Input::KI_BACK, plInputSlot_KeyBackspace },
    { plRmlUiInputButtons::Return, Rml::Input::KI_RETURN, plInputSlot_KeyReturn },
    { plRmlUiInputButtons::NumpadEnter, Rml::Input::KI_NUMPADENTER, plInputSlot_KeyNumpadEnter },
    { plRmlUiInputButtons::Escape, Rml::Input::KI_ESCAPE, plInputSlot_KeyEscape },
  };
};

PL_DECLARE_FLAGS_OPERATORS(plRmlUiInputButtons);

struct PL_RMLUIPLUGIN_DLL plRmlUiInputSnapshot
{
  plBitflags<plRmlUiInputButtons> m_Buttons;
  plUInt32 m_uiLastCharacter = 0;

  PL_ALWAYS_INLINE bool operator==(const plRmlUiInputSnapshot& rhs) const
  {
    return m_Buttons == rhs.m_Buttons && m_uiLastCharacter == rhs.m_uiLastCharacter;
  }

  PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const plRmlUiInputSnapshot&);

  [[nodiscard]] PL_ALWAYS_INLINE static plRmlUiInputSnapshot MakeEmpty() { return plRmlUiInputSnapshot(); }
  [[nodiscard]] static plRmlUiInputSnapshot MakeFromCurrentInput();
};

struct PL_RMLUIPLUGIN_DLL plRmlUiInputProvider
{
  bool Update(plRmlUiInputSnapshot input);

  plKeyState::Enum GetButtonState(plRmlUiInputButtons::Enum button) const;

  PL_ALWAYS_INLINE bool IsButtonDown(plRmlUiInputButtons::Enum button) const
  {
    return m_Buttons.IsSet(button);
  }

  PL_ALWAYS_INLINE bool IsAnyButtonDown() const
  {
    return m_Buttons.IsAnyFlagSet() || m_uiLastCharacter != 0;
  }

  PL_ALWAYS_INLINE bool HasAnyInput() const
  {
    return IsAnyButtonDown() || m_PrevButtons.IsAnyFlagSet();
  }

  plUInt32 m_uiLastCharacter = 0;
  plBitflags<plRmlUiInputButtons> m_Buttons, m_PrevButtons;
};
