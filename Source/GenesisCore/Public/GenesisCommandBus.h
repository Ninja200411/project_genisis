#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandTypes.h"
#include "GenesisEventJournal.h"

struct GENESISCORE_API FGenesisCommandHandler
{
    FName CommandType;
    TFunction<FGenesisCommandResult(const FGenesisCommandEnvelope&)> Authorize;
    TFunction<FGenesisCommandResult(const FGenesisCommandEnvelope&)> Validate;
    TFunction<FGenesisCommandResult(const FGenesisCommandEnvelope&, TArray<FGenesisDomainEvent>&)> Execute;
};

class GENESISCORE_API FGenesisCommandBus final
{
public:
    bool RegisterHandler(FGenesisCommandHandler Handler);
    bool UnregisterHandler(FName CommandType);
    void Reset();

    FGenesisCommandResult Enqueue(FGenesisCommandEnvelope Command);
    TArray<FGenesisCommandResult> ProcessTick(int64 Tick, FGenesisEventJournal& Journal);

    int32 GetPendingCount() const { return PendingCommands.Num(); }
    bool HasProcessedCommand(int64 CommandId) const { return ProcessedCommandIds.Contains(CommandId); }

private:
    void SortQueue();

    TArray<FGenesisCommandHandler> Handlers;
    TArray<FGenesisCommandEnvelope> PendingCommands;
    TSet<int64> KnownCommandIds;
    TSet<int64> ProcessedCommandIds;
};
