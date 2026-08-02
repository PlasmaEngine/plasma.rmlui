using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class RmlDebuggerModule : ModuleRules
{
    public RmlDebuggerModule(BuildContext context) : base(context)
    {
        Type = ModuleType.ThirdParty;
        SourceDirectory = "ThirdParty/RmlUi/Source/Debugger";

        // Folded into each RmlUi plugin DLL rather than imported across a DLL boundary.
        ExportsSymbols = false;

        PublicIncludePaths.Add("../../Include");

        PublicDefinitions.Add("BUILDSYSTEM_ENABLE_RMLUI_SUPPORT");
        PublicDefinitions.Add("RMLUI_CUSTOM_RTTI");
        PublicDefinitions.Add("RMLUI_STATIC_LIB");

        PublicDependencies.Add("RmlCoreModule");

        PrivateDependencies.Add("RmlSvgModule");

        PrivateDependencies.Add("LunasvgModule");

        CppStandard = CppStandard.Cpp20;
        TreatWarningsAsErrors = false;
    }
}