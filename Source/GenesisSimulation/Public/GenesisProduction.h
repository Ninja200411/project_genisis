#pragma once

#include "CoreMinimal.h"
#include "GenesisFixedPoint.h"
#include "GenesisInventory.h"
#include "GenesisProduction.generated.h"

UENUM(BlueprintType)
enum class EGenesisProductionState : uint8
{
    Idle,
    WaitingForInputs,
    Reserved,
    Running,
    Paused,
    Completing,
    OutputBlocked,
    Completed,
    Faulted
};

UENUM(BlueprintType)
enum class EGenesisRecipeSelectionMode : uint8
{
    Manual,
    LowestCost,
    LowestEnergy,
    HighestYield,
    HighestQuality,
    LowestEmissions,
    HighestRecycling
};

UENUM(BlueprintType)
enum class EGenesisMachineCondition : uint8
{
    Healthy,
    Degraded,
    Contaminated,
    Faulted,
    Cleaning,
    Repairing
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisRecipeAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ResourceId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Quantity;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisRecipeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName AlternativeGroup;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 DurationTicks = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGenesisRecipeAmount> Inputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGenesisRecipeAmount> Outputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Energy;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Cost;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Emissions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 QualityPermille = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RecyclingPermille = 0;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisProductionRuntime
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisProductionState State = EGenesisProductionState::Idle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisMachineCondition Condition = EGenesisMachineCondition::Healthy;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ActiveRecipeId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 StartedTick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 ProgressTicks = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 WearPermille = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContaminationPermille = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName BlockReason;
};

class GENESISSIMULATION_API FGenesisRecipeSelector
{
public:
    static const FGenesisRecipeDefinition* Select(
        const TArray<FGenesisRecipeDefinition>& Candidates,
        EGenesisRecipeSelectionMode Mode,
        FName ManualId = NAME_None);
};

class GENESISSIMULATION_API FGenesisRecipeProcessor
{
public:
    bool Start(const FGenesisRecipeDefinition& Recipe, FGenesisInventory& Input, FGenesisInventory& Output, int64 Tick);
    void Advance(const FGenesisRecipeDefinition& Recipe, FGenesisInventory& Input, FGenesisInventory& Output, int64 Tick);
    void Pause(FName Reason);
    void Resume();
    void ApplyMaintenance(bool bCleaning, int32 RepairPermille);
    const FGenesisProductionRuntime& GetRuntime() const { return Runtime; }

private:
    FGenesisProductionRuntime Runtime;
    TArray<FGuid> InputReservations;
};
