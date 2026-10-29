#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plEngineViewWidget;

class plRmlUiAssetDocumentWindow : public plEngineDocumentWindow
{
public:
  plRmlUiAssetDocumentWindow(plAssetDocument* pDocument);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();

  plEngineViewConfig m_ViewConfig;
  plEngineViewWidget* m_pViewWidget;
  plRmlUiAssetDocument* m_pAssetDoc;
};
