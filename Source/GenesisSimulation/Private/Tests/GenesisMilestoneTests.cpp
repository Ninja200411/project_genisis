#include "GenesisConstruction.h"
#include "GenesisProduction.h"
#include "GenesisVerification.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisRecipeSelectionTest,
    "Genesis.Simulation.Production.DeterministicRecipeSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisRecipeSelectionTest::RunTest(const FString& Parameters)
{
    FGenesisRuntimeRecipeDefinition Expensive;
    Expensive.Id = TEXT("recipe:expensive");
    Expensive.Cost = FGenesisFixedPoint::FromWhole(20);
    FGenesisRuntimeRecipeDefinition Cheap;
    Cheap.Id = TEXT("recipe:cheap");
    Cheap.Cost = FGenesisFixedPoint::FromWhole(10);
    const TArray<FGenesisRuntimeRecipeDefinition> Recipes{Expensive, Cheap};
    const FGenesisRuntimeRecipeDefinition* Selected = FGenesisRecipeSelector::Select(Recipes, EGenesisRecipeSelectionMode::LowestCost);
    TestNotNull(TEXT("A recipe is selected"), Selected);
    TestEqual(TEXT("Lowest cost recipe wins"), Selected->Id, Cheap.Id);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisPlacementParityTest,
    "Genesis.Simulation.Construction.PlacementValidationParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisPlacementParityTest::RunTest(const FString& Parameters)
{
    FGenesisPlacementRequest Request;
    Request.BuildingId = TEXT("building:factory");
    Request.TerrainSlopeDegrees = 18.0f;
    Request.bCollisionFree = false;
    const TArray<FGenesisPlacementViolation> Preview = FGenesisPlacementService::Validate(Request, 10.0f);
    const TArray<FGenesisPlacementViolation> Commit = FGenesisPlacementService::Validate(Request, 10.0f);
    TestEqual(TEXT("Preview and commit use identical validation"), Preview.Num(), Commit.Num());
    TestTrue(TEXT("At least one violation is reported"), Preview.Num() > 0);
    if (!Preview.IsEmpty() && !Commit.IsEmpty())
    {
        TestEqual(TEXT("First violation is stable"), Preview[0].Code, Commit[0].Code);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisStateHashTest,
    "Genesis.Simulation.Verification.StableStateHash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisStateHashTest::RunTest(const FString& Parameters)
{
    FGenesisCanonicalEntityState First;
    First.EntityId = FGuid(1, 2, 3, 4);
    First.Type = TEXT("inventory");
    First.IntegerFields.Add(TEXT("quantity"), 1000);
    FGenesisCanonicalEntityState Second;
    Second.EntityId = FGuid(5, 6, 7, 8);
    Second.Type = TEXT("machine");
    Second.StringFields.Add(TEXT("state"), TEXT("running"));
    TestEqual(
        TEXT("Entity order does not affect hash"),
        FGenesisStateVerifier::Hash({First, Second}),
        FGenesisStateVerifier::Hash({Second, First}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisPerformanceBudgetTest,
    "Genesis.Simulation.Verification.PerformanceBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisPerformanceBudgetTest::RunTest(const FString& Parameters)
{
    FGenesisPerformanceBudget Budget;
    Budget.MedianMilliseconds = 2.0;
    Budget.P95Milliseconds = 3.0;
    Budget.MaximumMilliseconds = 4.0;
    Budget.MaximumBytes = 1024;
    FString Reason;
    TestTrue(TEXT("Measurements inside budget pass"), FGenesisPerformanceGate::Passes(Budget, {1.0, 1.5, 2.5}, 512, Reason));
    TestFalse(TEXT("Memory regression fails"), FGenesisPerformanceGate::Passes(Budget, {1.0, 1.5, 2.5}, 2048, Reason));
    return true;
}

#endif
