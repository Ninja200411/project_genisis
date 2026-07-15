#include "Content/GenesisContentRegistry.h"

#include "Misc/SecureHash.h"

namespace Genesis::Content
{
const FDefinition* FRegistrySnapshot::Find(const FName Id) const
{
    return Definitions.Find(Id);
}

const TArray<FName>* FRegistrySnapshot::FindIdsByType(const FName Type) const
{
    return IdsByType.Find(Type);
}

FString FRegistryBuilder::NormalizePath(const FString& Path)
{
    FString Result = Path;
    Result.ReplaceInline(TEXT("\\"), TEXT("/"));
    Result.ToLowerInline();
    return Result;
}

bool FRegistryBuilder::IsCompatibleOverride(const FDefinition& Existing, const FDefinition& Replacement)
{
    return Existing.Type == Replacement.Type && Replacement.Replaces == Existing.Id;
}

void FRegistryBuilder::AddDiagnostic(TArray<FGenesisContentDiagnostic>& Diagnostics, const FName Code,
    const FDefinition& Definition, const FString& FieldPath, const FString& Rule, const FString& Message)
{
    FGenesisContentDiagnostic& Diagnostic = Diagnostics.AddDefaulted_GetRef();
    Diagnostic.Code = Code;
    Diagnostic.SourceFile = Definition.Source.NormalizedPath;
    Diagnostic.ObjectId = Definition.Id;
    Diagnostic.FieldPath = FieldPath;
    Diagnostic.Rule = Rule;
    Diagnostic.Message = Message;
}

FString FRegistryBuilder::ComputeContentHash(const TArray<FDefinition>& Definitions)
{
    FString Canonical;
    for (const FDefinition& Definition : Definitions)
    {
        Canonical += FString::Printf(TEXT("%d|%s|%s|%s|%s|%s|"), static_cast<int32>(Definition.Source.Layer),
            *Definition.Source.SourceName, *Definition.Source.NormalizedPath, *Definition.Source.Version,
            *Definition.Type.ToString(), *Definition.Id.ToString());
        Canonical += Definition.CanonicalPayload;
        Canonical += TEXT("\n");
    }

    FTCHARToUTF8 Utf8(*Canonical);
    uint8 Hash[FSHA1::DigestSize];
    FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Hash);
    return BytesToHex(Hash, FSHA1::DigestSize).ToLower();
}

FBuildResult FRegistryBuilder::Build(TArray<FDefinition> Definitions) const
{
    FBuildResult Result;
    for (FDefinition& Definition : Definitions)
    {
        Definition.Source.NormalizedPath = NormalizePath(Definition.Source.NormalizedPath);
        Definition.References.Sort(FNameLexicalLess());
    }

    Definitions.Sort([](const FDefinition& A, const FDefinition& B)
    {
        if (A.Source.Layer != B.Source.Layer) return A.Source.Layer < B.Source.Layer;
        const int32 PathCompare = A.Source.NormalizedPath.Compare(B.Source.NormalizedPath, ESearchCase::CaseSensitive);
        return PathCompare == 0 ? A.Id.LexicalLess(B.Id) : PathCompare < 0;
    });

    TSharedRef<FRegistrySnapshot> Snapshot = MakeShared<FRegistrySnapshot>();
    TArray<FDefinition> EffectiveDefinitions;
    for (const FDefinition& Definition : Definitions)
    {
        if (Definition.Id.IsNone() || Definition.Type.IsNone())
        {
            AddDiagnostic(Result.Diagnostics, TEXT("CONTENT_SCHEMA_REQUIRED"), Definition, TEXT("id/type"),
                TEXT("required"), TEXT("Definition requires non-empty id and type."));
            continue;
        }

        if (const FDefinition* Existing = Snapshot->Definitions.Find(Definition.Id))
        {
            if (!IsCompatibleOverride(*Existing, Definition))
            {
                AddDiagnostic(Result.Diagnostics, TEXT("CONTENT_OVERRIDE_CONFLICT"), Definition, TEXT("replaces"),
                    TEXT("compatible-override"), TEXT("Duplicate id requires a compatible replaces declaration."));
                continue;
            }
            EffectiveDefinitions.RemoveAll([&Definition](const FDefinition& Item) { return Item.Id == Definition.Id; });
        }

        Snapshot->Definitions.Add(Definition.Id, Definition);
        EffectiveDefinitions.Add(Definition);
    }

    for (const FDefinition& Definition : EffectiveDefinitions)
    {
        for (const FName Reference : Definition.References)
        {
            if (!Snapshot->Definitions.Contains(Reference))
            {
                AddDiagnostic(Result.Diagnostics, TEXT("CONTENT_REFERENCE_UNKNOWN"), Definition,
                    TEXT("references"), TEXT("known-id"), FString::Printf(TEXT("Unknown reference '%s'."), *Reference.ToString()));
            }
        }
    }

    if (Result.Diagnostics.ContainsByPredicate([](const FGenesisContentDiagnostic& D)
        { return D.Severity == EGenesisContentDiagnosticSeverity::Error; }))
    {
        return Result;
    }

    for (const FDefinition& Definition : EffectiveDefinitions)
    {
        Snapshot->IdsByType.FindOrAdd(Definition.Type).Add(Definition.Id);
    }
    for (TPair<FName, TArray<FName>>& Entry : Snapshot->IdsByType)
    {
        Entry.Value.Sort(FNameLexicalLess());
    }
    Snapshot->ContentHash = ComputeContentHash(EffectiveDefinitions);
    Result.Snapshot = Snapshot;
    return Result;
}

TSharedPtr<const FRegistrySnapshot> FAtomicRegistry::GetSnapshot() const
{
    FReadScopeLock Scope(Lock);
    return CurrentSnapshot;
}

bool FAtomicRegistry::Reload(const TArray<FDefinition>& Definitions, TArray<FGenesisContentDiagnostic>& OutDiagnostics)
{
    const FBuildResult Result = FRegistryBuilder().Build(Definitions);
    OutDiagnostics = Result.Diagnostics;
    if (!Result.IsSuccess()) return false;

    FWriteScopeLock Scope(Lock);
    CurrentSnapshot = Result.Snapshot;
    return true;
}
}
