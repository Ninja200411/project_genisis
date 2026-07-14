#pragma once

#include "CoreMinimal.h"

#include "GenesisSavegameTypes.generated.h"

UENUM(BlueprintType)
enum class EGenesisSavegameErrorCode : uint8
{
    None,
    InvalidContainer,
    UnsupportedSchema,
    ContentHashMismatch,
    BuildHashMismatch,
    MissingBlock,
    CorruptBlock,
    MigrationMissing,
    MigrationFailed
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisSavegameResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Savegame")
    bool bSucceeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Savegame")
    EGenesisSavegameErrorCode ErrorCode = EGenesisSavegameErrorCode::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Savegame")
    FName MessageKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Savegame")
    FName BlockId;

    static FGenesisSavegameResult Success()
    {
        FGenesisSavegameResult Result;
        Result.bSucceeded = true;
        return Result;
    }

    static FGenesisSavegameResult Failure(
        const EGenesisSavegameErrorCode Code,
        const FName InMessageKey,
        const FName InBlockId = NAME_None)
    {
        FGenesisSavegameResult Result;
        Result.ErrorCode = Code;
        Result.MessageKey = InMessageKey;
        Result.BlockId = InBlockId;
        return Result;
    }
};

USTRUCT()
struct GENESISCORE_API FGenesisSavegameBlock
{
    GENERATED_BODY()

    UPROPERTY()
    FName BlockId;

    UPROPERTY()
    int32 SchemaVersion = 1;

    UPROPERTY()
    uint32 Checksum = 0;

    UPROPERTY()
    TArray<uint8> Data;
};

USTRUCT()
struct GENESISCORE_API FGenesisSavegameContainer
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 4;

    UPROPERTY()
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY()
    FString ContentHash;

    UPROPERTY()
    FString BuildVersion;

    UPROPERTY()
    int64 SavedTick = 0;

    UPROPERTY()
    int64 Seed = 0;

    UPROPERTY()
    TArray<FGenesisSavegameBlock> Blocks;
};
