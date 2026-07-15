#include "GenesisResourceStack.h"

namespace
{
int32 Weighted(const int32 Left, const int64 LeftWeight, const int32 Right, const int64 RightWeight)
{
    const int64 Total = LeftWeight + RightWeight;
    return Total > 0 ? static_cast<int32>((static_cast<int64>(Left) * LeftWeight + static_cast<int64>(Right) * RightWeight) / Total) : 0;
}
}

bool FGenesisResourceStack::IsValid() const
{
    return !ResourceId.IsNone() && Quantity.Raw >= 0 && BatchId.IsValid() &&
        Quality.GradePermille >= 0 && Quality.GradePermille <= 1000 &&
        Quality.ContaminationPermille >= 0 && Quality.ContaminationPermille <= 1000;
}

bool FGenesisResourceStack::CanMerge(
    const FGenesisResourceStack& Other,
    const int32 GradeTolerancePermille,
    const int32 TemperatureToleranceMilliCelsius,
    const int32 ContaminationTolerancePermille) const
{
    return ResourceId == Other.ResourceId &&
        FMath::Abs(Quality.GradePermille - Other.Quality.GradePermille) <= GradeTolerancePermille &&
        FMath::Abs(Quality.TemperatureMilliCelsius - Other.Quality.TemperatureMilliCelsius) <= TemperatureToleranceMilliCelsius &&
        FMath::Abs(Quality.ContaminationPermille - Other.Quality.ContaminationPermille) <= ContaminationTolerancePermille;
}

bool FGenesisResourceStack::Split(const FGenesisFixedPoint SplitQuantity, FGenesisResourceStack& OutSplit)
{
    if (SplitQuantity.Raw <= 0 || SplitQuantity.Raw > Quantity.Raw)
    {
        return false;
    }

    OutSplit = *this;
    OutSplit.Quantity = SplitQuantity;
    OutSplit.BatchId = FGuid::NewGuid();
    Quantity.Raw -= SplitQuantity.Raw;
    return true;
}

bool FGenesisResourceStack::Merge(const FGenesisResourceStack& Other)
{
    if (Other.Quantity.Raw <= 0 || !CanMerge(Other, 0, 0, 0))
    {
        return false;
    }

    const int64 PreviousQuantity = Quantity.Raw;
    Quality.GradePermille = Weighted(Quality.GradePermille, PreviousQuantity, Other.Quality.GradePermille, Other.Quantity.Raw);
    Quality.TemperatureMilliCelsius = Weighted(Quality.TemperatureMilliCelsius, PreviousQuantity, Other.Quality.TemperatureMilliCelsius, Other.Quantity.Raw);
    Quality.ContaminationPermille = Weighted(Quality.ContaminationPermille, PreviousQuantity, Other.Quality.ContaminationPermille, Other.Quantity.Raw);
    Quantity.Raw += Other.Quantity.Raw;

    for (const TPair<FName, int32>& Pair : Other.OriginPermille)
    {
        const int32 Existing = OriginPermille.FindRef(Pair.Key);
        OriginPermille.Add(Pair.Key, Weighted(Existing, PreviousQuantity, Pair.Value, Other.Quantity.Raw));
    }
    return true;
}

FGenesisFixedPoint FGenesisBalanceJournal::Sum(const FName ResourceId) const
{
    int64 Raw = 0;
    for (const FGenesisBalanceEntry& Entry : Entries)
    {
        if (Entry.ResourceId == ResourceId)
        {
            Raw += Entry.Delta.Raw;
        }
    }
    return FGenesisFixedPoint::FromRaw(Raw);
}
