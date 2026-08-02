#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static plAtomicInteger32 s_RmlResourceCounter;

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRmlUiCanvas3DComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_RESOURCE_ACCESSOR_PROPERTY("ProxyMesh", GetProxyMesh, SetProxyMesh)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    PL_RESOURCE_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material"), new plDefaultValueAttribute("{ 05af8d07-0b38-44a6-8d50-49731ae2625d }")),
    PL_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new plDefaultValueAttribute(0)),
    PL_ACCESSOR_PROPERTY("TextureSlotName", GetTextureSlotName, SetTextureSlotName)->AddAttributes(new plDefaultValueAttribute("BaseTexture")),
    PL_ACCESSOR_PROPERTY("TextureSize", GetTextureSize, SetTextureSize)->AddAttributes(new plSuffixAttribute("px"), new plDefaultValueAttribute(plVec2U32(512, 512)), new plClampValueAttribute(plVec2U32(0), plVec2U32(4096))),
    PL_ACCESSOR_PROPERTY("DpiScale", GetDpiScale, SetDpiScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_ACCESSOR_PROPERTY("ClearStaleInput", GetClearStaleInput, SetClearStaleInput)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ACCESSOR_PROPERTY("IsInteractive", IsInteractive, SetInteractive)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input/RmlUi"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plRmlUiCanvas3DComponent::plRmlUiCanvas3DComponent()
{
  m_vSize = plVec2U32(512, 512);
}

plRmlUiCanvas3DComponent::~plRmlUiCanvas3DComponent() = default;
plRmlUiCanvas3DComponent& plRmlUiCanvas3DComponent::operator=(plRmlUiCanvas3DComponent&& rhs) = default;

void plRmlUiCanvas3DComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hBaseMaterial;
  s << m_uiMaterialIndex;
  s << m_sTextureSlotName;
  s << m_vSize;
  s << m_bClearStaleInput;
  s << m_bIsInteractive;
  s << m_fDpiScale;
  s << m_hProxyMesh;
}

void plRmlUiCanvas3DComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hBaseMaterial;
  s >> m_uiMaterialIndex;
  s >> m_sTextureSlotName;
  s >> m_vSize;
  s >> m_bClearStaleInput;
  s >> m_bIsInteractive;
  s >> m_fDpiScale;

  if (uiVersion >= 2)
  {
    s >> m_hProxyMesh;
  }
}

void plRmlUiCanvas3DComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_sTextureSlotName.IsEmpty())
  {
    m_sTextureSlotName.Assign("BaseTexture");
  }

  if (!m_hBaseMaterial.IsValid())
  {
    // use the "Fullbright" material as a fallback
    m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("{ 05af8d07-0b38-44a6-8d50-49731ae2625d }");
  }

  // Create context and load document (but don't show yet)
  GetOrCreateRmlContext();

  // Create texture and material before showing the document
  Update();

  // Now that texture, material, and context are properly configured, show the document
  if (m_pContext != nullptr)
  {
    m_pContext->ShowDocument();
  }

  if (m_hMaterial.IsValid())
  {
    plMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->PostMessageRecursive(msg, plTime::MakeZero());
  }
}

void plRmlUiCanvas3DComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_hMaterial.IsValid())
  {
    plMsgSetMeshMaterial msg;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->PostMessageRecursive(msg, plTime::MakeZero());
  }

  m_iInputAge = -1;
  m_hTexture.Invalidate();
  m_hMaterial.Invalidate();
  m_hCachedCpuMesh.Invalidate();
}

void plRmlUiCanvas3DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  if (m_bClearStaleInput && m_iInputAge >= 0)
  {
    m_iInputAge += 1;
    if (m_iInputAge > 3)
    {
      m_InputProvider.Update(plRmlUiInputSnapshot::MakeEmpty());
      m_bNeedsUpdate |= m_pContext->UpdateInput(plVec2::MakeZero(), m_InputProvider);
      m_iInputAge = -1;
    }
  }

  m_bNeedsUpdate |= UpdateTextureAndMaterial();

  SUPER::Update();
}

