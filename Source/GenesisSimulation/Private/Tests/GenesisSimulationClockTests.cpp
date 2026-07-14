#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisSimulationClock.h"
#include "GenesisSimulationStateHash.h"
#include "GenesisTickScheduler.h"
#include "HAL/PlatformProcess.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSimulationClockPauseAndStepTest,
    "Genesis.Simulation.Clock.PauseAndSingleStep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSimulationClockPauseAndStepTest::RunTest(const FString& Parameters)
{
    FGenesisSimulationClock Clock;
    Clock.Pause();
    TestEqual(TEXT("Paused clock consumes no realtime ticks"), Clock.ConsumeDueTicks(1.0, 100), 0);
    TestTrue(TEXT("Single step succeeds while paused"), Clock.ConsumeSingleStep());
    TestEqual(TEXT("Single step advances exactly one tick"), Clock.GetCurrentTick(), int64{1});
    Clock.Resume();
    TestEqual(TEXT("100 ms advances one normal-speed tick"), Clock.ConsumeDueTicks(0.1, 100), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSimulationClockSnapshotTest,
    "Genesis.Simulation.Clock.VersionedSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSimulationClockSnapshotTest::RunTest(const FString& Parameters)
{
    FGenesisSimulationClock Source(50000);
    Source.SetSpeed(EGenesisSimulationSpeed::Fast);
    Source.ConsumeDueTicks(0.125, 100);

    const FGenesisSimulationClockState State = Source.CaptureState();
    FGenesisSimulationClock Restored;
    TestTrue(TEXT("Versioned clock state restores"), Restored.RestoreState(State));
    TestEqual(TEXT("Tick is preserved"), Restored.GetCurrentTick(), Source.GetCurrentTick());
    TestEqual(TEXT("Tick duration is preserved"), Restored.GetTickDurationMicroseconds(), int64{50000});
    TestEqual(TEXT("Pending simulation time is preserved"), Restored.GetPendingSimulationSeconds(), Source.GetPendingSimulationSeconds());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisTickSchedulerStableOrderTest,
    "Genesis.Simulation.Scheduler.StableExecutionOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisTickSchedulerStableOrderTest::RunTest(const FString& Parameters)
{
    FGenesisTickScheduler Scheduler;
    TArray<FName> ExecutionOrder;

    auto Register = [&Scheduler, &ExecutionOrder](const FName Id, const EGenesisSimulationPhase Phase, const int32 Priority)
    {
        FGenesisScheduledSystem System;
        System.SystemId = Id;
        System.Phase = Phase;
        System.Priority = Priority;
        System.BudgetMilliseconds = 100.0;
        System.Execute = [&ExecutionOrder, Id](const int64) { ExecutionOrder.Add(Id); };
        return Scheduler.RegisterSystem(MoveTemp(System));
    };

    TestTrue(TEXT("Register simulation B"), Register(TEXT("System.B"), EGenesisSimulationPhase::Simulation, 10));
    TestTrue(TEXT("Register input"), Register(TEXT("System.Input"), EGenesisSimulationPhase::Input, 50));
    TestTrue(TEXT("Register simulation A"), Register(TEXT("System.A"), EGenesisSimulationPhase::Simulation, 10));
    TestFalse(TEXT("Duplicate system IDs are rejected"), Register(TEXT("System.A"), EGenesisSimulationPhase::Events, 0));

    Scheduler.ExecuteTick(1);
    TestEqual(TEXT("Input phase executes first"), ExecutionOrder[0], FName(TEXT("System.Input")));
    TestEqual(TEXT("Equal-priority systems use stable IDs"), ExecutionOrder[1], FName(TEXT("System.A")));
    TestEqual(TEXT("Second stable ID follows"), ExecutionOrder[2], FName(TEXT("System.B")));
    TestTrue(TEXT("Captured scheduler state matches runtime plan"), Scheduler.ValidateState(Scheduler.CaptureState()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisTickSchedulerBudgetTelemetryTest,
    "Genesis.Simulation.Scheduler.PerSystemBudgetTelemetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisTickSchedulerBudgetTelemetryTest::RunTest(const FString& Parameters)
{
    FGenesisTickScheduler Scheduler;
    FGenesisScheduledSystem System;
    System.SystemId = TEXT("System.Slow");
    System.BudgetMilliseconds = 0.0;
    System.Execute = [](const int64) { FPlatformProcess::SleepNoStats(0.001f); };
    TestTrue(TEXT("Slow system registers"), Scheduler.RegisterSystem(MoveTemp(System)));

    const FGenesisTickMetrics Metrics = Scheduler.ExecuteTick(1);
    TestEqual(TEXT("One system metric is emitted"), Metrics.Systems.Num(), 1);
    TestTrue(TEXT("Budget overrun is attributed to the system"), Metrics.Systems[0].bBudgetExceeded);
    TestEqual(TEXT("Per-system overrun counter increments"), Metrics.Systems[0].TotalBudgetOverruns, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSimulationDeterminismTenThousandTicksTest,
    "Genesis.Simulation.Determinism.TenThousandTicks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSimulationDeterminismTenThousandTicksTest::RunTest(const FString& Parameters)
{
    auto Run = []()
    {
        FGenesisSimulationClock Clock;
        FGenesisTickScheduler Scheduler;
        uint64 DomainState = 0x9E3779B97F4A7C15ull;

        FGenesisScheduledSystem Input;
        Input.SystemId = TEXT("Core.Input");
        Input.Phase = EGenesisSimulationPhase::Input;
        Input.Execute = [&DomainState](const int64 Tick) { DomainState ^= static_cast<uint64>(Tick) + 0x517CC1B727220A95ull; };
        Scheduler.RegisterSystem(MoveTemp(Input));

        FGenesisScheduledSystem Simulation;
        Simulation.SystemId = TEXT("Core.Simulation");
        Simulation.Phase = EGenesisSimulationPhase::Simulation;
        Simulation.Execute = [&DomainState](const int64 Tick) { DomainState = (DomainState * 1099511628211ull) ^ static_cast<uint64>(Tick); };
        Scheduler.RegisterSystem(MoveTemp(Simulation));

        Clock.Pause();
        for (int32 Index = 0; Index < 10000; ++Index)
        {
            Clock.ConsumeSingleStep();
            Scheduler.ExecuteTick(Clock.GetCurrentTick());
        }

        return FGenesisSimulationStateHash::Compute(Clock.CaptureState(), Scheduler.CaptureState(), DomainState);
    };

    const uint64 FirstHash = Run();
    const uint64 SecondHash = Run();
    TestEqual(TEXT("Identical inputs produce identical state hashes after 10,000 ticks"), FirstHash, SecondHash);
    return true;
}

#endif