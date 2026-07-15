#include "GenesisProduction.h"

namespace
{
int64 Score(const FGenesisRuntimeRecipeDefinition& Recipe, const EGenesisRecipeSelectionMode Mode)
{
    switch (Mode)
    {
    case EGenesisRecipeSelectionMode::LowestCost: return Recipe.Cost.Raw;
    case EGenesisRecipeSelectionMode::LowestEnergy: return Recipe.Energy.Raw;
    case EGenesisRecipeSelectionMode::HighestYield:
    {
        int64 Yield = 0;
        for (const FGenesisRecipeAmount& Output : Recipe.Outputs) Yield += Output.Quantity.Raw;
        return -Yield;
    }
    case EGenesisRecipeSelectionMode::HighestQuality: return -Recipe.QualityPermille;
    case EGenesisRecipeSelectionMode::LowestEmissions: return Recipe.Emissions.Raw;
    case EGenesisRecipeSelectionMode::HighestRecycling: return -Recipe.RecyclingPermille;
    default: return Recipe.Priority;
    }
}
}

const FGenesisRuntimeRecipeDefinition* FGenesisRecipeSelector::Select(
    const TArray<FGenesisRuntimeRecipeDefinition>& Candidates,
    const EGenesisRecipeSelectionMode Mode,
    const FName ManualId)
{
    const FGenesisRuntimeRecipeDefinition* Best = nullptr;
    for (const FGenesisRuntimeRecipeDefinition& Candidate : Candidates)
    {
        if (Mode == EGenesisRecipeSelectionMode::Manual && Candidate.Id != ManualId) continue;
        if (Best == nullptr || Score(Candidate, Mode) < Score(*Best, Mode) ||
            (Score(Candidate, Mode) == Score(*Best, Mode) && Candidate.Id.LexicalLess(Best->Id)))
        {
            Best = &Candidate;
        }
    }
    return Best;
}

bool FGenesisRecipeProcessor::Start(
    const FGenesisRuntimeRecipeDefinition& Recipe,
    FGenesisInventory& Input,
    FGenesisInventory& Output,
    const int64 Tick)
{
    if (Runtime.State == EGenesisProductionState::Running || Runtime.Condition == EGenesisMachineCondition::Faulted)
    {
        return false;
    }

    FGenesisInventoryTransaction Transaction;
    InputReservations.Reset();
    for (const FGenesisRecipeAmount& Amount : Recipe.Inputs)
    {
        FGenesisReservation Reservation;
        Reservation.ReservationId = FGuid::NewGuid();
        Reservation.OwnerId = FGuid::NewGuid();
        Reservation.ResourceId = Amount.ResourceId;
        Reservation.Quantity = Amount.Quantity;
        Reservation.Purpose = EGenesisReservationPurpose::Input;
        Reservation.ExpiryTick = Tick + FMath::Max<int64>(Recipe.DurationTicks, 1) + 1;
        if (!Transaction.Stage(Input, Reservation))
        {
            Transaction.Rollback();
            Runtime.State = EGenesisProductionState::WaitingForInputs;
            Runtime.BlockReason = TEXT("inputs.unavailable");
            return false;
        }
        InputReservations.Add(Reservation.ReservationId);
    }

    if (!Transaction.Commit())
    {
        Runtime.State = EGenesisProductionState::WaitingForInputs;
        Runtime.BlockReason = TEXT("reservation.commit_failed");
        return false;
    }

    Runtime.State = EGenesisProductionState::Running;
    Runtime.ActiveRecipeId = Recipe.Id;
    Runtime.StartedTick = Tick;
    Runtime.ProgressTicks = 0;
    Runtime.BlockReason = NAME_None;
    return true;
}

void FGenesisRecipeProcessor::Advance(
    const FGenesisRuntimeRecipeDefinition& Recipe,
    FGenesisInventory& Input,
    FGenesisInventory& Output,
    const int64 Tick)
{
    if (Runtime.State != EGenesisProductionState::Running || Runtime.ActiveRecipeId != Recipe.Id) return;
    ++Runtime.ProgressTicks;
    Runtime.WearPermille = FMath::Clamp(Runtime.WearPermille + 1, 0, 1000);
    Runtime.ContaminationPermille = FMath::Clamp(Runtime.ContaminationPermille + Recipe.RecyclingPermille / 1000, 0, 1000);

    if (Runtime.ProgressTicks < FMath::Max<int64>(Recipe.DurationTicks, 1)) return;

    for (const FGenesisRecipeAmount& OutputAmount : Recipe.Outputs)
    {
        FGenesisResourceStack Stack;
        Stack.ResourceId = OutputAmount.ResourceId;
        Stack.Quantity = OutputAmount.Quantity;
        Stack.BatchId = FGuid::NewGuid();
        Stack.Quality.GradePermille = Recipe.QualityPermille;
        if (!Output.Add(Stack, FGenesisFixedPoint::FromWhole(1), FGenesisFixedPoint::FromWhole(1), TEXT("general")))
        {
            Runtime.State = EGenesisProductionState::OutputBlocked;
            Runtime.BlockReason = TEXT("output.capacity");
            return;
        }
    }

    for (const FGuid ReservationId : InputReservations) Input.CommitReservation(ReservationId);
    Runtime.State = EGenesisProductionState::Completed;
    Runtime.BlockReason = NAME_None;
    if (Runtime.WearPermille >= 1000) Runtime.Condition = EGenesisMachineCondition::Faulted;
    else if (Runtime.WearPermille >= 700) Runtime.Condition = EGenesisMachineCondition::Degraded;
}

void FGenesisRecipeProcessor::Pause(const FName Reason)
{
    if (Runtime.State == EGenesisProductionState::Running)
    {
        Runtime.State = EGenesisProductionState::Paused;
        Runtime.BlockReason = Reason;
    }
}

void FGenesisRecipeProcessor::Resume()
{
    if (Runtime.State == EGenesisProductionState::Paused)
    {
        Runtime.State = EGenesisProductionState::Running;
        Runtime.BlockReason = NAME_None;
    }
}

void FGenesisRecipeProcessor::ApplyMaintenance(const bool bCleaning, const int32 RepairPermille)
{
    Runtime.Condition = bCleaning ? EGenesisMachineCondition::Cleaning : EGenesisMachineCondition::Repairing;
    if (bCleaning) Runtime.ContaminationPermille = 0;
    Runtime.WearPermille = FMath::Clamp(Runtime.WearPermille - FMath::Max(RepairPermille, 0), 0, 1000);
    Runtime.Condition = Runtime.WearPermille >= 700 ? EGenesisMachineCondition::Degraded : EGenesisMachineCondition::Healthy;
    if (Runtime.State == EGenesisProductionState::Faulted) Runtime.State = EGenesisProductionState::Idle;
}
