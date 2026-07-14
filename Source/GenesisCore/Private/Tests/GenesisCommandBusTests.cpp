#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisCommandBus.h"
#include "GenesisEventSerializer.h"
#include "Misc/AutomationTest.h"

namespace
{
FGenesisCommandHandler MakeAddHandler(int64& State)
{
    FGenesisCommandHandler Handler;
    Handler.CommandType = TEXT("Test.Add");
    Handler.Validate = [](const FGenesisCommandEnvelope& Command)
    {
        return Command.Payload.Num() == sizeof(int64)
            ? FGenesisCommandResult::Accepted()
            : FGenesisCommandResult::Rejected(
                EGenesisCommandErrorCode::SemanticValidationFailed,
                TEXT("Genesis.Command.InvalidPayload"));
    };
    Handler.Execute = [&State](const FGenesisCommandEnvelope& Command, TArray<FGenesisDomainEvent>& Events)
    {
        int64 Delta = 0;
        FMemory::Memcpy(&Delta, Command.Payload.GetData(), sizeof(int64));
        State += Delta;

        FGenesisDomainEvent Event;
        Event.EventType = TEXT("Test.Added");
        Event.Payload = Command.Payload;
        Events.Add(MoveTemp(Event));
        return FGenesisCommandResult::Accepted();
    };
    return Handler;
}

FGenesisCommandEnvelope MakeAddCommand(const int64 Id, const int64 Tick, const int32 Priority, const int64 Delta)
{
    FGenesisCommandEnvelope Command;
    Command.CommandId = Id;
    Command.TargetTick = Tick;
    Command.Priority = Priority;
    Command.Source = TEXT("Test");
    Command.CommandType = TEXT("Test.Add");
    Command.Payload.SetNumUninitialized(sizeof(int64));
    FMemory::Memcpy(Command.Payload.GetData(), &Delta, sizeof(int64));
    return Command;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisCommandDeterministicOrderTest,
    "Genesis.Core.Commands.DeterministicOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisCommandDeterministicOrderTest::RunTest(const FString& Parameters)
{
    int64 State = 0;
    FGenesisCommandBus Bus;
    FGenesisEventJournal Journal;
    TestTrue(TEXT("Handler registers"), Bus.RegisterHandler(MakeAddHandler(State)));

    TestTrue(TEXT("Command 30 enqueues"), Bus.Enqueue(MakeAddCommand(30, 5, 10, 3)).bAccepted);
    TestTrue(TEXT("Command 10 enqueues"), Bus.Enqueue(MakeAddCommand(10, 5, 0, 1)).bAccepted);
    TestTrue(TEXT("Command 20 enqueues"), Bus.Enqueue(MakeAddCommand(20, 5, 10, 2)).bAccepted);

    const TArray<FGenesisCommandResult> Results = Bus.ProcessTick(5, Journal);
    TestEqual(TEXT("All commands execute"), Results.Num(), 3);
    TestEqual(TEXT("State receives all deltas"), State, int64{6});
    TestEqual(TEXT("Three events emitted"), Journal.GetAll().Num(), 3);
    TestEqual(TEXT("Priority command executes first"), Journal.GetAll()[0].CommandId, int64{10});
    TestEqual(TEXT("Equal priority commands sort by ID"), Journal.GetAll()[1].CommandId, int64{20});
    TestEqual(TEXT("Highest ID follows"), Journal.GetAll()[2].CommandId, int64{30});
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisCommandInvalidAndDuplicateTest,
    "Genesis.Core.Commands.InvalidAndDuplicate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisCommandInvalidAndDuplicateTest::RunTest(const FString& Parameters)
{
    int64 State = 0;
    FGenesisCommandBus Bus;
    FGenesisEventJournal Journal;
    Bus.RegisterHandler(MakeAddHandler(State));

    FGenesisCommandEnvelope Invalid = MakeAddCommand(1, 1, 0, 5);
    Invalid.Payload.Reset();
    TestTrue(TEXT("Syntactically valid command enqueues"), Bus.Enqueue(Invalid).bAccepted);
    const TArray<FGenesisCommandResult> Results = Bus.ProcessTick(1, Journal);
    TestFalse(TEXT("Semantic validation rejects invalid payload"), Results[0].bAccepted);
    TestEqual(TEXT("Rejected command changes no state"), State, int64{0});
    TestEqual(TEXT("Rejected command emits no event"), Journal.GetAll().Num(), 0);

    const FGenesisCommandEnvelope Command = MakeAddCommand(2, 2, 0, 1);
    TestTrue(TEXT("First ID is accepted"), Bus.Enqueue(Command).bAccepted);
    const FGenesisCommandResult Duplicate = Bus.Enqueue(Command);
    TestFalse(TEXT("Duplicate ID is rejected idempotently"), Duplicate.bAccepted);
    TestEqual(TEXT("Duplicate uses stable error code"), Duplicate.ErrorCode, EGenesisCommandErrorCode::DuplicateCommand);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEventJournalQueryAndReplayTest,
    "Genesis.Core.Commands.JournalQueryAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEventJournalQueryAndReplayTest::RunTest(const FString& Parameters)
{
    FGenesisEventJournal Journal;
    FGenesisDomainEvent First;
    First.Tick = 1;
    First.CommandId = 10;
    First.EventType = TEXT("Test.Event");
    First.Entities.Add(FGenesisEntityId(7));
    First.Payload = {1, 2, 3};
    Journal.Append(First);

    FGenesisDomainEvent Second = First;
    Second.Tick = 2;
    Second.CommandId = 20;
    Second.Entities = {FGenesisEntityId(9)};
    Second.Payload = {4, 5};
    Journal.Append(Second);

    TestEqual(TEXT("Query by tick"), Journal.FindByTick(1).Num(), 1);
    TestEqual(TEXT("Query by entity"), Journal.FindByEntity(FGenesisEntityId(9)).Num(), 1);
    TestEqual(TEXT("Query by command"), Journal.FindByCommand(10).Num(), 1);

    auto ComputeReplayHash = [&Journal]()
    {
        uint64 Hash = 14695981039346656037ull;
        Journal.ReplayToTick(2, [&Hash](const FGenesisDomainEvent& Event)
        {
            Hash ^= static_cast<uint64>(Event.CommandId);
            Hash *= 1099511628211ull;
            for (const uint8 Byte : Event.Payload)
            {
                Hash ^= Byte;
                Hash *= 1099511628211ull;
            }
            return true;
        });
        return Hash;
    };

    TestEqual(TEXT("Replay produces stable hash"), ComputeReplayHash(), ComputeReplayHash());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEventSerializationRoundTripTest,
    "Genesis.Core.Commands.EventSerialization.RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEventSerializationRoundTripTest::RunTest(const FString& Parameters)
{
    FGenesisDomainEvent Source;
    Source.Sequence = 12;
    Source.Tick = 9;
    Source.CommandId = 42;
    Source.EventType = TEXT("Test.Serialized");
    Source.Entities = {FGenesisEntityId(8), FGenesisEntityId(3)};
    Source.Payload = {10, 20, 30};

    TArray<uint8> Bytes;
    TestTrue(TEXT("Event serializes"), FGenesisEventSerializer::Serialize(Source, Bytes));

    FGenesisDomainEvent Restored;
    TestTrue(TEXT("Event deserializes"), FGenesisEventSerializer::Deserialize(Bytes, Restored));
    TestEqual(TEXT("Sequence round-trips"), Restored.Sequence, Source.Sequence);
    TestEqual(TEXT("Tick round-trips"), Restored.Tick, Source.Tick);
    TestEqual(TEXT("Command round-trips"), Restored.CommandId, Source.CommandId);
    TestEqual(TEXT("Type round-trips"), Restored.EventType, Source.EventType);
    TestEqual(TEXT("Entities are canonicalized"), Restored.Entities[0], FGenesisEntityId(3));
    TestEqual(TEXT("Payload round-trips"), Restored.Payload, Source.Payload);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisEventSerializationForwardCompatibilityTest,
    "Genesis.Core.Commands.EventSerialization.ForwardCompatibleSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEventSerializationForwardCompatibilityTest::RunTest(const FString& Parameters)
{
    FGenesisDomainEvent FutureEvent;
    FutureEvent.SchemaVersion = FGenesisDomainEvent::CurrentSchemaVersion + 1;
    FutureEvent.Sequence = 1;
    FutureEvent.Tick = 2;
    FutureEvent.CommandId = 3;
    FutureEvent.EventType = TEXT("Test.FutureEvent");
    FutureEvent.Payload = {99};

    TArray<uint8> Bytes;
    TestTrue(TEXT("Future event schema can be stored"), FGenesisEventSerializer::Serialize(FutureEvent, Bytes));

    FGenesisDomainEvent Restored;
    TestTrue(TEXT("Known container preserves unknown event schema"), FGenesisEventSerializer::Deserialize(Bytes, Restored));
    TestEqual(TEXT("Unknown schema version is preserved"), Restored.SchemaVersion, FutureEvent.SchemaVersion);
    TestEqual(TEXT("Opaque payload is preserved"), Restored.Payload, FutureEvent.Payload);
    return true;
}

#endif
