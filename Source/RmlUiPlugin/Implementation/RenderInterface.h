#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/IdTable.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

namespace plRmlUiInternal
{
  struct GeometryId : public plGenericId<24, 8>
  {
    using plGenericId::plGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle hGeometry) { return GeometryId(static_cast<plUInt32>(hGeometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public plGenericId<24, 8>
  {
    using plGenericId::plGenericId;

    static TextureId FromRml(Rml::TextureHandle hTexture) { return TextureId(static_cast<plUInt32>(hTexture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  struct CompiledGeometry;
  struct CommandBuffer;

  class RenderInterface final : public Rml::RenderInterface
  {
  public:
    RenderInterface();
    virtual ~RenderInterface();

    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    virtual void RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry) override;

    virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource) override;
    virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize) override;
    virtual void ReleaseTexture(Rml::TextureHandle hTexture) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(Rml::Rectanglei region) override;

    virtual void EnableClipMask(bool bEnable) override;
    virtual void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation) override;

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    void BeginExtraction(const plHashedString& sName, plGALTextureHandle hTargetTexture);
    void EndExtraction();

  private:
    void GALEventHandler(const plGALDeviceEvent& e);
    void BeginFrame();
    void EndFrame();
    void FreeReleasedGeometry(GeometryId id);

    plUniquePtr<CommandBuffer> AllocateCommandBuffer();
    void FreeCommandBuffer(plUniquePtr<CommandBuffer>&& pBuffer);
    void SubmitCommandBuffer(plUniquePtr<CommandBuffer>&& pBuffer);

    plIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      plUInt64 m_uiFrame;
      GeometryId m_Id;
    };

    plMutex m_ReleasedCompiledGeometryMutex;
    plDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    struct TextureInfo
    {
      plTexture2DResourceHandle m_hTexture;
      bool m_bHasPremultipliedAlpha = false;
    };

    plIdTable<TextureId, TextureInfo> m_Textures;
    plTexture2DResourceHandle m_hFallbackTexture;

    plMat4 m_mTransform = plMat4::MakeIdentity();
    bool m_bUseStencilTest = false;

    plDynamicArray<plUniquePtr<CommandBuffer>> m_FreeCommandBuffers;
    plDynamicArray<plUniquePtr<CommandBuffer>> m_SubmittedCommandBuffers[2];

    plUniquePtr<CommandBuffer> m_pCurrentCommandBuffer;

    plShaderResourceHandle m_hShader;
    plConstantBufferStorageHandle m_hConstantBuffer;
    plSmallArray<plGALVertexAttribute, 3> m_VertexAttributes;
  };
} // namespace plRmlUiInternal
