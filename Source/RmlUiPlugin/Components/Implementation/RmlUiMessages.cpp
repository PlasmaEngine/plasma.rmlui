#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiMessages.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgRmlUiReload);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRmlUiReload, 1, plRTTIDefaultAllocator<plMsgRmlUiReload>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript(),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgRmlUiEvent);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRmlUiEvent, 1, plRTTIDefaultAllocator<plMsgRmlUiEvent>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Identifier", m_sIdentifier),
    PL_MEMBER_PROPERTY("Type", m_sType),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiMessages);
