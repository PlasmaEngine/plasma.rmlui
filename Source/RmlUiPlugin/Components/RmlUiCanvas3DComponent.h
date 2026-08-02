#pragma once

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using plCpuMeshResourceHandle = plTypedResourceHandle<class plCpuMeshResource>;

using plRmlUiCanvas3DComponentManager = plComponentManagerSimple<class plRmlUiCanvas3DComponent, plComponentUpdateType::Always, plBlockStorageType::Compact>;

class PL_RMLUIPLUGIN_DLL plRmlUiCanvas3DComponent : public plRmlUiCanvasComponentBase
{
  PL_DECLARE_COMPONENT_TYPE(plRmlUiCanvas3DComponent, plRmlUiCanvasComponentBase, plRmlUiCanvas3DComponentManager);

public:
  plRmlUiCanvas3DComponent();
  ~plRmlUiCanvas3DComponent();

  plRmlUiCanvas3DComponent& operator=(plRmlUiCanvas3DComponent&& rhs);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update() final override;

  bool ReceiveInput(const plVec2& vMousePosInsideCanvas, plRmlUiInputSnapshot input) override;
  bool RaycastInput(const plVec3& vRayOrigin, const plVec3& vRayDir, plRmlUiInputSnapshot input);

  /// \brief Changes which mesh will be used for hit testing.
  void SetProxyMesh(const plMeshResourceHandle& hMesh) { m_hProxyMesh = hMesh; } // [ property ]
  const plMeshResourceHandle& GetProxyMesh() const { return m_hProxyMesh; }      // [ property ]

  PL_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(ProxyMesh, m_hProxyMesh, SetProxyMesh);

  void SetBaseMaterial(const plMaterialResourceHandle& hMaterial);                    // [ property ]
  const plMaterialResourceHandle& GetBaseMaterial() const { return m_hBaseMaterial; } // [ property ]

  PL_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(BaseMaterial, m_hBaseMaterial, SetBaseMaterial);

  void SetMaterialIndex(plUInt32 uiMaterialIndex);                       // [ property ]
  plUInt32 GetMaterialIndex() const { return m_uiMaterialIndex; }        // [ property ]

  void SetTextureSlotName(plStringView sName);                           // [ property ]
  plStringView GetTextureSlotName() const { return m_sTextureSlotName; } // [ property ]

  void SetTextureSize(const plVec2U32& vSize);                           // [ property ]
  const plVec2U32& GetTextureSize() const { return m_vSize; }            // [ property ]

  void SetDpiScale(float fDpiScale);                                     // [ property ]
  float GetDpiScale() const { return m_fDpiScale; }                      // [ property ]

  void SetClearStaleInput(bool bClearStaleInput);                        // [ property ]
  bool GetClearStaleInput() const { return m_bClearStaleInput; }         // [ property ]

  void SetInteractive(bool bIsInteractive);                              // [ property ]
  bool IsInteractive() const { return m_bIsInteractive; }                // [ property ]

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const override; // [ msg handler ]
  virtual void OnMsgReload(plMsgRmlUiReload& msg) override;                        // [ msg handler ]

  bool UpdateTextureAndMaterial();

  static bool RaycastMeshTexCoords(const class plCpuMeshResource* pMesh, plUInt32 uiSubMeshIndex, const plVec3& vRayOrigin, const plVec3& vRayDir, plVec2& out_vTexCoords, float fEpsilon = 0.00001f);

  // properties
  plMeshResourceHandle m_hProxyMesh;
  plMaterialResourceHandle m_hBaseMaterial;
  plUInt32 m_uiMaterialIndex = 0;
  plHashedString m_sTextureSlotName;
  float m_fDpiScale = 1.0f;
  bool m_bClearStaleInput = true;
  bool m_bIsInteractive = true;

  // runtime data
  plInt8 m_iInputAge = -1;
  plCpuMeshResourceHandle m_hCachedCpuMesh;
  plMaterialResourceHandle m_hMaterial;
  plTexture2DResourceHandle m_hTexture;
};
