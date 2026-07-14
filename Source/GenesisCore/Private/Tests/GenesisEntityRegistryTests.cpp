#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisComponentStore.h"
#include "GenesisEntityRegistry.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FTestComponent
{
    int32 Value = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEntityRegistryLifecycleTest,
    "Genesis.Core.Entities.LifecycleAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEntityRegistryLifecycleTest::RunTest(const FString& Parameters)
{
    FGenesisEntityRegistry Registry;
    TGenesisComponentStore<FTestComponent> Store(TEXT("Test.Components"));
    TestTrue(TEXT("Store registers"), Registry.RegisterComponentStore(Store));

    const FGenesisEntityId First = Registry.QueueCreate();
    const FGenesisEntityId Second = Registry.QueueCreate();
    TestFalse(TEXT("Queued entity is not visible before phase commit"), Registry.IsAlive(First));
    Registry.ApplyPendingChanges();

    TestTrue(TEXT("First entity is alive"), Registry.IsAlive(First));
    TestTrue(TEXT("Second entity is alive"), Registry.IsAlive(Second));
    TestTrue(TEXT("Component can be added"), Store.Add(First, FTestComponent{42}));

    TestTrue(TEXT("Destroy is queued"), Registry.QueueDestroy(First));
    TestFalse(TEXT("Tombstoned entity is unavailable immediately"), Registry.IsAlive(First));
    TestTrue(TEXT("Component exists until phase commit"), Store.Contains(First));
    Registry.ApplyPendingChanges();

    TestFalse(TEXT("Destroyed entity is removed"), Registry.IsAlive(First));
    TestFalse(TEXT("Destroyed entity is removed from component stores"), Store.Contains(First));
    TestTrue(TEXT("Other entity remains"), Registry.IsAlive(Second));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEntityRegistrySaveLoadTest,
    "Genesis.Core.Entities.PersistentSequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEntityRegistrySaveLoadTest::RunTest(const FString& Parameters)
{
    FGenesisEntityRegistry Source;
    const FGenesisEntityId First = Source.QueueCreate();
    const FGenesisEntityId Second = Source.QueueCreate();
    Source.ApplyPendingChanges();

    const FGenesisEntityRegistrySnapshot Snapshot = Source.CaptureSnapshot();
    FGenesisEntityRegistry Restored;
    TestTrue(TEXT("Snapshot restores"), Restored.RestoreSnapshot(Snapshot));

    const FGenesisEntityId Third = Restored.QueueCreate();
    TestTrue(TEXT("ID sequence continues after load"), Third.Value > Second.Value);
    TestTrue(TEXT("IDs are never reused"), Third != First && Third != Second);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisComponentStoreOrderTest,
    "Genesis.Core.Entities.DeterministicComponentIteration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisComponentStoreOrderTest::RunTest(const FString& Parameters)
{
    TGenesisComponentStore<FTestComponent> Store(TEXT("Test.Order"));
    Store.Add(FGenesisEntityId(3), FTestComponent{3});
    Store.Add(FGenesisEntityId(1), FTestComponent{1});
    Store.Add(FGenesisEntityId(2), FTestComponent{2});

    TArray<int64> Order;
    Store.ForEach([&Order](const FGenesisEntityId EntityId, const FTestComponent&)
    {
        Order.Add(EntityId.Value);
    });

    TestEqual(TEXT("Three components iterate"), Order.Num(), 3);
    TestEqual(TEXT("First ID is stable"), Order[0], int64{1});
    TestEqual(TEXT("Second ID is stable"), Order[1], int64{2});
    TestEqual(TEXT("Third ID is stable"), Order[2], int64{3});
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEntityRegistryDiagnosticsTest,
    "Genesis.Core.Entities.Diagnostics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEntityRegistryDiagnosticsTest::RunTest(const FString& Parameters)
{
    FGenesisEntityRegistry Registry;
    TGenesisComponentStore<FTestComponent> Store(TEXT("Test.Diagnostics"));
    Registry.RegisterComponentStore(Store);
    Store.Add(FGenesisEntityId(1), FTestComponent{7});

    const TArray<FGenesisComponentStoreDiagnostics> Diagnostics = Registry.GetDiagnostics();
    TestEqual(TEXT("One store is reported"), Diagnostics.Num(), 1);
    TestEqual(TEXT("Component count is reported"), Diagnostics[0].ComponentCount, 1);
    TestTrue(TEXT("Allocated memory is reported"), Diagnostics[0].AllocatedBytes > 0);
    return true;
}

#endif