bool plRmlUiCanvas3DComponent::ReceiveInput(const plVec2& vMousePosInsideCanvas, plRmlUiInputSnapshot input)
{
  if (IsInteractive() && SUPER::ReceiveInput(vMousePosInsideCanvas, input))
  {
    m_iInputAge = 0;
    return true;
  }
  return false;
}

bool plRmlUiCanvas3DComponent::RaycastInput(const plVec3& vRayOrigin, const plVec3& vRayDir, plRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr || !IsInteractive())
    return false;

  if (!m_hCachedCpuMesh.IsValid())
  {
    plMeshResourceHandle hMesh = m_hProxyMesh;

    if (m_hProxyMesh.IsValid())
    {
      hMesh = m_hProxyMesh;
    }
    else
    {
      plMeshComponent* pMeshComponent = nullptr;
      if (GetOwner()->TryGetComponentOfBaseType(pMeshComponent))
      {
        hMesh = pMeshComponent->GetMesh();
      }
    }

    if (!hMesh.IsValid())
    {
      plLog::Error("plRmlUiCanvas3DComponent '{0}' has no mesh to raycast against.", GetOwner()->GetName());
      SetInteractive(false); // deactivate, to prevent repeated errors
      return false;
    }

    m_hCachedCpuMesh = plResourceManager::LoadResource<plCpuMeshResource>(hMesh.GetResourceID());
  }

  plResourceLock<plCpuMeshResource> pMesh(m_hCachedCpuMesh, plResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == plResourceAcquireResult::LoadingFallback)
  {
    // not yet loaded, skip a frame
    return false;
  }

  if (pMesh.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Error("plRmlUiCanvas3DComponent '{0}' failed to get CPU mesh for raycast.", GetOwner()->GetName());
    SetInteractive(false); // deactivate, to prevent repeated errors
    return false;
  }

  const plTransform tOwner = GetOwner()->GetGlobalTransform();

  if (plMath::IsZero(tOwner.GetMaxScale(), 0.001f))
  {
    // object was scaled to zero
    return false;
  }

  const plTransform worldToLocal = tOwner.GetInverse();
  const plVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  const plVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  const plMeshResourceDescriptor& desc = pMesh->GetDescriptor();
  for (plUInt32 uiSubMeshIndex = 0; uiSubMeshIndex < desc.GetSubMeshes().GetCount(); ++uiSubMeshIndex)
  {
    const plMeshResourceDescriptor::SubMesh& submesh = desc.GetSubMeshes()[uiSubMeshIndex];
    if (submesh.m_uiMaterialIndex != m_uiMaterialIndex)
      continue;

    plVec2 vTexCoords;
    if (!RaycastMeshTexCoords(pMesh.GetPointer(), uiSubMeshIndex, vRayOriginMeshSpace, vRayDirMeshSpace, vTexCoords))
      continue;

    plVec2 vCursorPos;
    vCursorPos.x = static_cast<float>(m_vSize.x) * vTexCoords.x;
    vCursorPos.y = static_cast<float>(m_vSize.y) * vTexCoords.y;

    ReceiveInput(vCursorPos, input);

    return true;
  }

  return false;
}

