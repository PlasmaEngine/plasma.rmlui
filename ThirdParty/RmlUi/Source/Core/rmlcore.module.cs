using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class RmlCoreModule : ModuleRules
{
    public RmlCoreModule(BuildContext context) : base(context)
    {
        Type = ModuleType.ThirdParty;
        SourceDirectory = "ThirdParty/RmlUi/Source/Core";

        // RmlUi's symbols are folded into each RmlUi plugin DLL rather than shared via
        // a dedicated DLL, so keep this module compiled into every consumer instead of
        // importing it across a DLL boundary.
        ExportsSymbols = false;

        PublicIncludePaths.Add("../../Include");
        PrivateIncludePaths.Add(".");
        PrivateIncludePaths.Add("../../Dependencies/freetype/include");

        PublicDefinitions.Add("BUILDSYSTEM_ENABLE_RMLUI_SUPPORT");
        PublicDefinitions.Add("RMLUI_CUSTOM_RTTI");
        // Fold RmlUi statically into each plugin DLL: no dllexport/dllimport so the
        // duplicated Rml symbols stay internal to each DLL and never collide.
        PublicDefinitions.Add("RMLUI_STATIC_LIB");
        PrivateDefinitions.Add("RMLUI_FONT_ENGINE_FREETYPE");
        PrivateDefinitions.Add("RMLUI_SVG_PLUGIN");

        ConfigureForPlatform(TargetPlatform.Windows, () =>
        {
            // LogDefault.cpp calls MessageBoxA when RmlUi reports a fatal log message.
            PublicSystemLibraries.Add("User32.lib");

            if (Context.Architecture == TargetArchitecture.X86)
            {
                PrivateLibraries.Add("ThirdParty/RmlUi/Dependencies/freetype/win/x86/freetype.lib");
            }
            else if (Context.Architecture == TargetArchitecture.ARM64)
            {
                PrivateLibraries.Add("ThirdParty/RmlUi/Dependencies/freetype/win/arm64/freetype.lib");
            }
            else
            {
                PrivateLibraries.Add("ThirdParty/RmlUi/Dependencies/freetype/win/x64/freetype.lib");
            }
        });
        ConfigureForPlatform(TargetPlatform.Linux, () =>
        {
            // the system FreeType, as the engine's EditorUi links it
            PublicSystemLibraries.Add("freetype");
        });
        ConfigureForPlatform(TargetPlatform.Mac, () =>
        {
            // the SDK's static FreeType for Apple silicon; this vendored copy has Windows libraries only
            PrivateLibraries.Add(PlasmaPackageSdk.Root + "/Code/ThirdParty/RmlUi/Dependencies/freetype/mac/arm64/libfreetype.a");
        });

        CppStandard = CppStandard.Cpp20;
        TreatWarningsAsErrors = false;
    }
}