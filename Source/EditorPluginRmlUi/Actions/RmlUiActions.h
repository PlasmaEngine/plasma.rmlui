#pragma once

#include <EditorPluginRmlUi/EditorPluginRmlUiDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plRmlUiAssetDocument;
struct plRmlUiAssetEvent;

class plRmlUiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActionsMenu(plStringView sMapping);
  static void MapActionsToolbar(plStringView sMapping);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hOpenInVSC;
};

class plRmlUiAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiAction, plButtonAction);

public:
  enum class ActionType
  {
    OpenInVSC,
  };

  plRmlUiAction(const plActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const plVariant& value) override;

private:
  plRmlUiAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
