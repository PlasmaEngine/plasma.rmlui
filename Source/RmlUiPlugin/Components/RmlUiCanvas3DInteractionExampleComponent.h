#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RmlUiPlugin/RmlUiInput.h>

using plRmlUiCanvas3DInteractionComponentManager = plComponentManagerSimple<class plRmlUiCanvas3DInteractionExampleComponent, plComponentUpdateType::WhenSimulating, plBlockStorageType::Compact>;

class plCpuMeshResource;
class plPhysicsWorldModuleInterface;

class PL_RMLUIPLUGIN_DLL plRmlUiCanvas3DInteractionExampleComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plRmlUiCanvas3DInteractionExampleComponent, plComponent, plRmlUiCanvas3DInteractionComponentManager);

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

public:
  plRmlUiCanvas3DInteractionExampleComponent();
  ~plRmlUiCanvas3DInteractionExampleComponent();

  void Update();
  void Interact(plRmlUiInputSnapshot input);

private:

  plUInt32 m_uiCollisionLayer = 0;
  float m_fMaxDistance = 2.0f;
  plPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};
