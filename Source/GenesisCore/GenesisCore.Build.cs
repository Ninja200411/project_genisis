using UnrealBuildTool;

public class GenesisCore : ModuleRules
{
    public GenesisCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Json"
        });
    }
}
