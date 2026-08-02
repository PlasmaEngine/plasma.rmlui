#pragma once

#include <RmlUiPlugin/Components/RmlUiMessages.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInput.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct plMsgExtractRenderData;
class plRmlUiContext;
class plRmlUiDataBinding;
class plBlackboard;

using plRmlUiResourceHandle = plTypedResourceHandle<class plRmlUiResource>;

class PL_RMLUIPLUGIN_DLL plRmlUiCanvasComponentBase : public plRenderComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plRmlUiCanvasComponentBase, plRenderComponent);

public:
  plRmlUiCanvasComponentBase();
  ~plRmlUiCanvasComponentBase();

  plRmlUiCanvasComponentBase& operator=(plRmlUiCanvasComponentBase&& rhs);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnDeactivated() override;

  virtual void Update();

  virtual bool ReceiveInput(const plVec2& vMousePosInsideCanvas, plRmlUiInputSnapshot input);

  PL_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(RmlResource, m_hResource, SetRmlResource);
  void SetRmlResource(const plRmlUiResourceHandle& hResource);                // [ property ]
  const plRmlUiResourceHandle& GetRmlResource() const { return m_hResource; } // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                           // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; } // [ property ]

  /// \brief If enabled, the component will send an plMsgRmlUiEventMessage for each RmlUI event that is triggered on the context.
  void SetSendEventMessage(bool bSendEventMessage);                // [ property ]
  bool GetSendEventMessage() const { return m_bSendEventMessage; } // [ property ]

  void SetOnDemandUpdate(bool bOnDemandUpdate);                    // [ property ]
  bool GetOnDemandUpdate() const { return m_bOnDemandUpdate; }     // [ property ]

  plUInt32 AddDataBinding(plUniquePtr<plRmlUiDataBinding>&& pDataBinding);
  void RemoveDataBinding(plUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  plUInt32 AddBlackboardBinding(const plSharedPtr<plBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(plUInt32 uiDataBindingIndex);

  plRmlUiContext* GetOrCreateRmlContext();
  plRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

protected:
  virtual void OnMsgReload(plMsgRmlUiReload& msg);                            // [ msg handler ]
  virtual void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const = 0; // [ msg handler ]

  void UpdateCachedValues();
  void UpdateAutobinding();
  void UpdateEventHandler();

  void EventHandler(const plHashedString& sIdentifier, Rml::Event& event);

  plRmlUiResourceHandle m_hResource;
  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  plVec2U32 m_vSize = plVec2U32::MakeZero();
  plVec2U32 m_vReferenceResolution = plVec2U32::MakeZero();
  bool m_bAutobindBlackboards = false;
  bool m_bSendEventMessage = false;
  bool m_bOnDemandUpdate = true;
  bool m_bNeedsUpdate = false;
  plUInt16 m_uiContextID = 0;

  plRmlUiContext* m_pContext = nullptr;
  plRmlUiInputProvider m_InputProvider;

  plDynamicArray<plUniquePtr<plRmlUiDataBinding>> m_DataBindings;
  plDynamicArray<plUInt32> m_AutoBindings;

  plEventMessageSender<plMsgRmlUiEvent> m_EventMessageSender; // [ event ]
};
