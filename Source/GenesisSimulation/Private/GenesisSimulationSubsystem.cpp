#include "GenesisSimulationSubsystem.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenesisSimulation, Log, All);

void UGenesisSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentTick = 0;
    UE_LOG(LogGenesisSimulation, Log, TEXT("Genesis simulation initialized with fixed step %.3f s"), FixedStepSeconds);
}

void UGenesisSimulationSubsystem::Deinitialize()
{
    UE_LOG(LogGenesisSimulation, Log, TEXT("Genesis simulation stopped at tick %lld"), CurrentTick);
    Super::Deinitialize();
}

void UGenesisSimulationSubsystem::AdvanceOneTick()
{
    ++CurrentTick;
}
