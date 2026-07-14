#pragma once

#include "CoreMinimal.h"
#include "GenesisCommandTypes.h"
#include "GenesisEntityRegistry.h"
#include "GenesisEventJournal.h"
#include "GenesisSavegameService.h"
#include "GenesisSimulationClock.h"
#include "GenesisTickScheduler.h"

struct GENESISSIMULATION_API FGenesisSimulationSaveSnapshot
{
    FGenesisSimulationClockState Clock;
    FGenesisSchedulerState Scheduler;
    FGenesisEntityRegistrySnapshot Entities;
    TArray<FGenesisDomainEvent> Journal;
    TArray<FGenesisCommandEnvelope> Commands;
    TArray<FGenesisSavegameBlock> ComponentBlocks;
};

class GENESISSIMULATION_API FGenesisSimulationSavegame final
{
public:
    static const FName ClockBlock;
    static const FName SchedulerBlock;
    static const FName EntitiesBlock;
    static const FName JournalBlock;
    static const FName CommandsBlock;

    static FGenesisSavegameResult WriteSnapshot(
        const FGenesisSimulationSaveSnapshot& Snapshot,
        FGenesisSavegameContainer& Container,
        EGenesisSavegameCompression Compression = EGenesisSavegameCompression::Zlib);

    static FGenesisSavegameResult ReadSnapshot(
        const FGenesisSavegameContainer& Container,
        FGenesisSimulationSaveSnapshot& OutSnapshot);
};
