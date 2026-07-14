#pragma once

#include "CoreMinimal.h"
#include "GenesisSimulationTypes.generated.h"

UENUM(BlueprintType)
enum class EGenesisSimulationSpeed : uint8
{
    Paused UMETA(DisplayName = "Paused"),
    Normal UMETA(DisplayName = "1x"),
    Fast UMETA(DisplayName = "2x"),
    VeryFast UMETA(DisplayName = "4x")
};

UENUM(BlueprintType)
enum class EGenesisSimulationPhase : uint8
{
    Input,
    Simulation,
    Resolution,
    Events,
    ReadModels
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisTickMetrics
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int64 Tick = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    double TickDurationMilliseconds = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int32 BudgetOverrunCount = 0;
};
