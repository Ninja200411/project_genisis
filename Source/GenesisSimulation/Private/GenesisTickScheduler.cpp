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
    BudgetOverrunsBySystem.Remove(SystemId);
    return ExecutionPlan.RemoveAll([SystemId](const FGenesisScheduledSystem& System)
    {
        return System.SystemId == SystemId;
    }) > 0;
}

void FGenesisTickScheduler::Clear()
{
    ExecutionPlan.Reset();
    BudgetOverrunsBySystem.Reset();
    TotalBudgetOverrunCount = 0;
}

FGenesisTickMetrics FGenesisTickScheduler::ExecuteTick(const int64 TickNumber)
{
    FGenesisTickMetrics Metrics;
    Metrics.Tick = TickNumber;
    Metrics.Systems.Reserve(ExecutionPlan.Num());

    const double TickStartSeconds = FPlatformTime::Seconds();

    for (const FGenesisScheduledSystem& System : ExecutionPlan)
    {
        const double SystemStartSeconds = FPlatformTime::Seconds();
        System.Execute(TickNumber);
        const double DurationMilliseconds = (FPlatformTime::Seconds() - SystemStartSeconds) * 1000.0;
        const bool bExceeded = DurationMilliseconds > System.BudgetMilliseconds;

        if (bExceeded)
        {
            ++TotalBudgetOverrunCount;
            BudgetOverrunsBySystem.FindOrAdd(System.SystemId)++;
            UE_LOG(LogGenesisTickScheduler, Warning,
                TEXT("Simulation system '%s' exceeded its %.3f ms budget with %.3f ms at tick %lld"),
                *System.SystemId.ToString(), System.BudgetMilliseconds, DurationMilliseconds, TickNumber);
        }

        FGenesisSystemTickMetrics& SystemMetrics = Metrics.Systems.AddDefaulted_GetRef();
        SystemMetrics.SystemId = System.SystemId;
        SystemMetrics.DurationMilliseconds = DurationMilliseconds;
        SystemMetrics.BudgetMilliseconds = System.BudgetMilliseconds;
        SystemMetrics.bBudgetExceeded = bExceeded;
        SystemMetrics.TotalBudgetOverruns = BudgetOverrunsBySystem.FindRef(System.SystemId);
    }

    Metrics.TickDurationMilliseconds = (FPlatformTime::Seconds() - TickStartSeconds) * 1000.0;
    Metrics.BudgetOverrunCount = TotalBudgetOverrunCount;
    return Metrics;
}

FGenesisSchedulerState FGenesisTickScheduler::CaptureState() const
{
    FGenesisSchedulerState State;
    State.Systems.Reserve(ExecutionPlan.Num());
    for (const FGenesisScheduledSystem& System : ExecutionPlan)
    {
        FGenesisScheduledSystemConfig& Config = State.Systems.AddDefaulted_GetRef();
        Config.SystemId = System.SystemId;
        Config.Phase = System.Phase;
        Config.Priority = System.Priority;
        Config.BudgetMilliseconds = System.BudgetMilliseconds;
    }
    return State;
}

bool FGenesisTickScheduler::ValidateState(const FGenesisSchedulerState& State) const
{
    if (State.SchemaVersion != FGenesisSchedulerState::CurrentSchemaVersion || State.Systems.Num() != ExecutionPlan.Num())
    {
        return false;
    }

    for (int32 Index = 0; Index < ExecutionPlan.Num(); ++Index)
    {
        const FGenesisScheduledSystem& Runtime = ExecutionPlan[Index];
        const FGenesisScheduledSystemConfig& Saved = State.Systems[Index];
        if (Runtime.SystemId != Saved.SystemId || Runtime.Phase != Saved.Phase || Runtime.Priority != Saved.Priority ||
            !FMath::IsNearlyEqual(Runtime.BudgetMilliseconds, Saved.BudgetMilliseconds))
        {
            return false;
        }
    }
    return true;
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