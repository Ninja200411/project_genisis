#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisCommandBus.h"
#include "GenesisTickScheduler.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisCommandPipelinePhaseOrderTest,
    "Genesis.Simulation.Commands.PhaseOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisCommandPipelinePhaseOrderTest::RunTest(const FString& Parameters)
{
    FGenesisCommandBus Bus;
    FGenesisEventJournal Journal;
    FGenesisTickScheduler Scheduler;
    TArray<FName> ObservedOrder;

    FGenesisCommandHandler Handler;
    Handler.CommandType = TEXT("Test.Command");
    Handler.Validate = [](const FGenesisCommandEnvelope&)
    {
        return FGenesisCommandResult::Accepted();
    };
    Handler.Execute = [&ObservedOrder](const FGenesisCommandEnvelope&, TArray<FGenesisDomainEvent>& Events)
    {
        ObservedOrder.Add(TEXT("Command"));
        FGenesisDomainEvent Event;
        Event.EventType = TEXT("Test.Event");
        Events.Add(MoveTemp(Event));
        return FGenesisCommandResult::Accepted();
    };
    TestTrue(TEXT("Handler registers"), Bus.RegisterHandler(MoveTemp(Handler)));

    FGenesisCommandEnvelope Command;
    Command.CommandId = 1;
    Command.TargetTick = 1;
    Command.Source = TEXT("Test");
    Command.CommandType = TEXT("Test.Command");
    TestTrue(TEXT("Command enqueues"), Bus.Enqueue(MoveTemp(Command)).bAccepted);

    FGenesisScheduledSystem Input;
    Input.SystemId = TEXT("Test.Commands");
    Input.Phase = EGenesisSimulationPhase::Input;
    Input.Execute = [&Bus, &Journal](const int64 Tick)
    {
        Bus.ProcessTick(Tick, Journal);
    };
    TestTrue(TEXT("Input system registers"), Scheduler.RegisterSystem(MoveTemp(Input)));

    FGenesisScheduledSystem Simulation;
    Simulation.SystemId = TEXT("Test.Simulation");
    Simulation.Phase = EGenesisSimulationPhase::Simulation;
    Simulation.Execute = [&ObservedOrder, &Journal](const int64 Tick)
    {
        ObservedOrder.Add(TEXT("Simulation"));
        check(Journal.FindByTick(Tick).Num() == 1);
    };
    TestTrue(TEXT("Simulation system registers"), Scheduler.RegisterSystem(MoveTemp(Simulation)));

    FGenesisScheduledSystem ReadModels;
    ReadModels.SystemId = TEXT("Test.ReadModels");
    ReadModels.Phase = EGenesisSimulationPhase::ReadModels;
    ReadModels.Execute = [&ObservedOrder, &Journal](const int64 Tick)
    {
        check(Journal.FindByTick(Tick).Num() == 1);
        ObservedOrder.Add(TEXT("ReadModels"));
    };
    TestTrue(TEXT("Read model system registers"), Scheduler.RegisterSystem(MoveTemp(ReadModels)));

    Scheduler.ExecuteTick(1);

    TestEqual(TEXT("Three stages execute"), ObservedOrder.Num(), 3);
    TestEqual(TEXT("Command executes in input phase"), ObservedOrder[0], FName(TEXT("Command")));
    TestEqual(TEXT("Simulation follows input"), ObservedOrder[1], FName(TEXT("Simulation")));
    TestEqual(TEXT("Read models publish last"), ObservedOrder[2], FName(TEXT("ReadModels")));
    return true;
}

#endif
