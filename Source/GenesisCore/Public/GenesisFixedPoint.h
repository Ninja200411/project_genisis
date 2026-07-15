#pragma once

#include "CoreMinimal.h"
#include "GenesisFixedPoint.generated.h"

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisFixedPoint
{
    GENERATED_BODY()

    static constexpr int64 Scale = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Simulation")
    int64 Raw = 0;

    static FGenesisFixedPoint FromRaw(const int64 Value)
    {
        FGenesisFixedPoint Result;
        Result.Raw = Value;
        return Result;
    }

    static FGenesisFixedPoint FromWhole(const int64 Value) { return FromRaw(Value * Scale); }

    double ToDouble() const { return static_cast<double>(Raw) / static_cast<double>(Scale); }
    bool IsNegative() const { return Raw < 0; }

    friend bool operator==(const FGenesisFixedPoint&, const FGenesisFixedPoint&) = default;
    friend bool operator<(const FGenesisFixedPoint& Left, const FGenesisFixedPoint& Right) { return Left.Raw < Right.Raw; }
    friend FGenesisFixedPoint operator+(const FGenesisFixedPoint& Left, const FGenesisFixedPoint& Right) { return FromRaw(Left.Raw + Right.Raw); }
    friend FGenesisFixedPoint operator-(const FGenesisFixedPoint& Left, const FGenesisFixedPoint& Right) { return FromRaw(Left.Raw - Right.Raw); }
};
