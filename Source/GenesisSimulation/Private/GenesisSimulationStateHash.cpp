#include "GenesisSimulationStateHash.h"

namespace
{
constexpr uint64 FnvOffset = 14695981039346656037ull;
constexpr uint64 FnvPrime = 1099511628211ull;
}

uint64 FGenesisSimulationStateHash::Compute(
    const FGenesisSimulationClockState& ClockState,
    const FGenesisSchedulerState& SchedulerState,
    const uint64 DomainState)
{
    uint64 Hash = FnvOffset;

    AddBytes(Hash, &ClockState.SchemaVersion, sizeof(ClockState.SchemaVersion));
    AddBytes(Hash, &ClockState.CurrentTick, sizeof(ClockState.CurrentTick));
    AddBytes(Hash, &ClockState.TickDurationMicroseconds, sizeof(ClockState.TickDurationMicroseconds));
    AddBytes(Hash, &ClockState.PendingSimulationMicroseconds, sizeof(ClockState.PendingSimulationMicroseconds));

    const uint8 Speed = static_cast<uint8>(ClockState.Speed);
    const uint8 ResumeSpeed = static_cast<uint8>(ClockState.ResumeSpeed);
    AddBytes(Hash, &Speed, sizeof(Speed));
    AddBytes(Hash, &ResumeSpeed, sizeof(ResumeSpeed));

    AddBytes(Hash, &SchedulerState.SchemaVersion, sizeof(SchedulerState.SchemaVersion));
    for (const FGenesisScheduledSystemConfig& System : SchedulerState.Systems)
    {
        AddName(Hash, System.SystemId);
        const uint8 Phase = static_cast<uint8>(System.Phase);
        AddBytes(Hash, &Phase, sizeof(Phase));
        AddBytes(Hash, &System.Priority, sizeof(System.Priority));
        AddBytes(Hash, &System.BudgetMilliseconds, sizeof(System.BudgetMilliseconds));
    }

    AddBytes(Hash, &DomainState, sizeof(DomainState));
    return Hash;
}

void FGenesisSimulationStateHash::AddBytes(uint64& Hash, const void* Data, const int32 Size)
{
    const uint8* Bytes = static_cast<const uint8*>(Data);
    for (int32 Index = 0; Index < Size; ++Index)
    {
        Hash ^= Bytes[Index];
        Hash *= FnvPrime;
    }
}

void FGenesisSimulationStateHash::AddName(uint64& Hash, const FName Value)
{
    const FTCHARToUTF8 Utf8(*Value.ToString());
    AddBytes(Hash, Utf8.Get(), Utf8.Length());
    const uint8 Terminator = 0;
    AddBytes(Hash, &Terminator, sizeof(Terminator));
}