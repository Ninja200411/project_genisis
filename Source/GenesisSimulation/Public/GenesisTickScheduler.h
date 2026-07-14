#pragma once

#include "CoreMinimal.h"
#include "GenesisSimulationTypes.h"

struct GENESISSIMULATION_API FGenesisScheduledSystem
{
    FName SystemId;
    EGenesisSimulationPhase Phase = EGenesisSimulationPhase::Simulation;
    int32 Priority = 0;
    double BudgetMilliseconds = 1.0;
    TFunction<void(int64)> Execute;
};

class GENESISSIMULATION_API FGenesisTickScheduler final
{
public:
    bool RegisterSystem(FGenesisScheduledSystem System);
    bool UnregisterSystem(FName SystemId);
    void Clear();

    FGenesisTickMetrics ExecuteTick(int64 TickNumber);
    const TArray<FGenesisScheduledSystem>& GetExecutionPlan() const { return ExecutionPlan; }

private:
    void SortExecutionPlan();

    TArray<FGenesisScheduledSystem> ExecutionPlan;
    int32 TotalBudgetOverrunCount = 0;
};
