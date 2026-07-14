#include "GenesisContentRegistry.h"

#include "Misc/SecureHash.h"

namespace
{
int32 TierRank(const EGenesisContentSourceTier Tier)
{
    return static_cast<int32>(Tier);
}

void SortUnique(TArray<FName>& Values)
{
    Values.Sort(FNameLexicalLess());
    Values.SetNum(Algo::Unique(Values));
}

void AddIndexValues(TMap<FName, TArray<FName>>& Index, const TArray<FName>& Keys, const FName DefinitionId)
{
    for (const FName Key : Keys)
    {
        if (!Key.IsNone())
        {
            Index.FindOrAdd(Key).Add(DefinitionId);
        }
    }
}

FGenesisContentDiagnostic Diagnostic(
    const FName Code,
    const FGenesisContentRecord& Record,
    const FString& FieldPath,
    const FName Rule,
    const FString& Message)
{
    FGenesisContentDiagnostic Result;
    Result.Code = Code;
    Result.DefinitionId = Record.Id;
    Result.File = Record.Source.NormalizedPath;
    Result.FieldPath = FieldPath;
    Result.Rule = Rule;
    Result.Message = Message;
    return Result;
}
}

const FGenesisContentRecord* FGenesisContentRegistrySnapshot::Find(const FName Id) const
{
    const int32* Index = ById.Find(Id);
    return Index != nullptr && Definitions.IsValidIndex(*Index) ? &Definitions[*Index] : nullptr;
}

bool FGenesisContentRegistry::Reload(
    const TArray<FGenesisContentRecord>& Candidates,
    TArray<FGenesisContentDiagnostic>& OutDiagnostics)
{
    TSharedPtr<FGenesisContentRegistrySnapshot> CandidateSnapshot;
    if (!BuildSnapshot(Candidates, CandidateSnapshot, OutDiagnostics))
    {
        return false;
    }

    PublishedSnapshot = MoveTemp(CandidateSnapshot);
    return true;
}

