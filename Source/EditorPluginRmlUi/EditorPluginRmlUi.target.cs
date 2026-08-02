using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginRmlUiTarget : TargetRules
{
    public EditorPluginRmlUiTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plEditorPluginRmlUi";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        TargetDependencies.Add("RmlUiPlugin");
        TargetDependencies.Add("EnginePluginRmlUi");

        ExtraModules.Add("EditorPluginRmlUiModule");
        ExtraModules.Add("RmlCoreModule");
        ExtraModules.Add("RmlDebuggerModule");
        ExtraModules.Add("RmlSvgModule");
        ExtraModules.Add("LunasvgModule");
    }
}
