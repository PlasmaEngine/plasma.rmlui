#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiBlitConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiRenderer, 1, plRTTIDefaultAllocator<plRmlUiRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiRenderer::plRmlUiRenderer()
{
  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/RmlUiBlit.plShader");
  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plRmlUiBlitConstants>();
}

plRmlUiRenderer::~plRmlUiRenderer()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void plRmlUiRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plRmlUiRenderData>());
}

void plRmlUiRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::GUI);
}

void plRmlUiRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  plBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
  bindGroup.BindBuffer("plRmlUiBlitConstants", m_hConstantBuffer);
  pRenderContext->BindNullMeshBuffer(plGALPrimitiveTopology::TriangleStrip, 2);

  const plVec2 targetSize = plVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
  const plVec2 scale = plVec2(2.0f, -2.0f).CompDiv(targetSize);
  const plVec2 offset = plVec2(-1.0f, 1.0f);

  for (auto it = batch.GetIterator<plRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const plRmlUiRenderData* pRenderData = it;
    const plGALTexture* pTexture = pDevice->GetTexture(pRenderData->m_hTexture);

    const plVec2 targetSize = plVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
    const plVec2 textureSize = plVec2(static_cast<float>(pTexture->GetDescription().m_uiWidth), static_cast<float>(pTexture->GetDescription().m_uiHeight));

    plRmlUiBlitConstants* pConstants = pRenderContext->GetConstantBufferData<plRmlUiBlitConstants>(m_hConstantBuffer);
    pConstants->Scale = textureSize.CompMul(scale);
    pConstants->Offset = pRenderData->m_vOffset.CompMul(scale) + offset;

    bindGroup.BindTexture("BaseTexture", pRenderData->m_hTexture);

    pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiRenderer);
