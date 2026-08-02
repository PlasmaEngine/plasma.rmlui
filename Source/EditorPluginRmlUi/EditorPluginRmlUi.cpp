#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    plRmlUiActions::RegisterActions();

    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
      //plRmlUiActions::MapActionsMenu("RmlUiAssetMenuBar");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      plRmlUiActions::MapActionsToolbar("RmlUiAssetToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  plRmlUiActions::UnregisterActions();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
