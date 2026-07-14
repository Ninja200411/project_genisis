using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectGenesisTarget : TargetRules
{
    public ProjectGenesisTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "ProjectGenesis" });
    }
}
