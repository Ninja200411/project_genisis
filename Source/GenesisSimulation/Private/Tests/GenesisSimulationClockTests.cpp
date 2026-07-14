#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisSimulationClock.h"
#include "GenesisTickScheduler.h"
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
    TestEqual(TEXT("Paused clock remains at tick zero"), Clock.GetCurrentTick(), int64{0});

    TestTrue(TEXT("Single step succeeds while paused"), Clock.ConsumeSingleStep());
    TestEqual(TEXT("Single step advances exactly one tick"), Clock.GetCurrentTick(), int64{1});

    Clock.Resume();
    TestEqual(TEXT("100 ms advances one normal-speed tick"), Clock.ConsumeDueTicks(0.1, 100), 1);
    TestEqual(TEXT("Clock reaches tick two"), Clock.GetCurrentTick(), int64{2});
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSimulationClockSpeedTest,
    "Genesis.Simulation.Clock.SpeedMultipliers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSimulationClockSpeedTest::RunTest(const FString& Parameters)
{
    FGenesisSimulationClock Clock;
    Clock.SetSpeed(EGenesisSimulationSpeed::VeryFast);

    TestEqual(TEXT("100 ms at 4x advances four ticks"), Clock.ConsumeDueTicks(0.1, 100), 4);
    TestEqual(TEXT("Clock stores all four ticks"), Clock.GetCurrentTick(), int64{4});
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
        System.Execute = [&ExecutionOrder, Id](const int64)
        {
            ExecutionOrder.Add(Id);
        };
        return Scheduler.RegisterSystem(MoveTemp(System));
    };

    TestTrue(TEXT("Register simulation B"), Register(TEXT("System.B"), EGenesisSimulationPhase::Simulation, 10));
    TestTrue(TEXT("Register input"), Register(TEXT("System.Input"), EGenesisSimulationPhase::Input, 50));
    TestTrue(TEXT("Register simulation A"), Register(TEXT("System.A"), EGenesisSimulationPhase::Simulation, 10));
    TestFalse(TEXT("Duplicate system IDs are rejected"), Register(TEXT("System.A"), EGenesisSimulationPhase::Events, 0));

    Scheduler.ExecuteTick(1);

    TestEqual(TEXT("Three systems execute"), ExecutionOrder.Num(), 3);
    TestEqual(TEXT("Input phase executes first"), ExecutionOrder[0], FName(TEXT("System.Input")));
    TestEqual(TEXT("Equal-priority systems use stable IDs"), ExecutionOrder[1], FName(TEXT("System.A")));
    TestEqual(TEXT("Second stable ID follows"), ExecutionOrder[2], FName(TEXT("System.B")));
    return true;
}

#endif
