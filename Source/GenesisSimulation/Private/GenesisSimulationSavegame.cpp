#include "GenesisSimulationSavegame.h"

#include "GenesisEventSerializer.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

const FName FGenesisSimulationSavegame::ClockBlock(TEXT("Core.Clock"));
const FName FGenesisSimulationSavegame::SchedulerBlock(TEXT("Core.Scheduler"));
const FName FGenesisSimulationSavegame::EntitiesBlock(TEXT("Core.Entities"));
const FName FGenesisSimulationSavegame::JournalBlock(TEXT("Core.EventJournal"));
const FName FGenesisSimulationSavegame::CommandsBlock(TEXT("Core.Commands"));

namespace
{
void SerializeName(FArchive& Ar, FName& Name)
{
    FString Text = Name.ToString();
    Ar << Text;
    if (Ar.IsLoading()) Name = FName(*Text);
}

TArray<uint8> WriteClock(const FGenesisSimulationClockState& S)
{
    TArray<uint8> B; FMemoryWriter W(B, true);
    int32 V=S.SchemaVersion; int64 T=S.CurrentTick, D=S.TickDurationMicroseconds, P=S.PendingSimulationMicroseconds;
    uint8 Speed=static_cast<uint8>(S.Speed), Resume=static_cast<uint8>(S.ResumeSpeed);
    W << V << T << D << P << Speed << Resume; return B;
}
bool ReadClock(const TArray<uint8>& B, FGenesisSimulationClockState& S)
{
    FMemoryReader R(B, true); uint8 Speed=0, Resume=0;
    R << S.SchemaVersion << S.CurrentTick << S.TickDurationMicroseconds << S.PendingSimulationMicroseconds << Speed << Resume;
    S.Speed=static_cast<EGenesisSimulationSpeed>(Speed); S.ResumeSpeed=static_cast<EGenesisSimulationSpeed>(Resume);
    return !R.IsError();
}
TArray<uint8> WriteScheduler(const FGenesisSchedulerState& S)
{
    TArray<uint8> B; FMemoryWriter W(B, true); int32 V=S.SchemaVersion, N=S.Systems.Num(); W << V << N;
    for (const FGenesisScheduledSystemConfig& C : S.Systems)
    {
        FName Id=C.SystemId; int32 Priority=C.Priority; double Budget=C.BudgetMilliseconds; uint8 Phase=static_cast<uint8>(C.Phase);
        SerializeName(W, Id); W << Phase << Priority << Budget;
    }
    return B;
}
bool ReadScheduler(const TArray<uint8>& B, FGenesisSchedulerState& S)
{
    FMemoryReader R(B, true); int32 N=0; R << S.SchemaVersion << N; if (N<0 || N>100000) return false;
    for (int32 I=0; I<N; ++I)
    {
        FGenesisScheduledSystemConfig& C=S.Systems.AddDefaulted_GetRef(); uint8 Phase=0;
        SerializeName(R,C.SystemId); R << Phase << C.Priority << C.BudgetMilliseconds; C.Phase=static_cast<EGenesisSimulationPhase>(Phase);
    }
    return !R.IsError();
}
TArray<uint8> WriteEntities(const FGenesisEntityRegistrySnapshot& S)
{
    TArray<uint8> B; FMemoryWriter W(B,true); int32 V=S.SchemaVersion, GV=S.Generator.SchemaVersion, N=S.ActiveEntities.Num(); int64 Next=S.Generator.NextValue;
    W << V << GV << Next << N; for (FGenesisEntityId Id:S.ActiveEntities){int64 X=Id.Value; W << X;} return B;
}
bool ReadEntities(const TArray<uint8>& B, FGenesisEntityRegistrySnapshot& S)
{
    FMemoryReader R(B,true); int32 N=0; R << S.SchemaVersion << S.Generator.SchemaVersion << S.Generator.NextValue << N; if(N<0||N>10000000)return false;
    for(int32 I=0;I<N;++I){int64 X=0;R<<X;S.ActiveEntities.Add(FGenesisEntityId(X));} return !R.IsError();
}
TArray<uint8> WriteJournal(const TArray<FGenesisDomainEvent>& Events)
{
    TArray<uint8> B; FMemoryWriter W(B,true); int32 N=Events.Num(); W<<N;
    for(const FGenesisDomainEvent& E:Events){TArray<uint8> EB; FGenesisEventSerializer::Serialize(E,EB); W<<EB;} return B;
}
bool ReadJournal(const TArray<uint8>& B, TArray<FGenesisDomainEvent>& Events)
{
    FMemoryReader R(B,true); int32 N=0;R<<N;if(N<0||N>10000000)return false;
    for(int32 I=0;I<N;++I){TArray<uint8> EB;R<<EB;FGenesisDomainEvent E;if(!FGenesisEventSerializer::Deserialize(EB,E))return false;Events.Add(MoveTemp(E));} return !R.IsError();
}
TArray<uint8> WriteCommands(const TArray<FGenesisCommandEnvelope>& Commands)
{
    TArray<FGenesisCommandEnvelope> Sorted=Commands; Sorted.Sort([](const auto& A,const auto& B){if(A.TargetTick!=B.TargetTick)return A.TargetTick<B.TargetTick;if(A.Priority!=B.Priority)return A.Priority<B.Priority;return A.CommandId<B.CommandId;});
    TArray<uint8>B;FMemoryWriter W(B,true);int32 N=Sorted.Num();W<<N;
    for(const auto& C:Sorted){int32 V=C.SchemaVersion,P=C.Priority;int64 Id=C.CommandId,T=C.TargetTick;FName S=C.Source,Type=C.CommandType;TArray<uint8>Payload=C.Payload;W<<V<<Id<<T<<P;SerializeName(W,S);SerializeName(W,Type);W<<Payload;}return B;
}
bool ReadCommands(const TArray<uint8>& B,TArray<FGenesisCommandEnvelope>& Commands)
{
    FMemoryReader R(B,true);int32 N=0;R<<N;if(N<0||N>10000000)return false;
    for(int32 I=0;I<N;++I){auto&C=Commands.AddDefaulted_GetRef();R<<C.SchemaVersion<<C.CommandId<<C.TargetTick<<C.Priority;SerializeName(R,C.Source);SerializeName(R,C.CommandType);R<<C.Payload;if(!C.IsSyntacticallyValid())return false;}return !R.IsError();
}
}