bool plRmlUiCanvas3DComponent::RaycastMeshTexCoords(const plCpuMeshResource* pMesh, plUInt32 uiSubMeshIndex, const plVec3& vRayOrigin, const plVec3& vRayDir, plVec2& out_vTexCoords, float FEpsilon)
{
  const plMeshBufferResourceDescriptor& mesh = pMesh->GetDescriptor().MeshBufferDesc();

  if (mesh.GetTopology() != plGALPrimitiveTopology::Triangles)
  {
    plLog::Warning("Topology '{}' is not supported for raycasting.", mesh.GetTopology());
    return false;
  }

  const plUInt16* pIndexBuffer = reinterpret_cast<const plUInt16*>(mesh.GetIndexBufferData().GetPtr());
  plUInt32 uiNumIndices = mesh.GetIndexBufferData().GetCount() / 2;
  if (mesh.Uses32BitIndices())
  {
    plLog::Warning("Meshes with 32 bit indices are not supported for raycasting.");
    return false;
  }

  plMeshResourceDescriptor::SubMesh submesh = pMesh->GetDescriptor().GetSubMeshes()[uiSubMeshIndex];
  plUInt32 uiFirstIndex = submesh.m_uiFirstPrimitive * 3;
  plUInt32 uiLastIndex = uiFirstIndex + submesh.m_uiPrimitiveCount * 3;
  PL_ASSERT_DEV(uiLastIndex <= uiNumIndices, "something is wrong");

  float fClosestDist = 1e20f;
  plUInt16 uiClosestIndex0 = 0, uiClosestIndex1 = 0, uiClosestIndex2 = 0;
  plVec3 vClosestPos;

  for (plUInt32 i = uiFirstIndex; i + 2 < uiLastIndex; i += 3)
  {
    plUInt16 uiIndex0 = pIndexBuffer[i];
    plUInt16 uiIndex1 = pIndexBuffer[i + 1];
    plUInt16 uiIndex2 = pIndexBuffer[i + 2];

    plVec3 vVertex0 = mesh.GetPosition(uiIndex0);
    plVec3 vVertex1 = mesh.GetPosition(uiIndex1);
    plVec3 vVertex2 = mesh.GetPosition(uiIndex2);

    float fDist;
    plVec3 vPos;

    bool bHit = plIntersectionUtils::RayTriangleIntersectionCullBackface(vRayOrigin, vRayDir, vVertex0, vVertex1, vVertex2, vPos, &fDist, nullptr);
    if (!bHit || fDist > fClosestDist)
      continue;

    fClosestDist = fDist;
    uiClosestIndex0 = uiIndex0;
    uiClosestIndex1 = uiIndex1;
    uiClosestIndex2 = uiIndex2;
    vClosestPos = vPos;
  }

  if (fClosestDist < 1e20f)
  {
    out_vTexCoords = plVec2::MakeZero();
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex0) * vClosestPos.x;
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex1) * vClosestPos.y;
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex2) * vClosestPos.z;
    out_vTexCoords.x = plMath::Fraction(plMath::Abs(out_vTexCoords.x));
    out_vTexCoords.y = plMath::Fraction(plMath::Abs(out_vTexCoords.y));

    return true;
  }

  return false;
}

void plRmlUiCanvas3DComponent::SetBaseMaterial(const plMaterialResourceHandle& hMaterial)
{
  if (hMaterial == m_hBaseMaterial)
    return;

  m_hBaseMaterial = hMaterial;
  m_hMaterial.Invalidate();
}

void plRmlUiCanvas3DComponent::SetMaterialIndex(plUInt32 uiMaterialIndex)
{
  if (uiMaterialIndex == m_uiMaterialIndex)
    return;

  const plUInt32 uiPrevIndex = m_uiMaterialIndex;
  m_uiMaterialIndex = uiMaterialIndex;

  if (m_hMaterial.IsValid() && IsActiveAndInitialized())
  {
    plMsgSetMeshMaterial msg;

    msg.m_uiMaterialSlot = uiPrevIndex;
    GetOwner()->PostMessageRecursive(msg, plTime::MakeZero());

    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    msg.m_hMaterial = m_hMaterial;
    GetOwner()->PostMessageRecursive(msg, plTime::MakeZero());
  }
}

void plRmlUiCanvas3DComponent::SetTextureSlotName(plStringView sName)
{
  if (m_sTextureSlotName != sName)
  {
    m_hMaterial.Invalidate();
    m_sTextureSlotName.Assign(sName);
  }
}

void plRmlUiCanvas3DComponent::SetTextureSize(const plVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize.x = plMath::Min(vSize.x, 4096u);
    m_vSize.y = plMath::Min(vSize.y, 4096u);

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }

    // recreate the texture next update
    m_hTexture.Invalidate();
  }
}

