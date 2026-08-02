#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/Messages/EventMessage.h>

struct PL_RMLUIPLUGIN_DLL plMsgRmlUiReload : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgRmlUiReload, plMessage);
};

//////////////////////////////////////////////////////////////////////////

struct PL_RMLUIPLUGIN_DLL plMsgRmlUiEvent : plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgRmlUiEvent, plEventMessage);

  plHashedString m_sIdentifier;
  plHashedString m_sType;
};
