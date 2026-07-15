#pragma once

#include "CoreMinimal.h"
#include "GenesisVerification.generated.h"

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisCanonicalEntityState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid EntityId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Type;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, int64> IntegerFields;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FString> StringFields;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisReferenceCheckpoint
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ExpectedHash;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisReferenceWorld
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Seed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ContentHash;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> Commands;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGenesisReferenceCheckpoint> Checkpoints;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisSaveEnvelope
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ContentHash;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Seed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGenesisCanonicalEntityState> Entities;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisVerificationDifference
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid EntityId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Field;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Expected;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Actual;
};

class GENESISSIMULATION_API IGenesisSaveMigration
{
public:
    virtual ~IGenesisSaveMigration() = default;
    virtual int32 FromVersion() const = 0;
    virtual int32 ToVersion() const = 0;
    virtual bool Apply(FGenesisSaveEnvelope& Save, FString& OutError) const = 0;
};

class GENESISSIMULATION_API FGenesisSaveMigrationRegistry
{
public:
    void Register(TSharedRef<const IGenesisSaveMigration> Migration);
    bool Migrate(FGenesisSaveEnvelope& Save, int32 TargetVersion, FString& OutError) const;

private:
    TMap<int32, TSharedRef<const IGenesisSaveMigration>> BySourceVersion;
};

class GENESISSIMULATION_API FGenesisStateVerifier
{
public:
    static FString Hash(const TArray<FGenesisCanonicalEntityState>& Entities);
    static TArray<FGenesisVerificationDifference> Diff(
        const TArray<FGenesisCanonicalEntityState>& Expected,
        const TArray<FGenesisCanonicalEntityState>& Actual,
        int64 Tick);
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisPerformanceBudget
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName System;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 EntityCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double MedianMilliseconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double P95Milliseconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double MaximumMilliseconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 MaximumBytes = 0;
};

class GENESISSIMULATION_API FGenesisPerformanceGate
{
public:
    static bool Passes(const FGenesisPerformanceBudget& Budget, const TArray<double>& Measurements, int64 UsedBytes, FString& OutReason);
};
