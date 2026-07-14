#include "GenesisSimulationSubsystem.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenesisSimulation, Log, All);

void UGenesisSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Clock.Reset();
    Scheduler.Clear();
    LastTickMetrics = FGenesisTickMetrics{};
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
