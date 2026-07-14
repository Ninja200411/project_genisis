#pragma once

#include "CoreMinimal.h"
#include "GenesisSimulationClock.h"
#include "GenesisTickScheduler.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GenesisSimulationSubsystem.generated.h"

UCLASS()
class GENESISSIMULATION_API UGenesisSimulationSubsystem final : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category = "Genesis|Simulation")
    int64 GetCurrentTick() const { return Clock.GetCurrentTick(); }

    UFUNCTION(BlueprintPure, Category = "Genesis|Simulation")
    EGenesisSimulationSpeed GetSimulationSpeed() const { return Clock.GetSpeed(); }

    UFUNCTION(BlueprintPure, Category = "Genesis|Simulation")
    FGenesisTickMetrics GetLastTickMetrics() const { return LastTickMetrics; }

    UFUNCTION(BlueprintCallable, Category = "Genesis|Simulation")
    void PauseSimulation();

    UFUNCTION(BlueprintCallable, Category = "Genesis|Simulation")
    void ResumeSimulation();

    UFUNCTION(BlueprintCallable, Category = "Genesis|Simulation")
    void SetSimulationSpeed(EGenesisSimulationSpeed Speed);

    UFUNCTION(BlueprintCallable, Category = "Genesis|Simulation")
    bool AdvanceOneTick();

    FGenesisTickScheduler& GetScheduler() { return Scheduler; }
    const FGenesisTickScheduler& GetScheduler() const { return Scheduler; }

private:
    void ExecuteDueTicks(int32 TickCount);

    static constexpr int32 MaxTicksPerFrame = 16;

    FGenesisSimulationClock Clock;
    FGenesisTickScheduler Scheduler;
    FGenesisTickMetrics LastTickMetrics;
    bool bInitialized = false;
};
