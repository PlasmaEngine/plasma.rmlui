#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plRmlUiContext;
struct plMsgExtractRenderData;

using plRmlUiResourceHandle = plTypedResourceHandle<class plRmlUiResource>;

/// \brief The RML configuration to be used on a specific platform
struct PL_RMLUIPLUGIN_DLL plRmlUiConfiguration
{
  plDynamicArray<plString> m_Fonts;

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/RmlUiConfig.ddl"_plsv;

  plResult Save(plStringView sFile = s_sConfigFile) const;
  plResult Load(plStringView sFile = s_sConfigFile);

  bool operator==(const plRmlUiConfiguration& rhs) const;
  bool operator!=(const plRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

class PL_RMLUIPLUGIN_DLL plRmlUi
{
  PL_DECLARE_SINGLETON(plRmlUi);

public:
  plRmlUi();
  ~plRmlUi();

  plRmlUiContext* CreateContext(const char* szName, const plVec2U32& vInitialSize);
  void DeleteContext(plRmlUiContext* pContext);

  bool AnyContextWantsInput();

  plResult LoadDocumentFromResource(plRmlUiContext& ref_context, const plRmlUiResourceHandle& hResource);
  plResult LoadDocumentFromString(plRmlUiContext& ref_context, const plStringView& sContent);

  void UnloadDocument(plRmlUiContext& ref_context);

  void ClearCaches();

  void ExtractContext(plRmlUiContext& ref_context, plGALTextureHandle hTexture);

  plMutex& GetContextMutex();

private:
  struct Data;
  plUniquePtr<Data> m_pData;
};
