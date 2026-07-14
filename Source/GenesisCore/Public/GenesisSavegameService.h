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

    static void AddOrReplaceBlock(FGenesisSavegameContainer& Container, FName BlockId, int32 SchemaVersion, TArray<uint8> Data);
    static const FGenesisSavegameBlock* FindBlock(const FGenesisSavegameContainer& Container, FName BlockId);

    bool Serialize(const FGenesisSavegameContainer& Container, TArray<uint8>& OutBytes) const;
    FGenesisSavegameResult Deserialize(
        const TArray<uint8>& Bytes,
        const FString& ExpectedContentHash,
        FGenesisSavegameContainer& OutContainer) const;

    FGenesisSavegameResult MigrateToCurrent(FGenesisSavegameContainer& Container) const;

private:
    static uint32 ComputeChecksum(const TArray<uint8>& Data);
    TArray<FGenesisSavegameMigration> Migrations;
};
