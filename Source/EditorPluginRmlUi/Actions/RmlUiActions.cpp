#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plRmlUiActions::s_hCategory;
plActionDescriptorHandle plRmlUiActions::s_hOpenInVSC;

void plRmlUiActions::RegisterActions()
{
  s_hCategory = PL_REGISTER_CATEGORY("RmlUiCategory");
  s_hOpenInVSC = PL_REGISTER_ACTION_1("RmlUi.OpenInVSC", plActionScope::Document, "RmlUi", "", plRmlUiAction, plRmlUiAction::ActionType::OpenInVSC);
}

void plRmlUiActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hOpenInVSC);
}

void plRmlUiActions::MapActionsMenu(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "G.Asset", 5.0f);

  pMap->MapAction(s_hOpenInVSC, "RmlUiCategory", 1.0f);
}

void plRmlUiActions::MapActionsToolbar(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "RmlUiCategory";

  pMap->MapAction(s_hOpenInVSC, szSubPath, 1.0f);
}

plRmlUiAction::plRmlUiAction(const plActionContext& context, const char* szName, plRmlUiAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<plRmlUiAssetDocument*>(static_cast<const plRmlUiAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
  }
}


void plRmlUiAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      m_pDocument->OpenExternalEditor();
      return;
  }
}
