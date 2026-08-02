using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginRmlUiModule : ModuleRules
{
    public EnginePluginRmlUiModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/EnginePluginRmlUi",
            "EnginePluginRmlUiPCH.h", "BUILDSYSTEM_BUILDING_ENGINEPLUGINRMLUI_LIB");

        PlasmaPackageSdk.ConfigureEditorSdkConsumer(this, context);
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/RmlUiPlugin", "plRmlUiPlugin");

        PublicDependencies.Add("RmlCoreModule");
    }
}
