using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class RmlUiPluginTarget : TargetRules
{
    public RmlUiPluginTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plRmlUiPlugin";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        ExtraModules.Add("RmlUiPluginModule");
        ExtraModules.Add("RmlCoreModule");
        ExtraModules.Add("RmlDebuggerModule");
        ExtraModules.Add("RmlSvgModule");
        ExtraModules.Add("LunasvgModule");
    }
}
