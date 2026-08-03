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

        // This package's own shader headers, shared between C++ and HLSL, plus the engine's under
        // the SDK's base data directory. Both have to be reachable by the same name from either side.
        PrivateIncludePaths.Add(Path.Combine(context.ProjectRoot, "Assets"));
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));

        // RmlCore links freetype, so the DLL has to travel with this plugin.
        PlasmaPackageFreetype.StageRuntime(context);
    }
}
