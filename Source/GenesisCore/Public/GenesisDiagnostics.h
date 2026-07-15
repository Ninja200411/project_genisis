#pragma once

#include "CoreMinimal.h"
#include "GenesisDiagnostics.generated.h"

UENUM(BlueprintType)
enum class EGenesisDiagnosticSeverity : uint8
{
    Info,
    Warning,
    Error,
    Fatal
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisDiagnostic
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisDiagnosticSeverity Severity = EGenesisDiagnosticSeverity::Error;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Code;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString SourceFile;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ObjectId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString FieldPath;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Rule;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Message;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString CorrectionHint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FName> RelatedIds;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisExecutionContext
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString BuildVersion;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString ContentHash;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Seed = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid CommandId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FGuid> EntityIds;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisTelemetrySample
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Metric;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) double Value = 0.0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Tick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TMap<FName, FString> Dimensions;
};

class GENESISCORE_API FGenesisDiagnosticReport
{
public:
    explicit FGenesisDiagnosticReport(FGenesisExecutionContext InContext = {});
    void Add(FGenesisDiagnostic Diagnostic);
    void AddTelemetry(FGenesisTelemetrySample Sample);
    bool HasErrors() const;
    TMap<FString, TArray<FGenesisDiagnostic>> GroupBySource() const;
    FString ToConsoleText() const;
    FString ToJson() const;
    const TArray<FGenesisDiagnostic>& GetDiagnostics() const { return Diagnostics; }

private:
    FGenesisExecutionContext Context;
    TArray<FGenesisDiagnostic> Diagnostics;
    TArray<FGenesisTelemetrySample> Telemetry;
};

class GENESISCORE_API FGenesisEventRingBuffer
{
public:
    explicit FGenesisEventRingBuffer(int32 InCapacity = 256) : Capacity(FMath::Max(InCapacity, 1)) {}
    void Push(FString Event);
    const TArray<FString>& GetEvents() const { return Events; }

private:
    int32 Capacity;
    TArray<FString> Events;
};
