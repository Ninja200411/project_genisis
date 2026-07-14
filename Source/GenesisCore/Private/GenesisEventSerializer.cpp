#include "GenesisEventSerializer.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
void SerializeName(FArchive& Archive, FName& Name)
{
    FString Value = Name.ToString();
    Archive << Value;
    if (Archive.IsLoading())
    {
        Name = FName(*Value);
    }
}
}

bool FGenesisEventSerializer::Serialize(const FGenesisDomainEvent& Event, TArray<uint8>& OutBytes)
{
    if (Event.SchemaVersion <= 0 || Event.Sequence < 0 || Event.Tick < 0 || Event.CommandId <= 0 || Event.EventType.IsNone())
    {
        return false;
    }

    OutBytes.Reset();
    FMemoryWriter Writer(OutBytes, true);

    uint32 FileMagic = Magic;
    int32 ContainerVersion = CurrentContainerVersion;
    int32 EventSchemaVersion = Event.SchemaVersion;
    int64 Sequence = Event.Sequence;
    int64 Tick = Event.Tick;
    int64 CommandId = Event.CommandId;
    FName EventType = Event.EventType;
    TArray<FGenesisEntityId> Entities = Event.Entities;
    TArray<uint8> Payload = Event.Payload;

    Writer << FileMagic;
    Writer << ContainerVersion;
    Writer << EventSchemaVersion;
    Writer << Sequence;
    Writer << Tick;
    Writer << CommandId;
    SerializeName(Writer, EventType);

    int32 EntityCount = Entities.Num();
    Writer << EntityCount;
    for (const FGenesisEntityId EntityId : Entities)
    {
        int64 Value = EntityId.Value;
        Writer << Value;
    }

    Writer << Payload;
    return !Writer.IsError();
}

bool FGenesisEventSerializer::Deserialize(const TArray<uint8>& Bytes, FGenesisDomainEvent& OutEvent)
{
    if (Bytes.IsEmpty())
    {
        return false;
    }

    FMemoryReader Reader(Bytes, true);
    uint32 FileMagic = 0;
    int32 ContainerVersion = 0;
    FGenesisDomainEvent Event;

    Reader << FileMagic;
    Reader << ContainerVersion;
    if (FileMagic != Magic || ContainerVersion <= 0 || ContainerVersion > CurrentContainerVersion)
    {
        return false;
    }

    Reader << Event.SchemaVersion;
    Reader << Event.Sequence;
    Reader << Event.Tick;
    Reader << Event.CommandId;
    SerializeName(Reader, Event.EventType);

    int32 EntityCount = 0;
    Reader << EntityCount;
    if (EntityCount < 0 || EntityCount > 1000000)
    {
        return false;
    }

    Event.Entities.Reserve(EntityCount);
    for (int32 Index = 0; Index < EntityCount; ++Index)
    {
        int64 Value = 0;
        Reader << Value;
        Event.Entities.Add(FGenesisEntityId(Value));
    }

    Reader << Event.Payload;
    if (Reader.IsError() || Event.SchemaVersion <= 0 || Event.Sequence < 0 || Event.Tick < 0 ||
        Event.CommandId <= 0 || Event.EventType.IsNone())
    {
        return false;
    }

    Event.Entities.Sort();
    OutEvent = MoveTemp(Event);
    return true;
}
