#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas3DInteractionExampleComponent.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/Input/InputManager.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRmlUiCanvas3DInteractionExampleComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new plDefaultValueAttribute(2.0f)),
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


plRmlUiCanvas3DInteractionExampleComponent::plRmlUiCanvas3DInteractionExampleComponent() = default;
plRmlUiCanvas3DInteractionExampleComponent::~plRmlUiCanvas3DInteractionExampleComponent() = default;

void plRmlUiCanvas3DInteractionExampleComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_fMaxDistance;
}

void plRmlUiCanvas3DInteractionExampleComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_fMaxDistance;
}

void plRmlUiCanvas3DInteractionExampleComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();
}

void plRmlUiCanvas3DInteractionExampleComponent::Interact(plRmlUiInputSnapshot input)
{
  if (!m_pPhysicsWorldModule)
  {
    return;
  }

  plVec3 vRayOrigin = GetOwner()->GetGlobalPosition();
  plVec3 vRayDir = GetOwner()->GetGlobalDirForwards().GetNormalized();

  plPhysicsCastResult hit;

  plPhysicsQueryParameters queryParams{m_uiCollisionLayer};
  queryParams.m_bIgnoreInitialOverlap = true;
  queryParams.m_ShapeTypes = plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic | plPhysicsShapeType::Trigger;

  if (!m_pPhysicsWorldModule->Raycast(hit, vRayOrigin, vRayDir, m_fMaxDistance, queryParams))
  {
    return;
  }
  
  plGameObject* pGameObject = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hShapeObject, pGameObject))
  {
    plLog::Dev("plRmlUiCanvas3DInteractionComponent: failed to acquire a game object from raycast result");
    return;
  }

  plRmlUiCanvas3DComponent* pCanvas = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pCanvas))
  {
    return;
  }

  pCanvas->RaycastInput(vRayOrigin, vRayDir, input);
}

void plRmlUiCanvas3DInteractionExampleComponent::Update()
{
  plRmlUiInputSnapshot input{};
  if (plInputManager::GetInputSlotState(plInputSlot_MouseButton1) == plKeyState::Down)
    input.m_Buttons |= plRmlUiInputButtons::Mouse0;
  if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelUp) == plKeyState::Pressed)
    input.m_Buttons |= plRmlUiInputButtons::MouseWheelUp;
  if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelDown) == plKeyState::Pressed)
    input.m_Buttons |= plRmlUiInputButtons::MouseWheelDown;
  input.m_uiLastCharacter = plInputManager::RetrieveLastCharacter(false);

  Interact(input);
}

PL_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DInteractionComponent);
