#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
#  include <RmlUi/Include/RmlUi/Debugger.h>
#endif

plResult plRmlUiConfiguration::Save(plStringView sFile) const
{
  PL_LOG_BLOCK("plRmlUiConfiguration::Save()");

  plFileWriter file;
  if (file.Open(sFile).Failed())
    return PL_FAILURE;

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  writer.BeginObject("Fonts");
  for (auto& font : m_Fonts)
  {
    plOpenDdlUtils::StoreString(writer, font);
  }
  writer.EndObject();

  return PL_SUCCESS;
}

plResult plRmlUiConfiguration::Load(plStringView sFile)
{
  PL_LOG_BLOCK("plRmlUiConfiguration::Load()");

  m_Fonts.Clear();

  plFileReader file;
  if (file.Open(sFile).Failed())
    return PL_FAILURE;

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse RmlUi config file '{0}'", sFile);
    return PL_FAILURE;
  }

  const plOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pChild = pTree->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Fonts"))
    {
      for (const plOpenDdlReaderElement* pFont = pChild->GetFirstChild(); pFont != nullptr; pFont = pFont->GetSibling())
      {
        m_Fonts.PushBack(pFont->GetPrimitivesString()[0]);
      }
    }
  }

  return PL_SUCCESS;
}

bool plRmlUiConfiguration::operator==(const plRmlUiConfiguration& rhs) const
{
  return m_Fonts == rhs.m_Fonts;
}

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_SINGLETON(plRmlUi);

struct plRmlUi::Data
{
  plMutex m_ExtractionMutex;
  plRmlUiInternal::RenderInterface m_RenderInterface;

  plRmlUiInternal::FileInterface m_FileInterface;
  plRmlUiInternal::SystemInterface m_SystemInterface;

  plRmlUiInternal::ContextInstancer m_ContextInstancer;
  plRmlUiInternal::EventListenerInstancer m_EventListenerInstancer;

  plMutex m_ContextsMutex;
  plDynamicArray<plRmlUiContext*> m_Contexts;

  plUInt64 m_uiLastClearedCacheFrame = 0;

  plRmlUiConfiguration m_Config;
};

plRmlUi::plRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pData = PL_DEFAULT_NEW(Data);

  Rml::SetRenderInterface(&m_pData->m_RenderInterface);
  Rml::SetFileInterface(&m_pData->m_FileInterface);
  Rml::SetSystemInterface(&m_pData->m_SystemInterface);

  Rml::Initialise();

  Rml::Factory::RegisterContextInstancer(&m_pData->m_ContextInstancer);
  Rml::Factory::RegisterEventListenerInstancer(&m_pData->m_EventListenerInstancer);

  if (m_pData->m_Config.Load().Failed())
  {
    plLog::Warning("No valid RmlUi configuration file available in '{}'.", plRmlUiConfiguration::s_sConfigFile);
    return;
  }

  for (auto& font : m_pData->m_Config.m_Fonts)
  {
    // Treat last font as fall back
    bool bIsFallbackFont = (font == m_pData->m_Config.m_Fonts.PeekBack());

    if (Rml::LoadFontFace(font.GetData(), bIsFallbackFont) == false)
    {
      plLog::Warning("Failed to load font face '{0}'.", font);
    }
  }
}

plRmlUi::~plRmlUi()
{
  Rml::Shutdown();
}

plRmlUiContext* plRmlUi::CreateContext(const char* szName, const plVec2U32& vInitialSize)
{
  PL_LOCK(m_pData->m_ContextsMutex);

  plRmlUiContext* pContext = static_cast<plRmlUiContext*>(Rml::CreateContext(szName, Rml::Vector2i(vInitialSize.x, vInitialSize.y)));
  PL_ASSERT_DEV(pContext != nullptr, "RML UI context creation failed");

  m_pData->m_Contexts.PushBack(pContext);

// #if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
//   Rml::Debugger::Initialise(pContext);
//   Rml::Debugger::SetVisible(true);
// #endif

  return pContext;
}

void plRmlUi::DeleteContext(plRmlUiContext* pContext)
{
  PL_LOCK(m_pData->m_ContextsMutex);

  m_pData->m_Contexts.RemoveAndCopy(pContext);

// #if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
//   Rml::Debugger::Shutdown();
// #else
  Rml::RemoveContext(pContext->GetName());
//#endif
}

bool plRmlUi::AnyContextWantsInput()
{
  PL_LOCK(m_pData->m_ContextsMutex);

  for (auto pContext : m_pData->m_Contexts)
  {
    if (pContext->WantsInput())
      return true;
  }

  return false;
}

plResult plRmlUi::LoadDocumentFromResource(plRmlUiContext& ref_context, const plRmlUiResourceHandle& hResource)
{
  UnloadDocument(ref_context);

  if (hResource.IsValid())
  {
    plResourceLock<plRmlUiResource> pResource(hResource, plResourceAcquireMode::BlockTillLoaded);
    if (pResource.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
      PL_LOCK(m_pData->m_ContextsMutex);

      ref_context.LoadDocument(pResource->GetRmlFile().GetData());
    }
  }

  return ref_context.HasDocument() ? PL_SUCCESS : PL_FAILURE;
}

plResult plRmlUi::LoadDocumentFromString(plRmlUiContext& ref_context, const plStringView& sContent)
{
  UnloadDocument(ref_context);

  if (!sContent.IsEmpty())
  {
    Rml::String sRmlContent = Rml::String(sContent.GetStartPointer(), sContent.GetElementCount());

    // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
    PL_LOCK(m_pData->m_ContextsMutex);

    ref_context.LoadDocumentFromMemory(sRmlContent);
  }

  return ref_context.HasDocument() ? PL_SUCCESS : PL_FAILURE;
}

void plRmlUi::UnloadDocument(plRmlUiContext& ref_context)
{
  if (ref_context.HasDocument())
  {
    // RmlUi is not thread safe, so we need to make that we only load/unload one document at a time.
    PL_LOCK(m_pData->m_ContextsMutex);

    static_cast<Rml::Context&>(ref_context).UnloadDocument(ref_context.GetDocument(0));
  }
}

void plRmlUi::ClearCaches()
{
  plUInt64 uiCurrentFrame = plRenderWorld::GetFrameCounter();
  if (uiCurrentFrame == m_pData->m_uiLastClearedCacheFrame)
    return;

  m_pData->m_uiLastClearedCacheFrame = uiCurrentFrame;

  PL_LOCK(m_pData->m_ContextsMutex);
  Rml::Factory::ClearStyleSheetCache();
  Rml::Factory::ClearTemplateCache();
}

void plRmlUi::ExtractContext(plRmlUiContext& ref_context, plGALTextureHandle hTexture)
{
  if (ref_context.HasDocument() == false)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  PL_LOCK(m_pData->m_ExtractionMutex);

  ref_context.ExtractRenderData(m_pData->m_RenderInterface, hTexture);
}

plMutex& plRmlUi::GetContextMutex()
{
  return m_pData->m_ContextsMutex;
}
