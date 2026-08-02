#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginRmlUi/DragDropHandlers/RmlUiDragDropHandler.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiComponentDragDropHandler, 1, plRTTIDefaultAllocator<plRmlUiComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;


float plRmlUiComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "RmlUi") ? 1.0f : 0.0f;
}

void plRmlUiComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "plRmlUiCanvas2DComponent";
  constexpr const char* szPropertyName = "RmlFile";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), plUuid(), -1);
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
