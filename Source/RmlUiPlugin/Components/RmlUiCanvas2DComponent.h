#pragma once

#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using plRmlUiCanvas2DComponentManager = plComponentManagerSimple<class plRmlUiCanvas2DComponent, plComponentUpdateType::Always, plBlockStorageType::Compact>;

class PL_RMLUIPLUGIN_DLL plRmlUiCanvas2DComponent : public plRmlUiCanvasComponentBase
{
  PL_DECLARE_COMPONENT_TYPE(plRmlUiCanvas2DComponent, plRmlUiCanvasComponentBase, plRmlUiCanvas2DComponentManager);

public:
  plRmlUiCanvas2DComponent();
  ~plRmlUiCanvas2DComponent();

  plRmlUiCanvas2DComponent& operator=(plRmlUiCanvas2DComponent&& rhs);

  virtual void Deinitialize() override;
  virtual void OnActivated() override;

  void Update() final override;

  void SetOffset(const plVec2I32& vOffset);                                   // [ property ]
  const plVec2I32& GetOffset() const { return m_vOffset; }                    // [ property ]

  void SetSize(const plVec2U32& vSize);                                       // [ property ]
  const plVec2U32& GetSize() const { return m_vSize; }                        // [ property ]

  void SetAnchorPoint(const plVec2& vAnchorPoint);                            // [ property ]
  const plVec2& GetAnchorPoint() const { return m_vAnchorPoint; }             // [ property ]

  void SetPassInput(bool bPassInput);                                         // [ property ]
  bool GetPassInput() const { return m_bPassInput; }                          // [ property ]

  void SetCustomScale(float fScale);                                          // [ property ]
  float GetCustomScale() const { return m_fCustomScale; }                     // [ property ]

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const override;
  bool UpdateSizeOffsetAndTexture(plVec2& out_viewSize);

  float m_fCustomScale = 1.0f;
  plVec2I32 m_vOffset = plVec2I32::MakeZero();
  plVec2 m_vAnchorPoint = plVec2::MakeZero();
  bool m_bPassInput = true;

  plVec2 m_vFinalOffset = plVec2::MakeZero();
  plGALTextureHandle m_hTexture;
};
