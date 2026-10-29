#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/UIServices/UIServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

plStringView FindNextHREF(plStringView& ref_sRml)
{
  const char* szCurrent = ref_sRml.FindSubString("href");
  if (szCurrent == nullptr)
    return plStringView();

  const char* szStart = nullptr;
  const char* szEnd = nullptr;
  while (*szCurrent != '\0')
  {
    if (*szCurrent == '\"')
    {
      if (szStart == nullptr)
      {
        szStart = szCurrent + 1;
      }
      else
      {
        szEnd = szCurrent;
        break;
      }
    }

    ++szCurrent;
  }

  if (szStart != nullptr && szEnd != nullptr)
  {
    ref_sRml.SetStartPosition(szEnd);

    plStringView href = plStringView(szStart, szEnd);
    if (href.HasExtension(".rcss"))
    {
      return href;
    }
    if (href.HasExtension(".rml"))
    {
      return href;
    }
  }

  return plStringView();
}

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plRmlUiAssetDocument::plRmlUiAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plRmlUiAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple)
{
}

void plRmlUiAssetDocument::OpenExternalEditor()
{
  plStringBuilder sFile(GetProperties()->m_sRmlFile);

  if (!plFileSystem::ExistsFile(sFile))
  {
    plEditorUiServices::MessageBoxInformation(plFmt("Can't find the file '{}'.\nTo create an RML file click the button next to 'RmlFile'.", sFile));

    ShowDocumentStatus("RML file doesn't exist.");
    return;
  }

  plStringBuilder sFileAbs;
  if (plFileSystem::ResolvePath(sFile, &sFileAbs, nullptr).Failed())
    return;

  {
    const plString args[] = {plToolsProject::GetSingleton()->GetProjectDirectory(), sFileAbs};

    if (plEditorUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      plEditorUiServices::OpenFileInDefaultProgram(sFileAbs);
    }
  }
}

plTransformStatus plRmlUiAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plRmlUiAssetProperties* pProp = GetProperties();

  plRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;
  desc.m_ScaleMode = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  PL_SUCCEED_OR_RETURN(FindDependencies(desc.m_DependencyFile, pProp->m_sRmlFile));

  PL_SUCCEED_OR_RETURN(desc.Save(stream));

  return plStatus(PL_SUCCESS);
}

plTransformStatus plRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

plStatus plRmlUiAssetDocument::FindDependencies(plDependencyFile& ref_Dependencies, plStringView sFilePath) const
{
  plStringBuilder sContent;
  {
    plFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return plStatus(plFmt("Failed to read file: '{}'", sFilePath));

    sContent.ReadAll(reader);
  }

  const plStringView sFileDir = sFilePath.GetFileDirectory();

  plStringBuilder sTemp;
  plStringView sContentView = sContent;

  while (true)
  {
    plStringView href = FindNextHREF(sContentView);
    if (href.IsEmpty())
      break;

    if (plFileSystem::ExistsFile(href))
    {
      ref_Dependencies.AddFileDependency(href);
      PL_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, href));
      continue;
    }

    sTemp.SetPath(sFileDir, href);
    if (plFileSystem::ExistsFile(sTemp))
    {
      ref_Dependencies.AddFileDependency(sTemp);
      PL_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, sTemp));
      continue;
    }
  }

  return plResult(PL_SUCCESS);
}

void plRmlUiAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const plRmlUiAssetProperties* pProp = GetProperties();

  plDependencyFile deps;
  FindDependencies(deps, pProp->m_sRmlFile).IgnoreResult();

  for (const auto& file : deps.GetFileDependencies())
  {
    pInfo->m_TransformDependencies.Insert(file);
  }
}
