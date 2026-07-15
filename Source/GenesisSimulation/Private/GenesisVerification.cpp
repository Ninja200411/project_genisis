#include "GenesisVerification.h"

#include "Misc/SecureHash.h"

void FGenesisSaveMigrationRegistry::Register(TSharedRef<const IGenesisSaveMigration> Migration)
{
    BySourceVersion.Add(Migration->FromVersion(), MoveTemp(Migration));
}

bool FGenesisSaveMigrationRegistry::Migrate(FGenesisSaveEnvelope& Save, const int32 TargetVersion, FString& OutError) const
{
    if (Save.SchemaVersion > TargetVersion)
    {
        OutError = FString::Printf(TEXT("Save schema %d is newer than supported target %d."), Save.SchemaVersion, TargetVersion);
        return false;
    }

    while (Save.SchemaVersion < TargetVersion)
    {
        const TSharedRef<const IGenesisSaveMigration>* Migration = BySourceVersion.Find(Save.SchemaVersion);
        if (Migration == nullptr)
        {
            OutError = FString::Printf(TEXT("Missing migration from schema %d."), Save.SchemaVersion);
            return false;
        }
        const int32 PreviousVersion = Save.SchemaVersion;
        if (!(*Migration)->Apply(Save, OutError)) return false;
        if (Save.SchemaVersion != (*Migration)->ToVersion() || Save.SchemaVersion <= PreviousVersion)
        {
            OutError = TEXT("Migration did not advance to its declared target version.");
            return false;
        }
    }
    return true;
}

FString FGenesisStateVerifier::Hash(const TArray<FGenesisCanonicalEntityState>& Entities)
{
    TArray<FGenesisCanonicalEntityState> Ordered = Entities;
    Ordered.Sort([](const FGenesisCanonicalEntityState& A, const FGenesisCanonicalEntityState& B)
    {
        return A.EntityId < B.EntityId;
    });

    FString Canonical;
    for (const FGenesisCanonicalEntityState& Entity : Ordered)
    {
        Canonical += Entity.EntityId.ToString(EGuidFormats::DigitsWithHyphensLower) + TEXT("|") + Entity.Type.ToString() + TEXT("|");

        TArray<FName> IntegerKeys;
        Entity.IntegerFields.GetKeys(IntegerKeys);
        IntegerKeys.Sort(FNameLexicalLess());
        for (const FName Key : IntegerKeys)
        {
            Canonical += FString::Printf(TEXT("i:%s=%lld;"), *Key.ToString(), Entity.IntegerFields[Key]);
        }

        TArray<FName> StringKeys;
        Entity.StringFields.GetKeys(StringKeys);
        StringKeys.Sort(FNameLexicalLess());
        for (const FName Key : StringKeys)
        {
            Canonical += FString::Printf(TEXT("s:%s=%s;"), *Key.ToString(), *Entity.StringFields[Key]);
        }
        Canonical += TEXT("\n");
    }

    FTCHARToUTF8 Utf8(*Canonical);
    uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Digest);
    return BytesToHex(Digest, FSHA1::DigestSize).ToLower();
}

TArray<FGenesisVerificationDifference> FGenesisStateVerifier::Diff(
    const TArray<FGenesisCanonicalEntityState>& Expected,
    const TArray<FGenesisCanonicalEntityState>& Actual,
    const int64 Tick)
{
    TMap<FGuid, const FGenesisCanonicalEntityState*> ActualById;
    for (const FGenesisCanonicalEntityState& Entity : Actual) ActualById.Add(Entity.EntityId, &Entity);

    TArray<FGenesisVerificationDifference> Result;
    for (const FGenesisCanonicalEntityState& ExpectedEntity : Expected)
    {
        const FGenesisCanonicalEntityState* const* ActualEntity = ActualById.Find(ExpectedEntity.EntityId);
        if (ActualEntity == nullptr)
        {
            Result.Add({Tick, ExpectedEntity.EntityId, TEXT("entity"), TEXT("present"), TEXT("missing")});
            continue;
        }
        for (const TPair<FName, int64>& Pair : ExpectedEntity.IntegerFields)
        {
            const int64* Value = (*ActualEntity)->IntegerFields.Find(Pair.Key);
            if (Value == nullptr || *Value != Pair.Value)
            {
                Result.Add({Tick, ExpectedEntity.EntityId, Pair.Key, FString::Printf(TEXT("%lld"), Pair.Value), Value ? FString::Printf(TEXT("%lld"), *Value) : TEXT("missing")});
            }
        }
        for (const TPair<FName, FString>& Pair : ExpectedEntity.StringFields)
        {
            const FString* Value = (*ActualEntity)->StringFields.Find(Pair.Key);
            if (Value == nullptr || *Value != Pair.Value)
            {
                Result.Add({Tick, ExpectedEntity.EntityId, Pair.Key, Pair.Value, Value ? *Value : TEXT("missing")});
            }
        }
    }
    Result.Sort([](const FGenesisVerificationDifference& A, const FGenesisVerificationDifference& B)
    {
        if (A.EntityId != B.EntityId) return A.EntityId < B.EntityId;
        return A.Field.LexicalLess(B.Field);
    });
    return Result;
}

bool FGenesisPerformanceGate::Passes(
    const FGenesisPerformanceBudget& Budget,
    const TArray<double>& Measurements,
    const int64 UsedBytes,
    FString& OutReason)
{
    if (Measurements.IsEmpty())
    {
        OutReason = TEXT("No measurements were supplied.");
        return false;
    }

    TArray<double> Ordered = Measurements;
    Ordered.Sort();
    const double Median = Ordered[Ordered.Num() / 2];
    const int32 P95Index = FMath::Clamp(FMath::CeilToInt(Ordered.Num() * 0.95) - 1, 0, Ordered.Num() - 1);
    const double P95 = Ordered[P95Index];
    const double Maximum = Ordered.Last();

    if (Median > Budget.MedianMilliseconds || P95 > Budget.P95Milliseconds || Maximum > Budget.MaximumMilliseconds)
    {
        OutReason = FString::Printf(TEXT("Timing budget exceeded: median %.3f, p95 %.3f, max %.3f ms."), Median, P95, Maximum);
        return false;
    }
    if (UsedBytes > Budget.MaximumBytes)
    {
        OutReason = FString::Printf(TEXT("Memory budget exceeded: %lld > %lld bytes."), UsedBytes, Budget.MaximumBytes);
        return false;
    }
    OutReason.Reset();
    return true;
}
