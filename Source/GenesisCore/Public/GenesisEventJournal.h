#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandTypes.h"

class GENESISCORE_API FGenesisEventJournal final
{
public:
    void Reset();
    void Append(FGenesisDomainEvent Event);

    const TArray<FGenesisDomainEvent>& GetAll() const { return Events; }
    TArray<FGenesisDomainEvent> FindByTick(int64 Tick) const;
    TArray<FGenesisDomainEvent> FindByEntity(FGenesisEntityId EntityId) const;
    TArray<FGenesisDomainEvent> FindByCommand(int64 CommandId) const;

    bool ReplayToTick(int64 InclusiveTick, const TFunctionRef<bool(const FGenesisDomainEvent&)>& Consumer) const;

private:
    TArray<FGenesisDomainEvent> Events;
    int64 NextSequence = 1;
};
