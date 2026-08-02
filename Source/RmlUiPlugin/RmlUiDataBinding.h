#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

#include <Foundation/Basics.h>

namespace Rml
{
  class Context;
}

class PL_RMLUIPLUGIN_DLL plRmlUiDataBinding
{
public:
  virtual ~plRmlUiDataBinding() = default;

  virtual plResult Initialize(Rml::Context& ref_context) = 0;
  virtual void Deinitialize(Rml::Context& ref_context) = 0;

  /// \brief Returns true if anything was updated
  virtual bool Update() = 0;
};