bool FGenesisContentRegistry::BuildSnapshot(
    const TArray<FGenesisContentRecord>& Candidates,
    TSharedPtr<FGenesisContentRegistrySnapshot>& OutSnapshot,
    TArray<FGenesisContentDiagnostic>& OutDiagnostics)
{
    OutDiagnostics.Reset();
    TArray<FGenesisContentRecord> Ordered = Candidates;
    Ordered.Sort([](const FGenesisContentRecord& Left, const FGenesisContentRecord& Right)
    {
        if (Left.Source.Tier != Right.Source.Tier)
        {
            return TierRank(Left.Source.Tier) < TierRank(Right.Source.Tier);
        }
        const int32 PathCompare = Left.Source.NormalizedPath.Compare(Right.Source.NormalizedPath, ESearchCase::IgnoreCase);
        if (PathCompare != 0)
        {
            return PathCompare < 0;
        }
        return Left.Id.LexicalLess(Right.Id);
    });

    TArray<FGenesisContentRecord> Resolved;
    TMap<FName, int32> ResolvedById;
    for (FGenesisContentRecord& Record : Ordered)
    {
        Record.Source.NormalizedPath.ReplaceInline(TEXT("\\"), TEXT("/"));
        Record.Source.NormalizedPath.ToLowerInline();
        SortUnique(Record.References);
        SortUnique(Record.Produces);
        SortUnique(Record.Consumes);
        SortUnique(Record.Unlocks);
        SortUnique(Record.Facilities);

        if (Record.Id.IsNone() || Record.SchemaMajor <= 0 || Record.Source.PackageId.IsNone() ||
            Record.Source.NormalizedPath.IsEmpty() || Record.CanonicalData.IsEmpty())
        {
            OutDiagnostics.Add(Diagnostic(
                TEXT("GEN-CONTENT-INVALID-RECORD"), Record, TEXT("$"), TEXT("record.required"),
                TEXT("Definition ID, schema, source and canonical data are required.")));
            continue;
        }

        int32* ExistingIndex = ResolvedById.Find(Record.Id);
        if (ExistingIndex == nullptr)
        {
            ResolvedById.Add(Record.Id, Resolved.Add(MoveTemp(Record)));
            continue;
        }

        const FGenesisContentRecord& Existing = Resolved[*ExistingIndex];
        const bool bValidOverride = Record.Replaces == Existing.Id &&
            TierRank(Record.Source.Tier) > TierRank(Existing.Source.Tier) &&
            Record.Type == Existing.Type && Record.SchemaMajor == Existing.SchemaMajor;
        if (!bValidOverride)
        {
            OutDiagnostics.Add(Diagnostic(
                TEXT("GEN-CONTENT-OVERRIDE-CONFLICT"), Record, TEXT("replaces"), TEXT("override.compatible"),
                FString::Printf(TEXT("Definition conflicts with existing source '%s'."), *Existing.Source.NormalizedPath))));
            continue;
        }

        Resolved[*ExistingIndex] = MoveTemp(Record);
    }

    if (!OutDiagnostics.IsEmpty())
    {
        return false;
    }

    Resolved.Sort([](const FGenesisContentRecord& Left, const FGenesisContentRecord& Right)
    {
        return Left.Id.LexicalLess(Right.Id);
    });
    ResolvedById.Reset();
    for (int32 Index = 0; Index < Resolved.Num(); ++Index)
    {
        ResolvedById.Add(Resolved[Index].Id, Index);
    }

    for (const FGenesisContentRecord& Record : Resolved)
    {
        for (const FName Reference : Record.References)
        {
            if (!ResolvedById.Contains(Reference))
            {
                OutDiagnostics.Add(Diagnostic(
                    TEXT("GEN-CONTENT-UNKNOWN-REFERENCE"), Record, TEXT("references"), TEXT("reference.exists"),
                    FString::Printf(TEXT("Unknown content reference '%s'."), *Reference.ToString()))));
            }
        }
    }
    if (!OutDiagnostics.IsEmpty())
    {
        return false;
    }

    TSharedPtr<FGenesisContentRegistrySnapshot> Snapshot = MakeShared<FGenesisContentRegistrySnapshot>();
    Snapshot->Definitions = MoveTemp(Resolved);
    for (int32 Index = 0; Index < Snapshot->Definitions.Num(); ++Index)
    {
        const FGenesisContentRecord& Record = Snapshot->Definitions[Index];
        Snapshot->ById.Add(Record.Id, Index);
        AddIndexValues(Snapshot->Producers, Record.Produces, Record.Id);
        AddIndexValues(Snapshot->Consumers, Record.Consumes, Record.Id);
        AddIndexValues(Snapshot->UnlockIndex, Record.Unlocks, Record.Id);
        AddIndexValues(Snapshot->FacilityIndex, Record.Facilities, Record.Id);
    }
    for (TPair<FName, TArray<FName>>& Pair : Snapshot->Producers) SortUnique(Pair.Value);
    for (TPair<FName, TArray<FName>>& Pair : Snapshot->Consumers) SortUnique(Pair.Value);
    for (TPair<FName, TArray<FName>>& Pair : Snapshot->UnlockIndex) SortUnique(Pair.Value);
    for (TPair<FName, TArray<FName>>& Pair : Snapshot->FacilityIndex) SortUnique(Pair.Value);

    FString Canonical;
    for (const FGenesisContentRecord& Record : Snapshot->Definitions)
    {
        Canonical += FString::Printf(
            TEXT("%d|%s|%d.%d|%d|%s|%s\n"),
            static_cast<int32>(Record.Type), *Record.Id.ToString(), Record.SchemaMajor, Record.SchemaMinor,
            TierRank(Record.Source.Tier), *Record.Source.PackageId.ToString(), *Record.CanonicalData);
    }
    FTCHARToUTF8 Utf8(*Canonical);
    uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Digest);
    Snapshot->ContentHash = BytesToHex(Digest, FSHA1::DigestSize).ToLower();

    OutSnapshot = MoveTemp(Snapshot);
    return true;
}
