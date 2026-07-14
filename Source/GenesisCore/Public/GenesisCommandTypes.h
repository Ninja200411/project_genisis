#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisCommandTypes.generated.h"

UENUM(BlueprintType)
enum class EGenesisCommandErrorCode : uint8
{
    None,
    InvalidEnvelope,
    DuplicateCommand,
    UnknownCommandType,
    Unauthorized,
    SemanticValidationFailed,
    HandlerFailed
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisCommandResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Command")
    bool bAccepted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Command")
    EGenesisCommandErrorCode ErrorCode = EGenesisCommandErrorCode::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Command")
    FName MessageKey;

    static FGenesisCommandResult Accepted()
    {
        FGenesisCommandResult Result;
        Result.bAccepted = true;
        return Result;
    }

    static FGenesisCommandResult Rejected(const EGenesisCommandErrorCode Code, const FName InMessageKey)
    {
        FGenesisCommandResult Result;
        Result.ErrorCode = Code;
        Result.MessageKey = InMessageKey;
        return Result;
    }
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisCommandEnvelope
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    int64 CommandId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    int64 TargetTick = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    FName Source;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    FName CommandType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Command")
    TArray<uint8> Payload;

    bool IsSyntacticallyValid() const
    {
        return SchemaVersion > 0 && CommandId > 0 && TargetTick >= 0 && !Source.IsNone() && !CommandType.IsNone();
    }
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisDomainEvent
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    int64 Sequence = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    int64 Tick = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    int64 CommandId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    FName EventType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    TArray<FGenesisEntityId> Entities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Event")
    TArray<uint8> Payload;
};
