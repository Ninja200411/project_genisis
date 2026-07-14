#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisReplayService.h"
#include "GenesisSimulationSavegame.h"
#include "GenesisSimulationStateHash.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisCompressedSavegameRoundtripTest,
    "Genesis.Core.Savegame.CompressedFullSnapshotRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisCompressedSavegameRoundtripTest::RunTest(const FString& Parameters)
{
    FGenesisSimulationSaveSnapshot Source;
    Source.Clock.CurrentTick = 42;
    Source.Clock.PendingSimulationMicroseconds = 25000;
    Source.Clock.Speed = EGenesisSimulationSpeed::Fast;
    Source.Entities.Generator.NextValue = 3;
    Source.Entities.ActiveEntities = {FGenesisEntityId(1), FGenesisEntityId(2)};

    FGenesisScheduledSystemConfig Config;
    Config.SystemId = TEXT("Test.System");
    Config.Phase = EGenesisSimulationPhase::Simulation;
    Config.Priority = 7;
    Source.Scheduler.Systems.Add(Config);

    FGenesisDomainEvent Event;
    Event.Sequence = 1;
    Event.Tick = 40;
    Event.CommandId = 1;
    Event.EventType = TEXT("Test.Event");
    Event.Entities = {FGenesisEntityId(2)};
    Event.Payload.Init(7, 2048);
    Source.Journal.Add(Event);

    FGenesisCommandEnvelope Command;
    Command.CommandId = 1;
    Command.TargetTick = 40;
    Command.Source = TEXT("Test");
    Command.CommandType = TEXT("Test.Command");
    Command.Payload = {1, 2, 3};
    Source.Commands.Add(Command);

    FGenesisSavegameBlock Component;
    Component.BlockId = TEXT("Component.Test.Reservations");
    Component.SchemaVersion = 1;
    TArray<uint8> ComponentData;
    ComponentData.Init(9, 4096);
    FGenesisSavegameContainer ComponentContainer;
    TestTrue(TEXT("Component block created"), FGenesisSavegameService::AddOrReplaceBlock(
        ComponentContainer, Component.BlockId, Component.SchemaVersion, ComponentData).bSucceeded);
    Source.ComponentBlocks.Add(ComponentContainer.Blocks[0]);

    FGenesisSavegameContainer Container;
    Container.ContentHash = TEXT("genesis-test-content-v1");
    Container.BuildVersion = TEXT("ue5.8-test");
    Container.Seed = 1337;
    TestTrue(TEXT("Full snapshot writes"), FGenesisSimulationSavegame::WriteSnapshot(Source, Container).bSucceeded);

    FGenesisSavegameService Service;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Container serializes"), Service.Serialize(Container, Bytes));

    FGenesisSavegameContainer Loaded;
    TestTrue(TEXT("Container deserializes"), Service.Deserialize(Bytes, Container.ContentHash, Loaded, Container.BuildVersion).bSucceeded);

    FGenesisSimulationSaveSnapshot Restored;
    TestTrue(TEXT("Full snapshot reads"), FGenesisSimulationSavegame::ReadSnapshot(Loaded, Restored).bSucceeded);
    TestEqual(TEXT("Tick restored"), Restored.Clock.CurrentTick, int64{42});
    TestEqual(TEXT("Entity reservation restored"), Restored.Entities.Generator.NextValue, int64{3});
    TestEqual(TEXT("Journal restored"), Restored.Journal.Num(), 1);
    TestEqual(TEXT("Commands restored"), Restored.Commands.Num(), 1);
    TestEqual(TEXT("Component blocks restored"), Restored.ComponentBlocks.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisGoldenSaveFixtureTest,
    "Genesis.Core.Savegame.GoldenFixturePresent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisGoldenSaveFixtureTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("TestData/GoldenSaves/genesis-core-v4.golden.json"));
    FString Fixture;
    TestTrue(TEXT("Golden fixture can be loaded"), FFileHelper::LoadFileToString(Fixture, *Path));
    TestTrue(TEXT("Fixture identifies schema 4"), Fixture.Contains(TEXT("\"container_schema\": 4")));
    TestTrue(TEXT("Fixture requires compressed data"), Fixture.Contains(TEXT("\"compression\": \"zlib\"")));
    TestTrue(TEXT("Fixture lists reservations block"), Fixture.Contains(TEXT("Component.Test.Reservations")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisReplayToTargetTickTest,
    "Genesis.Core.Savegame.ReplayToTargetTick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisReplayToTargetTickTest::RunTest(const FString& Parameters)
{
    int64 State = 10;
    int64 CurrentTick = 0;
    TArray<FGenesisCommandEnvelope> AcceptedCommands;

    FGenesisCommandEnvelope Command;
    Command.CommandId = 7;
    Command.TargetTick = 5;
    Command.Priority = 0;
    Command.Source = TEXT("Replay");
    Command.CommandType = TEXT("Test.Add");
    Command.Payload = {5};

    FGenesisReplayRequest Request;
    Request.InitialSave = {1};
    Request.Commands = {Command};
    Request.TargetTick = 5;
    Request.ExpectedContentHash = TEXT("content");

    auto HashState = [&State, &CurrentTick]()
    {
        uint64 Hash = 14695981039346656037ull;
        Hash ^= static_cast<uint64>(State); Hash *= 1099511628211ull;
        Hash ^= static_cast<uint64>(CurrentTick); Hash *= 1099511628211ull;
        return Hash;
    };

    FGenesisReplayCallbacks Callbacks;
    Callbacks.LoadInitialSave = [&State, &CurrentTick](const TArray<uint8>&, const FString&)
    {
        State = 10;
        CurrentTick = 0;
        return FGenesisSavegameResult::Success();
    };
    Callbacks.EnqueueCommand = [&AcceptedCommands](const FGenesisCommandEnvelope& InCommand)
    {
        AcceptedCommands.Add(InCommand);
        return FGenesisCommandResult::Accepted();
    };
    Callbacks.AdvanceToTick = [&State, &CurrentTick, &AcceptedCommands](const int64 TargetTick)
    {
        for (int64 Tick = CurrentTick + 1; Tick <= TargetTick; ++Tick)
        {
            for (const FGenesisCommandEnvelope& Pending : AcceptedCommands)
            {
                if (Pending.TargetTick == Tick)
                {
                    State += Pending.Payload[0];
                }
            }
        }
        CurrentTick = TargetTick;
        return true;
    };
    Callbacks.ComputeStateHash = HashState;

    CurrentTick = 5;
    State = 15;
    Request.ExpectedStateHash = HashState();
    State = 0;
    CurrentTick = 0;

    uint64 ReplayHash = 0;
    TestTrue(TEXT("Replay succeeds"), FGenesisReplayService::Replay(Request, Callbacks, ReplayHash).bSucceeded);
    TestEqual(TEXT("Replay stops at target tick"), CurrentTick, int64{5});
    TestEqual(TEXT("Replay restores deterministic state"), State, int64{15});
    TestEqual(TEXT("Replay hash matches"), ReplayHash, Request.ExpectedStateHash);
    return true;
}

#endif
