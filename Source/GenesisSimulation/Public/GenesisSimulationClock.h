#pragma once

#include "CoreMinimal.h"
#include "GenesisSimulationTypes.h"

class GENESISSIMULATION_API FGenesisSimulationClock final
{
public:
    static constexpr int64 DefaultTickDurationMicroseconds = 100000;

    explicit FGenesisSimulationClock(int64 InTickDurationMicroseconds = DefaultTickDurationMicroseconds);

    int32 ConsumeDueTicks(double RealDeltaSeconds, int32 MaxTicksToConsume);
    bool ConsumeSingleStep();

    void Reset();
    void Pause();
    void Resume();
    void SetSpeed(EGenesisSimulationSpeed InSpeed);

    int64 GetCurrentTick() const { return CurrentTick; }
    int64 GetTickDurationMicroseconds() const { return TickDurationMicroseconds; }
    EGenesisSimulationSpeed GetSpeed() const { return Speed; }
    bool IsPaused() const { return Speed == EGenesisSimulationSpeed::Paused; }
    double GetPendingSimulationSeconds() const;

private:
    static int32 GetSpeedMultiplier(EGenesisSimulationSpeed InSpeed);

    int64 CurrentTick = 0;
    int64 TickDurationMicroseconds = DefaultTickDurationMicroseconds;
    int64 AccumulatedSimulationMicroseconds = 0;
    EGenesisSimulationSpeed Speed = EGenesisSimulationSpeed::Normal;
    EGenesisSimulationSpeed ResumeSpeed = EGenesisSimulationSpeed::Normal;
};
