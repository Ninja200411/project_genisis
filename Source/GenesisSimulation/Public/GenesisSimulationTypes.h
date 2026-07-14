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
struct GENESISSIMULATION_API FGenesisSimulationClockState
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int64 CurrentTick = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int64 TickDurationMicroseconds = 100000;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int64 PendingSimulationMicroseconds = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    EGenesisSimulationSpeed Speed = EGenesisSimulationSpeed::Normal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    EGenesisSimulationSpeed ResumeSpeed = EGenesisSimulationSpeed::Normal;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisScheduledSystemConfig
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    FName SystemId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    EGenesisSimulationPhase Phase = EGenesisSimulationPhase::Simulation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int32 Priority = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    double BudgetMilliseconds = 1.0;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisSchedulerState
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    TArray<FGenesisScheduledSystemConfig> Systems;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisSystemTickMetrics
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    FName SystemId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    double DurationMilliseconds = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    double BudgetMilliseconds = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    bool bBudgetExceeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int32 TotalBudgetOverruns = 0;
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    TArray<FGenesisSystemTickMetrics> Systems;
};