void plRmlUiCanvas3DComponent::SetDpiScale(float fDpiScale)
{
  fDpiScale = fDpiScale > 0.0f ? fDpiScale : 1.0f;

  if (fDpiScale == m_fDpiScale)
    return;

  m_fDpiScale = fDpiScale;
  m_bNeedsUpdate = true;

  if (m_pContext != nullptr)
  {
    m_pContext->SetDpiScale(m_fDpiScale);
  }
}

void plRmlUiCanvas3DComponent::SetClearStaleInput(bool bClearStaleInput)
{
  m_bClearStaleInput = bClearStaleInput;
}

void plRmlUiCanvas3DComponent::SetInteractive(bool bIsInteractive)
{
  m_bIsInteractive = bIsInteractive;
}

void plRmlUiCanvas3DComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::MainView || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::EditorView)
  {
    if (m_pContext != nullptr && m_hTexture.IsValid())
    {
      plResourceLock<plTexture2DResource> pTexture(m_hTexture, plResourceAcquireMode::BlockTillLoaded);

      plRmlUi::GetSingleton()->ExtractContext(*m_pContext, pTexture->GetGALTexture());
    }
  }
}

void plRmlUiCanvas3DComponent::OnMsgReload(plMsgRmlUiReload& msg)
{
  SUPER::OnMsgReload(msg);

  m_hMaterial.Invalidate();
  m_hTexture.Invalidate();
}

bool plRmlUiCanvas3DComponent::UpdateTextureAndMaterial()
{
  if (m_vSize.x == 0 || m_vSize.y == 0 || !m_hBaseMaterial.IsValid() || m_sTextureSlotName.IsEmpty())
    return false;

  bool bNeedsUpdate = false;

  if (!m_hTexture.IsValid())
  {
    bNeedsUpdate = true;

    plTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = m_vSize.x;
    desc.m_DescGAL.m_uiHeight = m_vSize.y;
    desc.m_DescGAL.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    desc.m_DescGAL.m_ResourceAccess.m_bImmutable = false;
    desc.m_SamplerDesc.m_AddressU = plImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_AddressV = plImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_AddressW = plImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_BorderColor = plColor::MakeZero();
    desc.m_SamplerDesc.m_MinFilter = plGALTextureFilterMode::Anisotropic;
    desc.m_SamplerDesc.m_MagFilter = plGALTextureFilterMode::Anisotropic;
    desc.m_SamplerDesc.m_uiMaxAnisotropy = 8;

    if (plMath::IsPowerOf2(m_vSize.x) && plMath::IsPowerOf2(m_vSize.y))
    {
      desc.m_DescGAL.m_uiMipLevelCount = plMath::Max(plMath::Log2i(m_vSize.x), plMath::Log2i(m_vSize.y)) - 2;
      desc.m_DescGAL.m_bAllowDynamicMipGeneration = true;
    }

    plStringBuilder resourceName = "RmlUiCanvas3DComponent_Texture";
    resourceName.AppendFormat("_{0}", s_RmlResourceCounter.Increment());

    m_hTexture = plResourceManager::CreateResource<plTexture2DResource>(resourceName, std::move(desc));

    m_pContext->SetSize(m_vSize);
    m_pContext->SetDpiScale(m_fDpiScale);

    m_hMaterial.Invalidate();
  }

  if (!m_hMaterial.IsValid())
  {
    bNeedsUpdate = true;

    plMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = m_hBaseMaterial;
    auto& tb = desc.m_Texture2DBindings.ExpandAndGetRef();
    tb.m_Name = m_sTextureSlotName;
    tb.m_Value = m_hTexture;

    plStringBuilder resourceName = "RmlUiCanvas3DComponent_Material";
    resourceName.AppendFormat("_{0}", s_RmlResourceCounter.Increment());

    m_hMaterial = plResourceManager::CreateResource<plMaterialResource>(resourceName, std::move(desc));

    plMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    GetOwner()->PostMessageRecursive(msg, plTime::MakeZero());
  }

  return bNeedsUpdate;
}

PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
