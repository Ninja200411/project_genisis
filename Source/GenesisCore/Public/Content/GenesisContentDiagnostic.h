#pragma once

#include "CoreMinimal.h"
#include "GenesisContentDiagnostic.generated.h"

UENUM()
enum class EGenesisContentDiagnosticSeverity : uint8
{
    Info,
    Warning,
    Error
};

USTRUCT()
struct GENESISCORE_API FGenesisContentDiagnostic
{
    GENERATED_BODY()

    UPROPERTY()
    EGenesisContentDiagnosticSeverity Severity = EGenesisContentDiagnosticSeverity::Error;

    UPROPERTY()
    FName Code;

    UPROPERTY()
    FString SourceFile;

    UPROPERTY()
    FName ObjectId;

    UPROPERTY()
    FString FieldPath;

    UPROPERTY()
    FString Rule;

    UPROPERTY()
    FString Message;
};
