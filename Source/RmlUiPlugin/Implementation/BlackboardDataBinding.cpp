#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace plRmlUiInternal
{
  VariantVariableDefinition::VariantVariableDefinition()
    : VariableDefinition(Rml::DataVariableType::Scalar)
  {
  }

  bool VariantVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    plVariant* pValue = static_cast<plVariant*>(pPtr);
    out_variant = plRmlUiConversionUtils::ToVariant(*pValue);

    return true;
  }

  bool VariantVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    plLog::Warning("Can't set the value of an element in an variantarray. Arrays are read-only.");

    return false;
  }

  ////////////////////////////////////////////////////////////////

  BlackboardVariableDefinition::BlackboardVariableDefinition(bool bIsArray)
    : VariableDefinition(bIsArray ? Rml::DataVariableType::Array : Rml::DataVariableType::Scalar)
  {
  }

  bool BlackboardVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    plVariant v = pInfo->m_pBlackboard->GetEntryValue(pInfo->m_sName);
    out_variant = plRmlUiConversionUtils::ToVariant(v);

    return true;
  }

  bool BlackboardVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    pInfo->m_pBlackboard->SetEntryValue(pInfo->m_sName, plRmlUiConversionUtils::ToVariant(variant));

    return true;
  }

  int BlackboardVariableDefinition::Size(void* pPtr)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    plVariant v = pInfo->m_pBlackboard->GetEntryValue(pInfo->m_sName);
    if (v.IsA<plVariantArray>())
    {
      return static_cast<int>(v.Get<plVariantArray>().GetCount());
    }

    return 0;
  }

  Rml::DataVariable BlackboardVariableDefinition::Child(void* pPtr, const Rml::DataAddressEntry& address)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    plVariant v = pInfo->m_pBlackboard->GetEntryValue(pInfo->m_sName);
    if (!v.IsA<plVariantArray>())
      return Rml::DataVariable();

    auto& a = v.Get<plVariantArray>();

    const int index = address.index;
    const int count = static_cast<int>(a.GetCount());
    if (index < 0 || index >= count)
    {
      if (address.name == "size")
        return Rml::MakeLiteralIntVariable(count);

      plLog::Warning("Data array index out of bounds.");
      return Rml::DataVariable();
    }

    const plVariant& element = a[index];
    return Rml::DataVariable(&m_ScalarDefinition, const_cast<plVariant*>(&element));
  }

  //////////////////////////////////////////////////////////////////

  BlackboardDataBinding::BlackboardDataBinding(const plSharedPtr<plBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
    , m_ScalarDefinition(false)
    , m_ArrayDefinition(true)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  plResult BlackboardDataBinding::Initialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard == nullptr)
      return PL_FAILURE;

    const char* szModelName = m_pBlackboard->GetName();
    if (plStringUtils::IsNullOrEmpty(szModelName))
    {
      plLog::Error("Can't bind a blackboard without a valid name");
      return PL_FAILURE;
    }

    Rml::DataModelConstructor constructor = ref_context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return PL_FAILURE;
    }

    for (auto it : m_pBlackboard->GetAllEntries())
    {
      auto type = it.Value().m_Value.GetType();
      if ((type >= plVariantType::Invalid && type <= plVariantType::Double) ||
          type == plVariantType::String || type == plVariantType::HashedString ||
          type == plVariantType::VariantArray)
      {
        auto& info = m_EntryInfos.ExpandAndGetRef();
        info.m_pBlackboard = m_pBlackboard;
        info.m_sName = it.Key();
        info.m_uiChangeCounter = it.Value().m_uiChangeCounter;
        info.m_bIsArray = type == plVariantType::VariantArray;
      }
    }

    for (auto& info : m_EntryInfos)
    {
      auto pDefinition = info.m_bIsArray ? &m_ArrayDefinition : &m_ScalarDefinition;
      constructor.BindCustomDataVariable(plRmlUiConversionUtils::ToString(info.m_sName), Rml::DataVariable(pDefinition, &info));
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();

    return PL_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard != nullptr)
    {
      ref_context.RemoveDataModel(m_pBlackboard->GetName());
    }
  }

  bool BlackboardDataBinding::Update()
  {
    bool bUpdated = false;

    if (m_uiBlackboardChangeCounter != m_pBlackboard->GetBlackboardChangeCounter())
    {
      plLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_pBlackboard->GetBlackboardEntryChangeCounter())
    {
      for (auto& info : m_EntryInfos)
      {
        auto pEntry = m_pBlackboard->GetEntry(info.m_sName);

        if (pEntry != nullptr && info.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          m_hDataModel.DirtyVariable(plRmlUiConversionUtils::ToString(info.m_sName));
          info.m_uiChangeCounter = pEntry->m_uiChangeCounter;
          bUpdated = true;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }

    return bUpdated;
  }

} // namespace plRmlUiInternal
