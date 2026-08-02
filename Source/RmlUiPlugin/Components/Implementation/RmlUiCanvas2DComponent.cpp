#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRmlUiCanvas2DComponent, 5, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new plClampValueAttribute(plVec2(0), plVec2(1))),
    PL_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new plSuffixAttribute("px"), new plMinValueTextAttribute("Auto")),
    PL_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new plDefaultValueAttribute(plVec2::MakeZero()), new plSuffixAttribute("px")),
    PL_ACCESSOR_PROPERTY("CustomScale", GetCustomScale, SetCustomScale)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, 10.0f)),
    PL_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new plDefaultValueAttribute(true)),
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

plRmlUiCanvas2DComponent::plRmlUiCanvas2DComponent() = default;
plRmlUiCanvas2DComponent::~plRmlUiCanvas2DComponent() = default;
plRmlUiCanvas2DComponent& plRmlUiCanvas2DComponent::operator=(plRmlUiCanvas2DComponent&& rhs) = default;

void plRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  // Update once to ensure correct initial state
  Update();
}

void plRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  plGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);
}

void plRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  plVec2 viewSize = plVec2::MakeZero();
  m_bNeedsUpdate |= UpdateSizeOffsetAndTexture(viewSize);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    plVec2 mousePos;
    plInputManager::GetInputSlotState(plInputSlot_MousePositionX, &mousePos.x);
    plInputManager::GetInputSlotState(plInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - m_vFinalOffset;
    ReceiveInput(mousePos, plRmlUiInputSnapshot::MakeFromCurrentInput());
  }

  SUPER::Update();
}

void plRmlUiCanvas2DComponent::SetOffset(const plVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void plRmlUiCanvas2DComponent::SetSize(const plVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void plRmlUiCanvas2DComponent::SetAnchorPoint(const plVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
}

void plRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void plRmlUiCanvas2DComponent::SetCustomScale(float fScale)
{
  m_fCustomScale = fScale;
}

void plRmlUiCanvas2DComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_fCustomScale;
}

void plRmlUiCanvas2DComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion >= 5)
  {
    SUPER::DeserializeComponent(inout_stream);
    plStreamReader& s = inout_stream.GetStream();

    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;
    s >> m_fCustomScale;
  }
  else
  {
    plRenderComponent::DeserializeComponent(inout_stream);
    plStreamReader& s = inout_stream.GetStream();

    s >> m_hResource;
    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;

    if (uiVersion >= 2)
    {
      s >> m_bAutobindBlackboards;
    }

    if (uiVersion >= 3)
    {
      s >> m_bOnDemandUpdate;
    }

    if (uiVersion >= 4)
    {
      s >> m_fCustomScale;
    }
  }
}

void plRmlUiCanvas2DComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::Thumbnail)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr && m_hTexture.IsInvalidated() == false)
  {
    plRmlUi::GetSingleton()->ExtractContext(*m_pContext, m_hTexture);

    auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<plRmlUiRenderData>(GetOwner());
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_vOffset = m_vFinalOffset;

    msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::GUI, plRenderData::Caching::Never);
  }
}

bool plRmlUiCanvas2DComponent::UpdateSizeOffsetAndTexture(plVec2& out_viewSize)
{
  out_viewSize = plVec2(1.0f);
  if (plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, GetWorld()))
  {
    out_viewSize.x = pView->GetViewport().width;
    out_viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = out_viewSize.y / m_vReferenceResolution.y;
  }

  plVec2 size = plVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = out_viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = out_viewSize.y;
  }
  const plVec2U32 sizeU32 = plVec2U32(static_cast<plUInt32>(size.x), static_cast<plUInt32>(size.y));
  if (sizeU32.IsZero())
    return false;

  m_pContext->SetSize(sizeU32);
  m_pContext->SetDpiScale(fScale * m_fCustomScale);

  plVec2 offset = plVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset = (out_viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - plVec2(1.0f));
  m_vFinalOffset.x = plMath::Round(offset.x);
  m_vFinalOffset.y = plMath::Round(offset.y);

  // Recreate texture if necessary
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  const plGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != sizeU32.x || pTexture->GetDescription().m_uiHeight != sizeU32.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    plGALTextureCreationDescription desc;
    desc.m_uiWidth = sizeU32.x;
    desc.m_uiHeight = sizeU32.y;
    desc.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTexture = pDevice->CreateTexture(desc);

    return true;
  }

  return false;
}


PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
