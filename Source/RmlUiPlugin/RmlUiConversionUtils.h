#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/Variant.h>
#include <RmlUi/Include/RmlUi/Core.h>

namespace plRmlUiConversionUtils
{
  PL_RMLUIPLUGIN_DLL plVariant ToVariant(const Rml::Variant& value, plVariant::Type::Enum targetType = plVariant::Type::Invalid);
  PL_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const plVariant& value);

  PL_ALWAYS_INLINE plString ToString(const Rml::String& value)
  {
    return plStringView(value.c_str(), static_cast<plUInt32>(value.length()));
  }

  PL_ALWAYS_INLINE plString ToString(const Rml::StringView& value)
  {
    return plStringView(value.begin(), static_cast<plUInt32>(value.size()));
  }

  PL_ALWAYS_INLINE Rml::String ToString(const plString& sValue)
  {
    return Rml::String(sValue.GetData(), sValue.GetElementCount());
  }

  PL_ALWAYS_INLINE Rml::String ToString(plStringView sValue)
  {
    return Rml::String(sValue.GetStartPointer(), sValue.GetElementCount());
  }

  PL_ALWAYS_INLINE plStringView ToStringView(const Rml::StringView& value)
  {
    return plStringView(value.begin(), static_cast<plUInt32>(value.size()));
  }

  PL_ALWAYS_INLINE Rml::StringView ToStringView(plStringView sValue)
  {
    return Rml::StringView(sValue.GetStartPointer(), sValue.GetElementCount());
  }

} // namespace plRmlUiConversionUtils
