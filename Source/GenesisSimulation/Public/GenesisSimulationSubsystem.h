#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandBus.h"
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

    FGenesisCommandResult SubmitCommand(FGenesisCommandEnvelope Command);
    bool RegisterCommandHandler(FGenesisCommandHandler Handler);
    void AddReadModelConsumer(TFunction<void(const TArray<FGenesisDomainEvent>&)> Consumer);

    FGenesisTickScheduler& GetScheduler() { return Scheduler; }
    const FGenesisTickScheduler& GetScheduler() const { return Scheduler; }
    const FGenesisEventJournal& GetEventJournal() const { return EventJournal; }

private:
    void RegisterCoreSystems();
    void ExecuteDueTicks(int32 TickCount);
    void ProcessCommands(int64 TickNumber);
    void PublishReadModels(int64 TickNumber);

    static constexpr int32 MaxTicksPerFrame = 16;

    FGenesisSimulationClock Clock;
    FGenesisTickScheduler Scheduler;
    FGenesisCommandBus CommandBus;
    FGenesisEventJournal EventJournal;
    TArray<TFunction<void(const TArray<FGenesisDomainEvent>&)>> ReadModelConsumers;
    FGenesisTickMetrics LastTickMetrics;
    bool bInitialized = false;
};
