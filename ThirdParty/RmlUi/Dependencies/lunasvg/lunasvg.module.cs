using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class LunasvgModule : ModuleRules
{
    public LunasvgModule(BuildContext context) : base(context)
    {
        Type = ModuleType.ThirdParty;
        SourceDirectory = "ThirdParty/RmlUi/Dependencies/lunasvg";

        // Statically folded into each RmlUi plugin DLL (LUNASVG_BUILD_STATIC); never
        // imported across a DLL boundary.
        ExportsSymbols = false;

        PublicIncludePaths.Add("include");
        PrivateIncludePaths.Add("plutovg/include");

        PublicDefinitions.Add("LUNASVG_BUILD_STATIC");
        PublicDefinitions.Add("BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT");

        PrivateDefinitions.Add("LUNASVG_BUILD");
        PrivateDefinitions.Add("PLUTOVG_BUILD_STATIC");
        PrivateDefinitions.Add("_CRT_SECURE_NO_WARNINGS");

        ConfigureForPlatform(TargetPlatform.Windows, () =>
        {
            AdditionalCompilerFlags.Add("/wd4018");
            AdditionalCompilerFlags.Add("/wd4244");
            AdditionalCompilerFlags.Add("/wd4267");
            AdditionalCompilerFlags.Add("/wd4456");
            AdditionalCompilerFlags.Add("/wd4458");
            AdditionalCompilerFlags.Add("/wd4701");
        });

        CStandard = CStandard.C17;
        CppStandard = CppStandard.Cpp20;
        UseUnityBuild = false;
        TreatWarningsAsErrors = false;
    }
}