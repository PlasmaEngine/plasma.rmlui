#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiDocumentContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiViewContext.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiDocumentContext, 1, plRTTIDefaultAllocator<plRmlUiDocumentContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiDocumentContext::plRmlUiDocumentContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

plRmlUiDocumentContext::~plRmlUiDocumentContext() = default;

void plRmlUiDocumentContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  // Preview object
  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("RmlUiPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    plRmlUiCanvas2DComponent* pComponent = nullptr;
    plRmlUiCanvas2DComponent::CreateComponent(m_pMainObject, pComponent);

    pComponent->SetPassInput(false);
    pComponent->SetOnDemandUpdate(false);
    pComponent->SetAutobindBlackboards(false); // there is no blackboard that could be bound in this context

    plStringBuilder sResourceGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sResourceGuid);
    m_hMainResource = plResourceManager::LoadResource<plRmlUiResource>(sResourceGuid);

    pComponent->SetRmlResource(m_hMainResource);
  }
}

plEngineProcessViewContext* plRmlUiDocumentContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plRmlUiViewContext, this);
}

void plRmlUiDocumentContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

bool plRmlUiDocumentContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  PL_LOCK(m_pMainObject->GetWorld()->GetWriteMarker());

  m_pMainObject->UpdateLocalBounds();
  plBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  plRmlUiViewContext* pMeshViewContext = static_cast<plRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
