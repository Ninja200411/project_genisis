#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisSimulationClock.h"
#include "GenesisSimulationStateHash.h"
#include "GenesisTickScheduler.h"
#include "HAL/PlatformProcess.h"
#include "Misc/AutomationTest.h"

namespace GenesisSimulationPersistenceTests
{
bool RegisterSystem(FGenesisTickScheduler& Scheduler, const FName Id, const EGenesisSimulationPhase Phase, const int32 Priority, TFunction<void(int64)> Execute)
{
    FGenesisScheduledSystem System;
    System.SystemId = Id;
    System.Phase = Phase;
    System.Priority = Priority;
    System.BudgetMilliseconds = 100.0;
    System.Execute = MoveTemp(Execute);
    return Scheduler.RegisterSystem(MoveTemp(System));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSchedulerSaveLoadOrderTest,
    "Genesis.Simulation.Persistence.ExecutionOrderAfterSaveLoad",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSchedulerSaveLoadOrderTest::RunTest(const FString& Parameters)
{
    using namespace GenesisSimulationPersistenceTests;

    FGenesisTickScheduler Source;
    RegisterSystem(Source, TEXT("System.B"), EGenesisSimulationPhase::Simulation, 10, [](const int64) {});
    RegisterSystem(Source, TEXT("System.Input"), EGenesisSimulationPhase::Input, 50, [](const int64) {});
    RegisterSystem(Source, TEXT("System.A"), EGenesisSimulationPhase::Simulation, 10, [](const int64) {});
    const FGenesisSchedulerState SavedState = Source.CaptureState();

    TArray<FName> RestoredOrder;
    FGenesisTickScheduler Restored;
    RegisterSystem(Restored, TEXT("System.A"), EGenesisSimulationPhase::Simulation, 10,
        [&RestoredOrder](const int64) { RestoredOrder.Add(TEXT("System.A")); });
    RegisterSystem(Restored, TEXT("System.B"), EGenesisSimulationPhase::Simulation, 10,
        [&RestoredOrder](const int64) { RestoredOrder.Add(TEXT("System.B")); });
    RegisterSystem(Restored, TEXT("System.Input"), EGenesisSimulationPhase::Input, 50,
        [&RestoredOrder](const int64) { RestoredOrder.Add(TEXT("System.Input")); });

    TestTrue(TEXT("Restored runtime plan matches saved versioned plan"), Restored.ValidateState(SavedState));
    Restored.ExecuteTick(1);
    TestEqual(TEXT("Input remains first after load"), RestoredOrder[0], FName(TEXT("System.Input")));
    TestEqual(TEXT("Stable ID A remains second after load"), RestoredOrder[1], FName(TEXT("System.A")));
    TestEqual(TEXT("Stable ID B remains third after load"), RestoredOrder[2], FName(TEXT("System.B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSimulationOverloadIndependenceTest,
    "Genesis.Simulation.Determinism.OverloadChangesWallTimeOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSimulationOverloadIndependenceTest::RunTest(const FString& Parameters)
{
    auto Run = [](const bool bSimulateOverload)
    {
        FGenesisSimulationClock Clock;
        FGenesisTickScheduler Scheduler;
        uint64 DomainState = 17;

        GenesisSimulationPersistenceTests::RegisterSystem(
            Scheduler,
            TEXT("System.State"),
            EGenesisSimulationPhase::Simulation,
            0,
            [&DomainState, bSimulateOverload](const int64 Tick)
            {
                if (bSimulateOverload)
                {
                    FPlatformProcess::SleepNoStats(0.0001f);
                }
                DomainState = DomainState * 31ull + static_cast<uint64>(Tick);
            });

        Clock.Pause();
        for (int32 Index = 0; Index < 256; ++Index)
        {
            Clock.ConsumeSingleStep();
            Scheduler.ExecuteTick(Clock.GetCurrentTick());
        }

        return FGenesisSimulationStateHash::Compute(Clock.CaptureState(), Scheduler.CaptureState(), DomainState);
    };

    TestEqual(TEXT("Added wall time does not change the authoritative state hash"), Run(false), Run(true));
    return true;
}

#endif