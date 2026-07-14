#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandTypes.h"

class GENESISCORE_API FGenesisEventSerializer final
{
public:
    static constexpr uint32 Magic = 0x47455654;
    static constexpr int32 CurrentContainerVersion = 1;

    static bool Serialize(const FGenesisDomainEvent& Event, TArray<uint8>& OutBytes);
    static bool Deserialize(const TArray<uint8>& Bytes, FGenesisDomainEvent& OutEvent);
};
