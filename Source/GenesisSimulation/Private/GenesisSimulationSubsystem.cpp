#include "GenesisSimulationSubsystem.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenesisSimulation, Log, All);

void UGenesisSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Clock.Reset();
    Scheduler.Clear();
    CommandBus.Reset();
    EventJournal.Reset();
    ReadModelConsumers.Reset();
    LastTickMetrics = FGenesisTickMetrics{};
    RegisterCoreSystems();
    bInitialized = true;

    UE_LOG(
        LogGenesisSimulation,
        Log,
        TEXT("Genesis simulation initialized with fixed step %lld us"),
        Clock.GetTickDurationMicroseconds());
}

void UGenesisSimulationSubsystem::Deinitialize()
{
    bInitialized = false;
    ReadModelConsumers.Reset();
    CommandBus.Reset();
    EventJournal.Reset();
    Scheduler.Clear();

    UE_LOG(LogGenesisSimulation, Log, TEXT("Genesis simulation stopped at tick %lld"), Clock.GetCurrentTick());
    Super::Deinitialize();
}

void UGenesisSimulationSubsystem::Tick(const float DeltaTime)
{
    ExecuteDueTicks(Clock.ConsumeDueTicks(DeltaTime, MaxTicksPerFrame));
}

bool UGenesisSimulationSubsystem::IsTickable() const
{
    return bInitialized && !IsTemplate();
}

TStatId UGenesisSimulationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGenesisSimulationSubsystem, STATGROUP_Tickables);
}

void UGenesisSimulationSubsystem::PauseSimulation()
{
    Clock.Pause();
}

void UGenesisSimulationSubsystem::ResumeSimulation()
{
    Clock.Resume();
}

void UGenesisSimulationSubsystem::SetSimulationSpeed(const EGenesisSimulationSpeed Speed)
{
    Clock.SetSpeed(Speed);
}

bool UGenesisSimulationSubsystem::AdvanceOneTick()
{
    if (!Clock.ConsumeSingleStep())
    {
        return false;
    }

    LastTickMetrics = Scheduler.ExecuteTick(Clock.GetCurrentTick());
    return true;
}

FGenesisCommandResult UGenesisSimulationSubsystem::SubmitCommand(FGenesisCommandEnvelope Command)
{
    return CommandBus.Enqueue(MoveTemp(Command));
}

bool UGenesisSimulationSubsystem::RegisterCommandHandler(FGenesisCommandHandler Handler)
{
    return CommandBus.RegisterHandler(MoveTemp(Handler));
}

void UGenesisSimulationSubsystem::AddReadModelConsumer(TFunction<void(const TArray<FGenesisDomainEvent>&)> Consumer)
{
    if (Consumer)
    {
        ReadModelConsumers.Add(MoveTemp(Consumer));
    }
}

void UGenesisSimulationSubsystem::RegisterCoreSystems()
{
    FGenesisScheduledSystem CommandSystem;
    CommandSystem.SystemId = TEXT("Genesis.Core.Commands");
    CommandSystem.Phase = EGenesisSimulationPhase::Input;
    CommandSystem.Priority = 0;
    CommandSystem.BudgetMilliseconds = 1.0;
    CommandSystem.Execute = [this](const int64 TickNumber)
    {
        ProcessCommands(TickNumber);
    };
    check(Scheduler.RegisterSystem(MoveTemp(CommandSystem)));

    FGenesisScheduledSystem ReadModelSystem;
    ReadModelSystem.SystemId = TEXT("Genesis.Core.ReadModels");
    ReadModelSystem.Phase = EGenesisSimulationPhase::ReadModels;
    ReadModelSystem.Priority = MAX_int32;
    ReadModelSystem.BudgetMilliseconds = 1.0;
    ReadModelSystem.Execute = [this](const int64 TickNumber)
    {
        PublishReadModels(TickNumber);
    };
    check(Scheduler.RegisterSystem(MoveTemp(ReadModelSystem)));
}

void UGenesisSimulationSubsystem::ExecuteDueTicks(const int32 TickCount)
{
    if (TickCount <= 0)
    {
        return;
    }

    const int64 FirstTick = Clock.GetCurrentTick() - TickCount + 1;
    for (int32 TickOffset = 0; TickOffset < TickCount; ++TickOffset)
    {
        LastTickMetrics = Scheduler.ExecuteTick(FirstTick + TickOffset);
    }
}

void UGenesisSimulationSubsystem::ProcessCommands(const int64 TickNumber)
{
    CommandBus.ProcessTick(TickNumber, EventJournal);
}

void UGenesisSimulationSubsystem::PublishReadModels(const int64 TickNumber)
{
    const TArray<FGenesisDomainEvent> Events = EventJournal.FindByTick(TickNumber);
    if (Events.IsEmpty())
    {
        return;
    }

    for (const TFunction<void(const TArray<FGenesisDomainEvent>&)>& Consumer : ReadModelConsumers)
    {
        Consumer(Events);
    }
}
