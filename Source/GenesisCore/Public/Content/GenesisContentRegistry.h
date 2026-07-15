#pragma once

#include "CoreMinimal.h"
#include "Content/GenesisContentDiagnostic.h"

namespace Genesis::Content
{
enum class ELoadPhase : uint8
{
    Discover,
    Parse,
    SchemaValidate,
    ResolveReferences,
    SemanticValidate,
    Freeze
};

enum class ESourceLayer : uint8
{
    Core,
    Extension,
    Mod,
    Scenario
};

struct GENESISCORE_API FSourceMetadata
{
    ESourceLayer Layer = ESourceLayer::Core;
    FString SourceName;
    FString NormalizedPath;
    FString Version;
};

struct GENESISCORE_API FDefinition
{
    FName Id;
    FName Type;
    FName Replaces;
    FString CanonicalPayload;
    TArray<FName> References;
    FSourceMetadata Source;
};

class GENESISCORE_API FRegistrySnapshot
{
public:
    const FDefinition* Find(FName Id) const;
    const TArray<FName>* FindIdsByType(FName Type) const;
    const FString& GetContentHash() const { return ContentHash; }
    int32 Num() const { return Definitions.Num(); }

private:
    friend class FRegistryBuilder;

    TMap<FName, FDefinition> Definitions;
    TMap<FName, TArray<FName>> IdsByType;
    FString ContentHash;
};

struct GENESISCORE_API FBuildResult
{
    TSharedPtr<const FRegistrySnapshot> Snapshot;
    TArray<FGenesisContentDiagnostic> Diagnostics;

    bool IsSuccess() const { return Snapshot.IsValid(); }
};

class GENESISCORE_API FRegistryBuilder
{
public:
    FBuildResult Build(TArray<FDefinition> Definitions) const;

private:
    static FString NormalizePath(const FString& Path);
    static bool IsCompatibleOverride(const FDefinition& Existing, const FDefinition& Replacement);
    static FString ComputeContentHash(const TArray<FDefinition>& Definitions);
    static void AddDiagnostic(
        TArray<FGenesisContentDiagnostic>& Diagnostics,
        FName Code,
        const FDefinition& Definition,
        const FString& FieldPath,
        const FString& Rule,
        const FString& Message);
};

class GENESISCORE_API FAtomicRegistry
{
public:
    TSharedPtr<const FRegistrySnapshot> GetSnapshot() const;
    bool Reload(const TArray<FDefinition>& Definitions, TArray<FGenesisContentDiagnostic>& OutDiagnostics);

private:
    mutable FRWLock Lock;
    TSharedPtr<const FRegistrySnapshot> CurrentSnapshot;
};
}
