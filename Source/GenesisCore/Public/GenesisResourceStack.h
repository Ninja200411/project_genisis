#pragma once

#include "CoreMinimal.h"
#include "GenesisFixedPoint.h"
#include "GenesisResourceStack.generated.h"

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisQualityVector
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GradePermille = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TemperatureMilliCelsius = 20000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ContaminationPermille = 0;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisResourceStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ResourceId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Quantity;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid BatchId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisQualityVector Quality;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, int32> OriginPermille;

    bool IsValid() const;
    bool CanMerge(const FGenesisResourceStack& Other, int32 GradeTolerancePermille, int32 TemperatureToleranceMilliCelsius, int32 ContaminationTolerancePermille) const;
    bool Split(FGenesisFixedPoint SplitQuantity, FGenesisResourceStack& OutSplit);
    bool Merge(const FGenesisResourceStack& Other);
};

UENUM(BlueprintType)
enum class EGenesisBalanceReason : uint8
{
    Produced,
    Consumed,
    Lost,
    Converted,
    Transported
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisBalanceEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ResourceId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGenesisFixedPoint Delta;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisBalanceReason Reason = EGenesisBalanceReason::Produced;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid CauseId;
};

class GENESISCORE_API FGenesisBalanceJournal
{
public:
    void Record(const FGenesisBalanceEntry& Entry) { Entries.Add(Entry); }
    const TArray<FGenesisBalanceEntry>& GetEntries() const { return Entries; }
    FGenesisFixedPoint Sum(FName ResourceId) const;

private:
    TArray<FGenesisBalanceEntry> Entries;
};
