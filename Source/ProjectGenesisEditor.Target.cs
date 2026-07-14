using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectGenesisEditorTarget : TargetRules
{
    public ProjectGenesisEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "ProjectGenesis" });
    }
}
