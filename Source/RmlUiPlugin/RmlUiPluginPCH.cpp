#include <RmlUiPlugin/RmlUiPluginPCH.h>

PL_STATICLINK_LIBRARY(RmlUiPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
  PL_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiMessages);
  PL_STATICLINK_REFERENCE(RmlUiPlugin_Implementation_RmlUiRenderer);
  PL_STATICLINK_REFERENCE(RmlUiPlugin_Resources_RmlUiResource);
  PL_STATICLINK_REFERENCE(RmlUiPlugin_Startup);
}
