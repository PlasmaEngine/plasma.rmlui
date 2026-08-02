using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginRmlUiModule : ModuleRules
{
    public EditorPluginRmlUiModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/EditorPluginRmlUi",
            "EditorPluginRmlUiPCH.h", "BUILDSYSTEM_BUILDING_EDITORPLUGINRMLUI_LIB");

        PlasmaPackageSdk.ConfigureEditorSdkConsumer(this, context);

        // The RmlUi asset document type builds on EditorPluginAssets, from the plasma.assets built-in.
        PlasmaPackageSdk.ConfigureAssetsBuiltIn(this, context);

        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/RmlUiPlugin", "plRmlUiPlugin");
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/EnginePluginRmlUi", "plEnginePluginRmlUi");

        PublicDependencies.Add("RmlCoreModule");

        PlasmaPackageQt.ConfigureQtModule(this, context, "EditorPluginRmlUiModule",
            "Source/EditorPluginRmlUi", "Core", "Gui", "Widgets");
    }
}
