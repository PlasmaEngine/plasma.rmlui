#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plRmlUiCanvasComponentBase, 2)
{
  PL_BEGIN_PROPERTIES
  {
    PL_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    PL_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards),
    PL_ACCESSOR_PROPERTY("SendEventMessage", GetSendEventMessage, SetSendEventMessage),
    PL_ACCESSOR_PROPERTY("OnDemandUpdate", GetOnDemandUpdate, SetOnDemandUpdate)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgRmlUiReload, OnMsgReload)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input/RmlUi"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

static plAtomicInteger32 s_RmlContextIdCounter;

plRmlUiCanvasComponentBase::plRmlUiCanvasComponentBase() = default;
plRmlUiCanvasComponentBase::~plRmlUiCanvasComponentBase() = default;
plRmlUiCanvasComponentBase& plRmlUiCanvasComponentBase::operator=(plRmlUiCanvasComponentBase&& rhs) = default;

void plRmlUiCanvasComponentBase::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_bAutobindBlackboards;
  s << m_bSendEventMessage;
  s << m_bOnDemandUpdate;
}

void plRmlUiCanvasComponentBase::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_bAutobindBlackboards;

  if (uiVersion >= 2)
  {
    s >> m_bSendEventMessage;
  }

  s >> m_bOnDemandUpdate;
}

void plRmlUiCanvasComponentBase::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void plRmlUiCanvasComponentBase::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_pContext != nullptr)
  {
    plRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void plRmlUiCanvasComponentBase::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void plRmlUiCanvasComponentBase::Update()
{
  if (m_pContext == nullptr)
    return;

  const plTime tDiff = plClock::GetGlobalClock()->GetTimeDiff();
  m_bNeedsUpdate |= m_pContext->GetNextUpdateDelay() < plMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      m_bNeedsUpdate |= pDataBinding->Update();
    }
  }

  if (m_bNeedsUpdate || m_bOnDemandUpdate == false)
  {
    m_pContext->Update();

    m_bNeedsUpdate = false;
  }
}

bool plRmlUiCanvasComponentBase::ReceiveInput(const plVec2& vMousePosInsideCanvas, plRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr)
    return false;

  m_InputProvider.Update(input);
  m_bNeedsUpdate |= m_pContext->UpdateInput(vMousePosInsideCanvas, m_InputProvider);

  return true;
}

void plRmlUiCanvasComponentBase::SetRmlResource(const plRmlUiResourceHandle& hResource)
{
  if (m_hResource != hResource)
  {
    m_hResource = hResource;

    if (m_pContext != nullptr)
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }

      UpdateCachedValues();
    }
  }
}

void plRmlUiCanvasComponentBase::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

void plRmlUiCanvasComponentBase::SetSendEventMessage(bool bSendEventMessage)
{
  if (m_bSendEventMessage != bSendEventMessage)
  {
    m_bSendEventMessage = bSendEventMessage;

    UpdateEventHandler();
  }
}

void plRmlUiCanvasComponentBase::SetOnDemandUpdate(bool bOnDemandUpdate)
{
  m_bOnDemandUpdate = bOnDemandUpdate;
  m_bNeedsUpdate = true;
}

plUInt32 plRmlUiCanvasComponentBase::AddDataBinding(plUniquePtr<plRmlUiDataBinding>&& pDataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (pDataBinding->Initialize(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  for (plUInt32 i = 0; i < m_DataBindings.GetCount(); ++i)
  {
    if (pDataBinding == nullptr)
    {
      m_DataBindings[i] = std::move(pDataBinding);
      return i;
    }
  }

  plUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(pDataBinding));
  return uiDataBindingIndex;
}

void plRmlUiCanvasComponentBase::RemoveDataBinding(plUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

plUInt32 plRmlUiCanvasComponentBase::AddBlackboardBinding(const plSharedPtr<plBlackboard>& pBlackboard)
{
  auto pDataBinding = PL_DEFAULT_NEW(plRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void plRmlUiCanvasComponentBase::RemoveBlackboardBinding(plUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

plResult plRmlUiCanvasComponentBase::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PL_SUCCESS;
}

plRmlUiContext* plRmlUiCanvasComponentBase::GetOrCreateRmlContext()
{
  if (m_pContext != nullptr)
  {
    return m_pContext;
  }

  plStringBuilder sName = "RmlUi_";
  if (m_hResource.IsValid())
  {
    plStringView sResourceID = m_hResource.GetResourceIdOrDescription();
    sName.Append(sResourceID.GetFileName());
  }

  if (m_uiContextID == 0)
  {
    m_uiContextID = s_RmlContextIdCounter.Increment();
  }

  sName.AppendFormat("_{}", m_uiContextID);

  m_pContext = plRmlUi::GetSingleton()->CreateContext(sName, m_vSize);
  PL_ASSERT_DEV(m_pContext != nullptr, "RML UI context creation failed");

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();
  UpdateEventHandler();

  return m_pContext;
}

void plRmlUiCanvasComponentBase::OnMsgReload(plMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

void plRmlUiCanvasComponentBase::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    plResourceLock pResource(m_hResource, plResourceAcquireMode::BlockTillLoaded);

    if (pResource->GetScaleMode() == plRmlUiScaleMode::WithScreenSize)
    {
      m_vReferenceResolution = pResource->GetReferenceResolution();
    }

    pResource->m_ResourceEvents.AddEventHandler(
      [hComponent = GetHandle(), pWorld = GetWorld()](const plResourceEvent& e)
      {
        if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading)
        {
          pWorld->PostMessage(hComponent, plMsgRmlUiReload(), plTime::MakeZero());
        }
      },
      m_ResourceEventUnsubscriber);
  }
}

void plRmlUiCanvasComponentBase::UpdateAutobinding()
{
  for (plUInt32 uiIndex : m_AutoBindings)
  {
    RemoveDataBinding(uiIndex);
  }

  m_AutoBindings.Clear();

  if (m_bAutobindBlackboards)
  {
    plHybridArray<plBlackboardComponent*, 4> blackboardComponents;

    plGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      pObject->TryGetComponentsOfBaseType(blackboardComponents);

      for (auto pBlackboardComponent : blackboardComponents)
      {
        pBlackboardComponent->EnsureInitialized();

        m_AutoBindings.PushBack(AddBlackboardBinding(pBlackboardComponent->GetBoard()));
      }

      pObject = pObject->GetParent();
    }
  }
}

void plRmlUiCanvasComponentBase::UpdateEventHandler()
{
  if (m_pContext != nullptr)
  {
    if (m_bSendEventMessage)
    {
      m_pContext->RegisterFallbackEventHandler(
        [hComponent = GetHandle()](const plHashedString& sIdentifier, Rml::Event& event)
        {
          plRmlUiCanvasComponentBase* pComponent = nullptr;
          if (plWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
          {
            pComponent->EventHandler(sIdentifier, event);
          }
        });
    }
    else
    {
      m_pContext->DeregisterFallbackEventHandler();
    }
  }
}

void plRmlUiCanvasComponentBase::EventHandler(const plHashedString& sIdentifier, Rml::Event& event)
{
  plMsgRmlUiEvent msg;
  msg.m_sIdentifier = sIdentifier;
  msg.m_sType.Assign(plRmlUiConversionUtils::ToStringView(event.GetType()));

  m_EventMessageSender.SendEventMessage(msg, this, GetOwner());
}


PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvasComponentBase);
