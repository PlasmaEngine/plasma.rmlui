#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Time/Clock.h>

namespace plRmlUiInternal
{
  double SystemInterface::GetElapsedTime()
  {
    return plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds();
  }

  int SystemInterface::TranslateString(Rml::String& out_sTranslated, const Rml::String& sInput)
  {
    plStringView sTrimmedInput = plRmlUiConversionUtils::ToStringView(sInput);
    sTrimmedInput.Trim(" \t\r\n");
    if (sTrimmedInput.IsEmpty() == false)
    {
      plStringView sTranslated = plTranslate(sTrimmedInput);
      if (sTranslated != sTrimmedInput)
      {
        out_sTranslated = plRmlUiConversionUtils::ToString(sTranslated);
        return 1;
      }
    }

    out_sTranslated = sInput;
    return 0;
  }

  void SystemInterface::JoinPath(Rml::String& out_sTranslatedPath, const Rml::String& sDocumentPath, const Rml::String& sPath)
  {
    if (plFileSystem::ExistsFile(plRmlUiConversionUtils::ToStringView(sPath)))
    {
      // path is already a valid path for pl file system so don't join with document path
      out_sTranslatedPath = sPath;
      return;
    }

    Rml::SystemInterface::JoinPath(out_sTranslatedPath, sDocumentPath, sPath);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& sMessage)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        plLog::Error("{}", plRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ASSERT:
        PL_REPORT_FAILURE(sMessage.c_str());
        break;

      case Rml::Log::LT_WARNING:
        plLog::Warning("{}", plRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        plLog::Info("{}", plRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_DEBUG:
        plLog::Debug("{}", plRmlUiConversionUtils::ToStringView(sMessage));
        break;
      default:
        break;
    }

    return true;
  }

} // namespace plRmlUiInternal
