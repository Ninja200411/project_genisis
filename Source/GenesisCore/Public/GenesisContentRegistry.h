#pragma once

#include "CoreMinimal.h"
#include "GenesisContentRegistry.generated.h"

UENUM(BlueprintType)
enum class EGenesisContentSourceTier : uint8
{
    Core,
    Extension,
    Mod,
    Scenario
};

UENUM(BlueprintType)
enum class EGenesisContentDefinitionType : uint8
{
    Building,
    Resource,
    PersonRole,
    Vehicle,
    Recipe,
    Technology
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisContentSource
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content")
    EGenesisContentSourceTier Tier = EGenesisContentSourceTier::Core;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content")
    FName PackageId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content")
    FString NormalizedPath;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisContentDiagnostic
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FName Code;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FName DefinitionId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FString File;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FString FieldPath;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FName Rule;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Content") FString Message;
};

USTRUCT()
struct GENESISCORE_API FGenesisContentRecord
{
    GENERATED_BODY()

    UPROPERTY() EGenesisContentDefinitionType Type = EGenesisContentDefinitionType::Resource;
    UPROPERTY() FName Id;
    UPROPERTY() int32 SchemaMajor = 1;
    UPROPERTY() int32 SchemaMinor = 0;
    UPROPERTY() FGenesisContentSource Source;
    UPROPERTY() FName Replaces;
    UPROPERTY() TArray<FName> References;
    UPROPERTY() TArray<FName> Produces;
    UPROPERTY() TArray<FName> Consumes;
    UPROPERTY() TArray<FName> Unlocks;
    UPROPERTY() TArray<FName> Facilities;
    UPROPERTY() FString CanonicalData;
};

class GENESISCORE_API FGenesisContentRegistrySnapshot final
{
public:
    const FGenesisContentRecord* Find(FName Id) const;
    const TArray<FGenesisContentRecord>& GetDefinitions() const { return Definitions; }
    const TArray<FName>* FindProducers(FName Id) const { return Producers.Find(Id); }
    const TArray<FName>* FindConsumers(FName Id) const { return Consumers.Find(Id); }
    const TArray<FName>* FindUnlocks(FName Id) const { return UnlockIndex.Find(Id); }
    const TArray<FName>* FindFacilities(FName Id) const { return FacilityIndex.Find(Id); }
    const FString& GetContentHash() const { return ContentHash; }

private:
    friend class FGenesisContentRegistry;
    TArray<FGenesisContentRecord> Definitions;
    TMap<FName, int32> ById;
    TMap<FName, TArray<FName>> Producers;
    TMap<FName, TArray<FName>> Consumers;
    TMap<FName, TArray<FName>> UnlockIndex;
    TMap<FName, TArray<FName>> FacilityIndex;
    FString ContentHash;
};

class GENESISCORE_API FGenesisContentRegistry final
{
public:
    bool Reload(const TArray<FGenesisContentRecord>& Candidates, TArray<FGenesisContentDiagnostic>& OutDiagnostics);
    TSharedPtr<const FGenesisContentRegistrySnapshot> GetSnapshot() const { return PublishedSnapshot; }

private:
    static bool BuildSnapshot(
        const TArray<FGenesisContentRecord>& Candidates,
        TSharedPtr<FGenesisContentRegistrySnapshot>& OutSnapshot,
        TArray<FGenesisContentDiagnostic>& OutDiagnostics);

    TSharedPtr<const FGenesisContentRegistrySnapshot> PublishedSnapshot;
};
