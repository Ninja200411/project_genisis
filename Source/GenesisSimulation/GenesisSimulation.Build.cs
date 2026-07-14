using UnrealBuildTool;

public class GenesisSimulation : ModuleRules
{
    public GenesisSimulation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GenesisCore"
        });
    }
}
