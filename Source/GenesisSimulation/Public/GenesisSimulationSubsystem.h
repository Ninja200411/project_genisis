#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisSimulationSubsystem.generated.h"

UCLASS()
class GENESISSIMULATION_API UGenesisSimulationSubsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "Genesis|Simulation")
    int64 GetCurrentTick() const { return CurrentTick; }

    void AdvanceOneTick();

private:
    static constexpr double FixedStepSeconds = 0.1;
    int64 CurrentTick = 0;
};
