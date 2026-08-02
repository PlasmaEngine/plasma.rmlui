using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginRmlUiTarget : TargetRules
{
    public EnginePluginRmlUiTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plEnginePluginRmlUi";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        TargetDependencies.Add("RmlUiPlugin");

        ExtraModules.Add("EnginePluginRmlUiModule");
        ExtraModules.Add("RmlCoreModule");
        ExtraModules.Add("RmlDebuggerModule");
        ExtraModules.Add("RmlSvgModule");
        ExtraModules.Add("LunasvgModule");
    }
}