FGenesisSavegameResult FGenesisSimulationSavegame::WriteSnapshot(const FGenesisSimulationSaveSnapshot& Snapshot,FGenesisSavegameContainer& Container,const EGenesisSavegameCompression Compression)
{
    FGenesisSavegameResult R;
    R=FGenesisSavegameService::AddOrReplaceBlock(Container,ClockBlock,1,WriteClock(Snapshot.Clock),Compression);if(!R.bSucceeded)return R;
    R=FGenesisSavegameService::AddOrReplaceBlock(Container,SchedulerBlock,1,WriteScheduler(Snapshot.Scheduler),Compression);if(!R.bSucceeded)return R;
    R=FGenesisSavegameService::AddOrReplaceBlock(Container,EntitiesBlock,1,WriteEntities(Snapshot.Entities),Compression);if(!R.bSucceeded)return R;
    R=FGenesisSavegameService::AddOrReplaceBlock(Container,JournalBlock,1,WriteJournal(Snapshot.Journal),Compression);if(!R.bSucceeded)return R;
    R=FGenesisSavegameService::AddOrReplaceBlock(Container,CommandsBlock,1,WriteCommands(Snapshot.Commands),Compression);if(!R.bSucceeded)return R;
    for(const FGenesisSavegameBlock& Block:Snapshot.ComponentBlocks)
    {
        TArray<uint8> Data;
        FGenesisSavegameContainer Temp;Temp.Blocks.Add(Block);
        R=FGenesisSavegameService::ReadBlock(Temp,Block.BlockId,Data);if(!R.bSucceeded)return R;
        R=FGenesisSavegameService::AddOrReplaceBlock(Container,Block.BlockId,Block.SchemaVersion,MoveTemp(Data),Compression);if(!R.bSucceeded)return R;
    }
    Container.SavedTick=Snapshot.Clock.CurrentTick;
    return FGenesisSavegameResult::Success();
}

FGenesisSavegameResult FGenesisSimulationSavegame::ReadSnapshot(const FGenesisSavegameContainer& Container,FGenesisSimulationSaveSnapshot& OutSnapshot)
{
    TArray<uint8>B;FGenesisSavegameResult R;
    R=FGenesisSavegameService::ReadBlock(Container,ClockBlock,B);if(!R.bSucceeded||!ReadClock(B,OutSnapshot.Clock))return R.bSucceeded?FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock,TEXT("Genesis.Save.ClockCorrupt"),ClockBlock):R;
    R=FGenesisSavegameService::ReadBlock(Container,SchedulerBlock,B);if(!R.bSucceeded||!ReadScheduler(B,OutSnapshot.Scheduler))return R.bSucceeded?FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock,TEXT("Genesis.Save.SchedulerCorrupt"),SchedulerBlock):R;
    R=FGenesisSavegameService::ReadBlock(Container,EntitiesBlock,B);if(!R.bSucceeded||!ReadEntities(B,OutSnapshot.Entities))return R.bSucceeded?FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock,TEXT("Genesis.Save.EntitiesCorrupt"),EntitiesBlock):R;
    R=FGenesisSavegameService::ReadBlock(Container,JournalBlock,B);if(!R.bSucceeded||!ReadJournal(B,OutSnapshot.Journal))return R.bSucceeded?FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock,TEXT("Genesis.Save.JournalCorrupt"),JournalBlock):R;
    R=FGenesisSavegameService::ReadBlock(Container,CommandsBlock,B);if(!R.bSucceeded||!ReadCommands(B,OutSnapshot.Commands))return R.bSucceeded?FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock,TEXT("Genesis.Save.CommandsCorrupt"),CommandsBlock):R;
    for(const FGenesisSavegameBlock& Block:Container.Blocks)
    {
        if(Block.BlockId!=ClockBlock&&Block.BlockId!=SchedulerBlock&&Block.BlockId!=EntitiesBlock&&Block.BlockId!=JournalBlock&&Block.BlockId!=CommandsBlock)OutSnapshot.ComponentBlocks.Add(Block);
    }
    return FGenesisSavegameResult::Success();
}
