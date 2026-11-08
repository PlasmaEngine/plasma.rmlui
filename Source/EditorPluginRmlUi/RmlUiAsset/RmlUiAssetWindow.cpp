#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/DocumentWindow/EngineViewWidget.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetWindow.h>

plRmlUiAssetDocumentWindow::plRmlUiAssetDocumentWindow(plAssetDocument* pDocument)
  : plEngineDocumentWindow(pDocument)
{
  SetMenuBarActions("RmlUiAssetMenuBar");
  SetToolBarActions("RmlUiAssetToolBar");

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plEngineViewWidget(this, &m_ViewConfig);
    AddViewWidget(m_pViewWidget);
  }

  // the shell's properties panel follows the selection
  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  m_pAssetDoc = static_cast<plRmlUiAssetDocument*>(pDocument);
}

void plRmlUiAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plEngineDocumentWindow::InternalRedraw();
}

void plRmlUiAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
