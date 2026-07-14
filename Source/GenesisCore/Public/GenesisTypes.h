#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.generated.h"

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisEntityId
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int64 Value = 0;

    explicit FGenesisEntityId(const int64 InValue = 0)
        : Value(InValue)
    {
    }

    bool IsValid() const { return Value > 0; }

    friend bool operator==(const FGenesisEntityId& Left, const FGenesisEntityId& Right)
    {
        return Left.Value == Right.Value;
    }

    friend bool operator!=(const FGenesisEntityId& Left, const FGenesisEntityId& Right)
    {
        return !(Left == Right);
    }

    friend bool operator<(const FGenesisEntityId& Left, const FGenesisEntityId& Right)
    {
        return Left.Value < Right.Value;
    }
};

FORCEINLINE uint32 GetTypeHash(const FGenesisEntityId& EntityId)
{
    return GetTypeHash(EntityId.Value);
}
