#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandTypes.h"
#include "GenesisSavegameTypes.h"

struct GENESISCORE_API FGenesisReplayRequest
{
    TArray<uint8> InitialSave;
    TArray<FGenesisCommandEnvelope> Commands;
    int64 TargetTick = 0;
    FString ExpectedContentHash;
    uint64 ExpectedStateHash = 0;
};

struct GENESISCORE_API FGenesisReplayCallbacks
{
    TFunction<FGenesisSavegameResult(const TArray<uint8>&, const FString&)> LoadInitialSave;
    TFunction<FGenesisCommandResult(const FGenesisCommandEnvelope&)> EnqueueCommand;
    TFunction<bool(int64)> AdvanceToTick;
    TFunction<uint64()> ComputeStateHash;
};

class GENESISCORE_API FGenesisReplayService final
{
public:
    static FGenesisSavegameResult Replay(const FGenesisReplayRequest& Request, const FGenesisReplayCallbacks& Callbacks, uint64& OutStateHash);
};
