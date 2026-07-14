#include "GenesisSimulationClock.h"

FGenesisSimulationClock::FGenesisSimulationClock(const int64 InTickDurationMicroseconds)
    : TickDurationMicroseconds(FMath::Max<int64>(1, InTickDurationMicroseconds))
{
}

int32 FGenesisSimulationClock::ConsumeDueTicks(const double RealDeltaSeconds, const int32 MaxTicksToConsume)
{
    if (IsPaused() || RealDeltaSeconds <= 0.0 || MaxTicksToConsume <= 0)
    {
        return 0;
    }

    const int64 RealDeltaMicroseconds = FMath::Max<int64>(0, FMath::RoundToInt64(RealDeltaSeconds * 1000000.0));
    AccumulatedSimulationMicroseconds += RealDeltaMicroseconds * GetSpeedMultiplier(Speed);

    const int64 DueTicks = AccumulatedSimulationMicroseconds / TickDurationMicroseconds;
    const int32 ConsumedTicks = static_cast<int32>(FMath::Min<int64>(DueTicks, MaxTicksToConsume));

    AccumulatedSimulationMicroseconds -= static_cast<int64>(ConsumedTicks) * TickDurationMicroseconds;
    CurrentTick += ConsumedTicks;
    return ConsumedTicks;
}

bool FGenesisSimulationClock::ConsumeSingleStep()
{
    if (!IsPaused())
    {
        return false;
    }

    ++CurrentTick;
    return true;
}

void FGenesisSimulationClock::Reset()
{
    CurrentTick = 0;
    AccumulatedSimulationMicroseconds = 0;
    Speed = EGenesisSimulationSpeed::Normal;
    ResumeSpeed = EGenesisSimulationSpeed::Normal;
}

void FGenesisSimulationClock::Pause()
{
    if (!IsPaused())
    {
        ResumeSpeed = Speed;
        Speed = EGenesisSimulationSpeed::Paused;
    }
}

void FGenesisSimulationClock::Resume()
{
    if (IsPaused())
    {
        Speed = ResumeSpeed == EGenesisSimulationSpeed::Paused ? EGenesisSimulationSpeed::Normal : ResumeSpeed;
    }
}

void FGenesisSimulationClock::SetSpeed(const EGenesisSimulationSpeed InSpeed)
{
    Speed = InSpeed;
    if (InSpeed != EGenesisSimulationSpeed::Paused)
    {
        ResumeSpeed = InSpeed;
    }
}

double FGenesisSimulationClock::GetPendingSimulationSeconds() const
{
    return static_cast<double>(AccumulatedSimulationMicroseconds) / 1000000.0;
}

int32 FGenesisSimulationClock::GetSpeedMultiplier(const EGenesisSimulationSpeed InSpeed)
{
    switch (InSpeed)
    {
    case EGenesisSimulationSpeed::Fast:
        return 2;
    case EGenesisSimulationSpeed::VeryFast:
        return 4;
    case EGenesisSimulationSpeed::Normal:
        return 1;
    case EGenesisSimulationSpeed::Paused:
    default:
        return 0;
    }
}
