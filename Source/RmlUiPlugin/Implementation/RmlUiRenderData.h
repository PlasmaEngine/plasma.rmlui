#pragma once

#include <RendererCore/Pipeline/RenderData.h>

class plRmlUiRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiRenderData, plRenderData);

public:
  plGALTextureHandle m_hTexture;
  plVec2 m_vOffset = plVec2::MakeZero();
};
