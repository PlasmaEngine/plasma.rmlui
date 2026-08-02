using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class RmlSvgModule : ModuleRules
{
    public RmlSvgModule(BuildContext context) : base(context)
    {
        Type = ModuleType.ThirdParty;
        SourceDirectory = "ThirdParty/RmlUi/Source/SVG";

        // Folded into each RmlUi plugin DLL rather than imported across a DLL boundary.
        ExportsSymbols = false;

        PublicIncludePaths.Add("../../Include");

        PublicDefinitions.Add("RMLUI_STATIC_LIB");

        PrivateDefinitions.Add("RMLUI_CORE_EXPORTS");
        PrivateDefinitions.Add("RMLUI_SVG_PLUGIN");

        PrivateDependencies.Add("RmlCoreModule");
        PrivateDependencies.Add("LunasvgModule");

        CppStandard = CppStandard.Cpp20;
        TreatWarningsAsErrors = false;
    }
}