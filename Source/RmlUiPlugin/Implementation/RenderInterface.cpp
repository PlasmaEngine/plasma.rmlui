

#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>

#include <Shaders/RmlUiConstants.h>

namespace plRmlUiInternal
{
  struct Vertex
  {
    PL_DECLARE_POD_TYPE();

    plVec2 m_Position;
    plVec2 m_TexCoord;
    plColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    plUInt32 m_uiTriangleCount = 0;
    plGALBufferHandle m_hVertexBuffer;
    plGALBufferHandle m_hIndexBuffer;
  };

  struct CommandType
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Invalid,
      RenderGeometry,
      SetScissorRegion,
      RenderToClipMask,

      Default = Invalid
    };
  };

  struct alignas(4) CommandHeader
  {
    plEnum<CommandType> m_Type;
    bool m_bNeedsPremultipliedAlpha = false;
    bool m_bUseStencilTest = false;
    plUInt8 m_uiPadding = 0;
  };

  struct CommandRenderGeometry : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderGeometry;

    CompiledGeometry m_CompiledGeometry;
    plGALTextureHandle m_hTexture;
    plMat4 m_Transform = plMat4::MakeIdentity();
    plVec2 m_Translation = plVec2::MakeZero();
  };

  struct CommandSetScissorRegion : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::SetScissorRegion;

    plRectU32 m_ScissorRect = {};
  };

  struct CommandRenderToClipMask : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderToClipMask;

    Rml::ClipMaskOperation m_Operation;
    CompiledGeometry m_CompiledGeometry;
    plMat4 m_Transform = plMat4::MakeIdentity();
    plVec2 m_Translation = plVec2::MakeZero();
  };

  struct CommandBuffer
  {
    plHashedString m_sName;
    plDynamicArray<plUInt8> m_Buffer;
    plGALTextureHandle m_hTargetTexture;
    plUInt32 m_uiTargetWidth = 0;
    plUInt32 m_uiTargetHeight = 0;

    template <typename T>
    T& AddCommand()
    {
      T& cmd = *reinterpret_cast<T*>(m_Buffer.ExpandBy(sizeof(T)));
      cmd.m_Type = T::Type;
      return cmd;
    }

    template <typename T>
    const T& ConsumeCommand(plUInt32& inout_uiOffset) const
    {
      const T& cmd = *reinterpret_cast<const T*>(m_Buffer.GetData() + inout_uiOffset);
      inout_uiOffset += sizeof(T);
      return cmd;
    }

    CommandType::Enum PeekCommandType(plUInt32 uiOffset) const
    {
      return static_cast<CommandType::Enum>(*(m_Buffer.GetData() + uiOffset));
    }

    void Clear()
    {
      m_Buffer.Clear();
    }
  };

  //////////////////////////////////////////////////////////////////////////

  RenderInterface::RenderInterface()
  {
    plGALDevice::s_Events.AddEventHandler(plMakeDelegate(&RenderInterface::GALEventHandler, this));

    m_hFallbackTexture = plResourceManager::LoadResource<plTexture2DResource>("White.color");
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/RmlUi.plShader");
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plRmlUiConstants>();

    // Setup the vertex declaration
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = plGALVertexAttributeSemantic::Position;
      va.m_eFormat = plGALResourceFormat::XYFloat;
      va.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_Position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = plGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = plGALResourceFormat::UVFloat;
      va.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_TexCoord);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = plGALVertexAttributeSemantic::Color0;
      va.m_eFormat = plGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_Color);
    }
  }

  RenderInterface::~RenderInterface()
  {
    plGALDevice::s_Events.RemoveEventHandler(plMakeDelegate(&RenderInterface::GALEventHandler, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }

    m_Textures.Clear();
    m_hFallbackTexture.Invalidate();
  }

  Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
  {
    const plUInt32 uiNumVertices = static_cast<plUInt32>(vertices.size());
    const plUInt32 uiNumIndices = static_cast<plUInt32>(indices.size());

    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = uiNumIndices / 3;

    // vertices
    {
      plDynamicArray<Vertex> vertexStorage(plFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(uiNumVertices);

      for (plUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = vertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = plVec2(srcVertex.position.x, srcVertex.position.y);
        destVertex.m_TexCoord = plVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color = reinterpret_cast<const plColorLinearUB&>(srcVertex.colour);
      }

      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferFlags = plGALBufferUsageFlags::VertexBuffer;

      geometry.m_hVertexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(plUInt32);
      desc.m_uiTotalSize = uiNumIndices * desc.m_uiStructSize;
      desc.m_BufferFlags = plGALBufferUsageFlags::IndexBuffer;

      geometry.m_hIndexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc, plMakeArrayPtr(indices.data(), uiNumIndices).ToByteArray());
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void RenderInterface::RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderGeometry>();
    PL_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry), cmd.m_CompiledGeometry), "Invalid compiled geometry");

    TextureInfo textureInfo;
    if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), textureInfo) == false)
    {
      textureInfo.m_hTexture = m_hFallbackTexture;
    }

    plResourceLock<plTexture2DResource> pTexture(textureInfo.m_hTexture, plResourceAcquireMode::BlockTillLoaded);
    cmd.m_hTexture = pTexture->GetGALTexture();

    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = plVec2(translation.x, translation.y);
    cmd.m_bNeedsPremultipliedAlpha = textureInfo.m_bHasPremultipliedAlpha == false;
    cmd.m_bUseStencilTest = m_bUseStencilTest;
  }

  void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry)
  {
    PL_LOCK(m_ReleasedCompiledGeometryMutex);

    m_ReleasedCompiledGeometry.PushBack({plRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry)});
  }

  Rml::TextureHandle RenderInterface::LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource)
  {
    plTexture2DResourceHandle hTexture = plResourceManager::LoadResource<plTexture2DResource>(plRmlUiConversionUtils::ToStringView(sSource));

    plResourceLock<plTexture2DResource> pTexture(hTexture, plResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      out_textureSize = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      TextureInfo textureInfo;
      textureInfo.m_hTexture = hTexture;
      textureInfo.m_bHasPremultipliedAlpha = false;

      return m_Textures.Insert(textureInfo).ToRml();
    }

    return plRmlUiInternal::TextureId().ToRml();
  }

  Rml::TextureHandle RenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize)
  {
    plUInt32 uiWidth = sourceSize.x;
    plUInt32 uiHeight = sourceSize.y;
    plUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;
    PL_ASSERT_DEV(uiSizeInBytes == source.size(), "Invalid source size");

    plUInt64 uiHash = plHashingUtils::xxHash64(source.data(), uiSizeInBytes);

    plStringBuilder sTextureName;
    sTextureName.SetFormat("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    plTexture2DResourceHandle hTexture = plResourceManager::GetExistingResource<plTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      plGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = plMakeByteBlobPtr(source.data(), uiSizeInBytes);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      plTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent = plMakeArrayPtr(&memoryDesc, 1);

      hTexture = plResourceManager::GetOrCreateResource<plTexture2DResource>(sTextureName, std::move(desc));
    }

    TextureInfo textureInfo;
    textureInfo.m_hTexture = hTexture;
    textureInfo.m_bHasPremultipliedAlpha = true;

    return m_Textures.Insert(textureInfo).ToRml();
  }

  void RenderInterface::ReleaseTexture(Rml::TextureHandle hTexture)
  {
    TextureId textureId = TextureId::FromRml(hTexture);
    if (textureId.IsInvalidated() == false)
    {
      PL_VERIFY(m_Textures.Remove(textureId), "Invalid texture handle");
    }
  }

  void RenderInterface::EnableScissorRegion(bool bEnable)
  {
    // Scissor is always enabled in our shaders, set the full viewport if disabled
    if (bEnable == false)
    {
      SetScissorRegion(Rml::Rectanglei::FromSize({(int)m_pCurrentCommandBuffer->m_uiTargetWidth, (int)m_pCurrentCommandBuffer->m_uiTargetHeight}));
    }
  }

  void RenderInterface::SetScissorRegion(Rml::Rectanglei region)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandSetScissorRegion>();
    cmd.m_ScissorRect = plRectU32(region.Left(), region.Top(), region.Width(), region.Height());
  }

  void RenderInterface::EnableClipMask(bool bEnable)
  {
    m_bUseStencilTest = bEnable;
  }

  void RenderInterface::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderToClipMask>();
    cmd.m_Operation = operation;
    cmd.m_CompiledGeometry = m_CompiledGeometry[GeometryId::FromRml(hGeometry)];
    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = plVec2(translation.x, translation.y);
  }

  void RenderInterface::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr bool bColumnMajor = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value;

      if (bColumnMajor)
        m_mTransform = plMat4::MakeFromColumnMajorArray(pTransform->data());
      else
        m_mTransform = plMat4::MakeFromRowMajorArray(pTransform->data());
    }
    else
    {
      m_mTransform.SetIdentity();
    }
  }

  void RenderInterface::BeginExtraction(const plHashedString& sName, plGALTextureHandle hTargetTexture)
  {
    m_mTransform = plMat4::MakeIdentity();
    m_bUseStencilTest = false;

    m_pCurrentCommandBuffer = AllocateCommandBuffer();
    m_pCurrentCommandBuffer->m_sName = sName;
    m_pCurrentCommandBuffer->m_hTargetTexture = hTargetTexture;

    const plGALTexture* pTargetTexture = plGALDevice::GetDefaultDevice()->GetTexture(hTargetTexture);
    m_pCurrentCommandBuffer->m_uiTargetWidth = pTargetTexture->GetDescription().m_uiWidth;
    m_pCurrentCommandBuffer->m_uiTargetHeight = pTargetTexture->GetDescription().m_uiHeight;
  }

  void RenderInterface::EndExtraction()
  {
    SubmitCommandBuffer(std::move(m_pCurrentCommandBuffer));
  }

  void RenderInterface::GALEventHandler(const plGALDeviceEvent& e)
  {
    if (e.m_Type == plGALDeviceEvent::AfterBeginFrame)
    {
      BeginFrame();
    }
    else if (e.m_Type == plGALDeviceEvent::BeforeEndFrame)
    {
      EndFrame();
    }
  }

  void RenderInterface::BeginFrame()
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[plRenderWorld::GetDataIndexForRendering()];
    if (submittedCommandBuffers.IsEmpty())
      return;

    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    plGALCommandEncoder* pCommandEncoder = pDevice->BeginCommands("RmlUi");

    plRenderContext* pRenderContext = plRenderContext::GetDefaultInstance();
    plGPUResourcePool* pGpuResourcePool = plGPUResourcePool::GetDefaultInstance();

    // Force shader loading, otherwise we could end up rendering nothing when lazy update is enabled on the context
    bool bAllowAsyncShaderLoading = pRenderContext->GetAllowAsyncShaderLoading();
    pRenderContext->SetAllowAsyncShaderLoading(false);
    PL_SCOPE_EXIT(pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

    const plGALResourceFormat::Enum tempTargetFormat = plGALResourceFormat::RGBAUByteNormalized;
    const plGALResourceFormat::Enum tempStencilFormat = plGALResourceFormat::D24S8;
    const plGALMSAASampleCount::Enum msaaSampleCount = plGALMSAASampleCount::FourSamples;

    pRenderContext->BindShader(m_hShader);
    plBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("plRmlUiConstants", m_hConstantBuffer);

    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      const plGALTexture* pTargetTexture = pDevice->GetTexture(pCommandBuffer->m_hTargetTexture);
      if (pTargetTexture == nullptr)
        continue;

      auto& textureDesc = pTargetTexture->GetDescription();
      plRectFloat viewport(static_cast<float>(textureDesc.m_uiWidth), static_cast<float>(textureDesc.m_uiHeight));
      plRectU32 scissorRect(0, 0, textureDesc.m_uiWidth, textureDesc.m_uiHeight);

      plGALTextureHandle hTempTarget = pGpuResourcePool->GetRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempTargetFormat, msaaSampleCount);
      plGALTextureHandle hTempStencil = pGpuResourcePool->GetRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempStencilFormat, msaaSampleCount);

      plGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(hTempTarget));
      renderingSetup.SetClearColor(0);
      renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hTempStencil), plGALRenderTargetLoadOp::DontCare, plGALRenderTargetStoreOp::Discard);
      renderingSetup.SetClearStencil().SetClearDepth();

      pRenderContext->BeginRendering(renderingSetup, viewport, pCommandBuffer->m_sName, false);
      pCommandEncoder->SetScissorRect(scissorRect);

      plUInt32 uiCommandOffset = 0;
      while (uiCommandOffset < pCommandBuffer->m_Buffer.GetCount())
      {
        CommandType::Enum cmdType = pCommandBuffer->PeekCommandType(uiCommandOffset);
        switch (cmdType)
        {
          case CommandType::RenderGeometry:
          {
            auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderGeometry>(uiCommandOffset);

            pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", cmd.m_bUseStencilTest ? plTempHashedString("RMLUI_MODE_STENCIL_TEST") : plTempHashedString("RMLUI_MODE_NORMAL"));

            plRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<plRmlUiConstants>(m_hConstantBuffer);
            pConstants->UiTransform = cmd.m_Transform;
            pConstants->UiTranslation = cmd.m_Translation.GetAsVec4(0, 1);
            pConstants->TextureNeedsAlphaMultiplication = cmd.m_bNeedsPremultipliedAlpha;

            pRenderContext->BindMeshBuffer(plMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, plGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

            bindGroup.BindTexture("BaseTexture", cmd.m_hTexture);

            pRenderContext->DrawMeshBuffer().IgnoreResult();
          }
          break;

          case CommandType::SetScissorRegion:
          {
            auto& cmd = pCommandBuffer->ConsumeCommand<CommandSetScissorRegion>(uiCommandOffset);

            pCommandEncoder->SetScissorRect(cmd.m_ScissorRect);
          }
          break;

          case CommandType::RenderToClipMask:
          {
            auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderToClipMask>(uiCommandOffset);

            PL_ASSERT_DEV(cmd.m_Operation == Rml::ClipMaskOperation::Set, "Only 'Set' clip mask operation is implemented.");
            pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

            plRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<plRmlUiConstants>(m_hConstantBuffer);
            pConstants->UiTransform = cmd.m_Transform;
            pConstants->UiTranslation = cmd.m_Translation.GetAsVec4(0, 1);

            pRenderContext->BindMeshBuffer(plMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, plGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

            pRenderContext->DrawMeshBuffer().IgnoreResult();
          }
          break;

          default:
          {
            PL_ASSERT_ALWAYS(false, "RmlUI: Command Type '{}' is not implemented.", cmdType);
            break;
          }
        }
      }

      pRenderContext->EndRendering();

      pCommandEncoder->ResolveTexture(pCommandBuffer->m_hTargetTexture, plGALTextureSubresource(), hTempTarget, plGALTextureSubresource());
      if (textureDesc.m_uiMipLevelCount > 1 && textureDesc.m_bAllowDynamicMipGeneration)
      {
        pCommandEncoder->GenerateMipMaps(pCommandBuffer->m_hTargetTexture, plGALTextureRange::MakeFromMipRange());
      }

      pGpuResourcePool->ReturnRenderTarget(hTempTarget);
      pGpuResourcePool->ReturnRenderTarget(hTempStencil);

      FreeCommandBuffer(std::move(pCommandBuffer));
    }

    pDevice->EndCommands(pCommandEncoder);

    submittedCommandBuffers.Clear();
  }

  void RenderInterface::EndFrame()
  {
    plUInt64 uiFrameCounter = plRenderWorld::GetFrameCounter();

    PL_LOCK(m_ReleasedCompiledGeometryMutex);

    while (!m_ReleasedCompiledGeometry.IsEmpty())
    {
      auto& releasedGeometry = m_ReleasedCompiledGeometry.PeekFront();

      if (releasedGeometry.m_uiFrame >= uiFrameCounter)
        break;

      FreeReleasedGeometry(releasedGeometry.m_Id);

      m_CompiledGeometry.Remove(releasedGeometry.m_Id);
      m_ReleasedCompiledGeometry.PopFront();
    }
  }

  void RenderInterface::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    plGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    plGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();
  }

  plUniquePtr<CommandBuffer> RenderInterface::AllocateCommandBuffer()
  {
    if (m_FreeCommandBuffers.IsEmpty() == false)
    {
      plUniquePtr<CommandBuffer> cmdBuffer = std::move(m_FreeCommandBuffers.PeekBack());
      m_FreeCommandBuffers.PopBack();
      return cmdBuffer;
    }

    return PL_DEFAULT_NEW(CommandBuffer);
  }

  void RenderInterface::FreeCommandBuffer(plUniquePtr<CommandBuffer>&& pBuffer)
  {
    pBuffer->Clear();

    m_FreeCommandBuffers.PushBack(std::move(pBuffer));
  }

  void RenderInterface::SubmitCommandBuffer(plUniquePtr<CommandBuffer>&& pBuffer)
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[plRenderWorld::GetDataIndexForExtraction()];
    submittedCommandBuffers.PushBack(std::move(pBuffer));
  }

} // namespace plRmlUiInternal
