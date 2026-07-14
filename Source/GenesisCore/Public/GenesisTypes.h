#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.generated.h"

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisEntityId
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    uint64 Value = 0;

    bool IsValid() const { return Value != 0; }

    friend bool operator==(const FGenesisEntityId& Left, const FGenesisEntityId& Right)
    {
        return Left.Value == Right.Value;
    }
};

FORCEINLINE uint32 GetTypeHash(const FGenesisEntityId& EntityId)
{
    return GetTypeHash(EntityId.Value);
}
