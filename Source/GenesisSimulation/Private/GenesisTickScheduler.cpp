#include "GenesisTickScheduler.h"

#include "HAL/PlatformTime.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenesisTickScheduler, Log, All);

bool FGenesisTickScheduler::RegisterSystem(FGenesisScheduledSystem System)
{
    if (System.SystemId.IsNone() || !System.Execute)
    {
        return false;
    }

    if (ExecutionPlan.ContainsByPredicate([&System](const FGenesisScheduledSystem& Existing)
        {
            return Existing.SystemId == System.SystemId;
        }))
    {
        return false;
    }

    System.BudgetMilliseconds = FMath::Max(0.0, System.BudgetMilliseconds);
    ExecutionPlan.Add(MoveTemp(System));
    SortExecutionPlan();
    return true;
}

bool FGenesisTickScheduler::UnregisterSystem(const FName SystemId)
{
    return ExecutionPlan.RemoveAll([SystemId](const FGenesisScheduledSystem& System)
    {
        return System.SystemId == SystemId;
    }) > 0;
}

void FGenesisTickScheduler::Clear()
{
    ExecutionPlan.Reset();
    TotalBudgetOverrunCount = 0;
}

FGenesisTickMetrics FGenesisTickScheduler::ExecuteTick(const int64 TickNumber)
{
    FGenesisTickMetrics Metrics;
    Metrics.Tick = TickNumber;

    const double TickStartSeconds = FPlatformTime::Seconds();

    for (const FGenesisScheduledSystem& System : ExecutionPlan)
    {
        const double SystemStartSeconds = FPlatformTime::Seconds();
        System.Execute(TickNumber);
        const double SystemDurationMilliseconds = (FPlatformTime::Seconds() - SystemStartSeconds) * 1000.0;

        if (SystemDurationMilliseconds > System.BudgetMilliseconds)
        {
            ++TotalBudgetOverrunCount;
            UE_LOG(
                LogGenesisTickScheduler,
                Warning,
                TEXT("Simulation system '%s' exceeded its %.3f ms budget with %.3f ms at tick %lld"),
                *System.SystemId.ToString(),
                System.BudgetMilliseconds,
                SystemDurationMilliseconds,
                TickNumber);
        }
    }

    Metrics.TickDurationMilliseconds = (FPlatformTime::Seconds() - TickStartSeconds) * 1000.0;
    Metrics.BudgetOverrunCount = TotalBudgetOverrunCount;
    return Metrics;
}

void FGenesisTickScheduler::SortExecutionPlan()
{
    ExecutionPlan.Sort([](const FGenesisScheduledSystem& Left, const FGenesisScheduledSystem& Right)
    {
        if (Left.Phase != Right.Phase)
        {
            return static_cast<uint8>(Left.Phase) < static_cast<uint8>(Right.Phase);
        }

        if (Left.Priority != Right.Priority)
        {
            return Left.Priority < Right.Priority;
        }

        return Left.SystemId.LexicalLess(Right.SystemId);
    });
}
