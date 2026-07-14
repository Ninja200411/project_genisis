#include "GenesisContentRegistry.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FGenesisContentRecord MakeRecord(
    const FName Id,
    const EGenesisContentDefinitionType Type,
    const EGenesisContentSourceTier Tier,
    const TCHAR* Path,
    const TCHAR* Data)
{
    FGenesisContentRecord Record;
    Record.Id = Id;
    Record.Type = Type;
    Record.Source.Tier = Tier;
    Record.Source.PackageId = Tier == EGenesisContentSourceTier::Core ? TEXT("core") : TEXT("test.package");
    Record.Source.NormalizedPath = Path;
    Record.CanonicalData = Data;
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisContentRegistryDeterministicHashTest,
    "Genesis.Core.Content.Registry.DeterministicHash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisContentRegistryDeterministicHashTest::RunTest(const FString& Parameters)
{
    FGenesisContentRecord Resource = MakeRecord(
        TEXT("resource:iron"), EGenesisContentDefinitionType::Resource,
        EGenesisContentSourceTier::Core, TEXT("Core\\Resources\\Iron.json"), TEXT("{id:resource:iron}"));
    FGenesisContentRecord Recipe = MakeRecord(
        TEXT("recipe:plate"), EGenesisContentDefinitionType::Recipe,
        EGenesisContentSourceTier::Core, TEXT("core/recipes/plate.json"), TEXT("{id:recipe:plate}"));
    Recipe.References.Add(Resource.Id);
    Recipe.Consumes.Add(Resource.Id);

    FGenesisContentRegistry First;
    TArray<FGenesisContentDiagnostic> Diagnostics;
    TestTrue(TEXT("First order loads"), First.Reload({Recipe, Resource}, Diagnostics));
    const FString FirstHash = First.GetSnapshot()->GetContentHash();

    FGenesisContentRegistry Second;
    TestTrue(TEXT("Second order loads"), Second.Reload({Resource, Recipe}, Diagnostics));
    TestEqual(TEXT("Filesystem order does not change hash"), Second.GetSnapshot()->GetContentHash(), FirstHash);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisContentRegistryAtomicReloadTest,
    "Genesis.Core.Content.Registry.AtomicReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisContentRegistryAtomicReloadTest::RunTest(const FString& Parameters)
{
    FGenesisContentRegistry Registry;
    TArray<FGenesisContentDiagnostic> Diagnostics;
    FGenesisContentRecord Valid = MakeRecord(
        TEXT("resource:water"), EGenesisContentDefinitionType::Resource,
        EGenesisContentSourceTier::Core, TEXT("core/resources/water.json"), TEXT("{id:resource:water}"));
    TestTrue(TEXT("Valid snapshot publishes"), Registry.Reload({Valid}, Diagnostics));
    const TSharedPtr<const FGenesisContentRegistrySnapshot> Published = Registry.GetSnapshot();

    FGenesisContentRecord Invalid = MakeRecord(
        TEXT("recipe:missing"), EGenesisContentDefinitionType::Recipe,
        EGenesisContentSourceTier::Core, TEXT("core/recipes/missing.json"), TEXT("{id:recipe:missing}"));
    Invalid.References.Add(TEXT("resource:unknown"));
    TestFalse(TEXT("Invalid reload fails"), Registry.Reload({Invalid}, Diagnostics));
    TestTrue(TEXT("Previous snapshot remains published"), Registry.GetSnapshot() == Published);
    TestEqual(TEXT("Diagnostic identifies definition"), Diagnostics[0].DefinitionId, Invalid.Id);
    TestEqual(TEXT("Diagnostic identifies field"), Diagnostics[0].FieldPath, FString(TEXT("references")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisContentRegistryOverrideTest,
    "Genesis.Core.Content.Registry.OverrideRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisContentRegistryOverrideTest::RunTest(const FString& Parameters)
{
    FGenesisContentRecord Core = MakeRecord(
        TEXT("building:mine"), EGenesisContentDefinitionType::Building,
        EGenesisContentSourceTier::Core, TEXT("core/buildings/mine.json"), TEXT("{power:10}"));
    FGenesisContentRecord Mod = MakeRecord(
        Core.Id, EGenesisContentDefinitionType::Building,
        EGenesisContentSourceTier::Mod, TEXT("mods/test/buildings/mine.json"), TEXT("{power:12}"));
    Mod.Replaces = Core.Id;

    FGenesisContentRegistry Registry;
    TArray<FGenesisContentDiagnostic> Diagnostics;
    TestTrue(TEXT("Explicit compatible override succeeds"), Registry.Reload({Mod, Core}, Diagnostics));
    TestEqual(TEXT("Override source is retained"), Registry.GetSnapshot()->Find(Core.Id)->Source.Tier, EGenesisContentSourceTier::Mod);

    Mod.Replaces = NAME_None;
    TestFalse(TEXT("Implicit override fails"), Registry.Reload({Core, Mod}, Diagnostics));
    TestEqual(TEXT("Conflict has stable code"), Diagnostics[0].Code, FName(TEXT("GEN-CONTENT-OVERRIDE-CONFLICT")));
    return true;
}

#endif
