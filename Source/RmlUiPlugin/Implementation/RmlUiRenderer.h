#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

class PL_RMLUIPLUGIN_DLL plRmlUiRenderer : public plRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiRenderer, plRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plRmlUiRenderer);

public:
  plRmlUiRenderer();
  ~plRmlUiRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

private:
  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;
};
