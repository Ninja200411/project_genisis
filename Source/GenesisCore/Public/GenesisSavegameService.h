#pragma once

#include "CoreMinimal.h"
#include "GenesisSavegameTypes.h"

struct GENESISCORE_API FGenesisSavegameMigration
{
    int32 FromVersion = 0;
    int32 ToVersion = 0;
    TFunction<bool(FGenesisSavegameContainer&)> Apply;
};

class GENESISCORE_API FGenesisSavegameService final
{
public:
    bool RegisterMigration(FGenesisSavegameMigration Migration);

    static FGenesisSavegameResult AddOrReplaceBlock(
        FGenesisSavegameContainer& Container,
        FName BlockId,
        int32 SchemaVersion,
        TArray<uint8> Data,
        EGenesisSavegameCompression Compression = EGenesisSavegameCompression::None);

    static const FGenesisSavegameBlock* FindBlock(const FGenesisSavegameContainer& Container, FName BlockId);
    static FGenesisSavegameResult ReadBlock(const FGenesisSavegameContainer& Container, FName BlockId, TArray<uint8>& OutData);

    bool Serialize(const FGenesisSavegameContainer& Container, TArray<uint8>& OutBytes) const;
    FGenesisSavegameResult Deserialize(
        const TArray<uint8>& Bytes,
        const FString& ExpectedContentHash,
        FGenesisSavegameContainer& OutContainer,
        const FString& ExpectedBuildVersion = FString()) const;

    FGenesisSavegameResult MigrateToCurrent(FGenesisSavegameContainer& Container) const;

private:
    static uint32 ComputeChecksum(const TArray<uint8>& Data);
    static bool Compress(const TArray<uint8>& Input, TArray<uint8>& Output);
    static bool Decompress(const TArray<uint8>& Input, int32 UncompressedSize, TArray<uint8>& Output);

    TArray<FGenesisSavegameMigration> Migrations;
};
