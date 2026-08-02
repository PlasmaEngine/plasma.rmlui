using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class RmlUiPluginModule : ModuleRules
{
    public RmlUiPluginModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/RmlUiPlugin", "RmlUiPluginPCH.h",
            "BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB");

        // RmlUi's own libraries, vendored in this package.
        PublicDependencies.Add("RmlCoreModule");
        PublicDependencies.Add("RmlDebuggerModule");

        // Shader headers shared with the engine's base data directory. In the engine tree this was
        // a relative hop out of the plugin folder; here it resolves against the SDK.
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));

        // RmlCore links freetype, so the DLL has to travel with this plugin.
        PlasmaPackageFreetype.StageRuntime(context);
    }
}
