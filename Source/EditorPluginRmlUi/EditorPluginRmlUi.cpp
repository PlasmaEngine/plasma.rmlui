#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.h>
#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <EditorUi/Widgets.h>
#include <Foundation/Configuration/Plugin.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

/// The package's icon folder: the icons under their ids (":/AssetIcons/RmlUi.svg" is
/// "EditorIcons/AssetIcons/RmlUi.svg"). The plugin binary lives in <package>/Bin/<configuration>.
static plString s_sIconFolder;

static void RegisterIconFolder()
{
  plStringBuilder sPlugin, sCopy;
  plPlugin::GetPluginPaths("plEditorPluginRmlUi", sPlugin, sCopy, 0);

  plStringBuilder sFolder = sPlugin.GetFileDirectory();
  sFolder.AppendPath("..", "..", "EditorIcons");
  sFolder.MakeCleanPath();

  s_sIconFolder = sFolder;
  plEditorUi::AddIconFolder(s_sIconFolder);
}

static void UnregisterIconFolder()
{
  plEditorUi::RemoveIconFolder(s_sIconFolder);
  s_sIconFolder.Clear();
}

void OnLoadPlugin()
{
  RegisterIconFolder();

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
  UnregisterIconFolder();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
