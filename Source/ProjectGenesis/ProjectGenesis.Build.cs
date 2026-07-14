using UnrealBuildTool;

public class ProjectGenesis : ModuleRules
{
    public ProjectGenesis(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "GameplayTags",
            "GenesisCore",
            "GenesisSimulation"
        });
    }
}
