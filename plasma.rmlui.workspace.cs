// #pl-version 1

using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class PlasmaRmlUiWorkspace : WorkspaceRules
{
    public PlasmaRmlUiWorkspace(BuildContext context) : base(context)
    {
        TargetNames.Add("RmlUiPlugin");
        TargetNames.Add("EnginePluginRmlUi");
        TargetNames.Add("EditorPluginRmlUi");
    }
}
