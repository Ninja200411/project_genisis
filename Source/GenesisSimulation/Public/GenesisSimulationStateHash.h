#pragma once

#include "CoreMinimal.h"
#include "GenesisSimulationTypes.h"

class GENESISSIMULATION_API FGenesisSimulationStateHash final
{
public:
    static uint64 Compute(const FGenesisSimulationClockState& ClockState, const FGenesisSchedulerState& SchedulerState, uint64 DomainState = 0);

private:
    static void AddBytes(uint64& Hash, const void* Data, int32 Size);
    static void AddName(uint64& Hash, FName Value);
};