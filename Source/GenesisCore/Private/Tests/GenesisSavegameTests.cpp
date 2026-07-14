#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisSavegameService.h"
#include "Misc/AutomationTest.h"

namespace
{
FGenesisSavegameService MakeMigrationService()
{
    FGenesisSavegameService Service;
    for (int32 Version = 1; Version < FGenesisSavegameContainer::CurrentSchemaVersion; ++Version)
    {
        FGenesisSavegameMigration Migration;
        Migration.FromVersion = Version;
        Migration.ToVersion = Version + 1;
        Migration.Apply = [Version](FGenesisSavegameContainer& Container)
        {
            TArray<uint8> Marker = {static_cast<uint8>(Version + 1)};
            FGenesisSavegameService::AddOrReplaceBlock(
                Container,
                FName(*FString::Printf(TEXT("Migration.V%d"), Version + 1)),
                1,
                MoveTemp(Marker));
            return true;
        };
        Service.RegisterMigration(MoveTemp(Migration));
    }
    return Service;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSavegameRoundtripTest,
    "Genesis.Core.Savegame.Roundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSavegameRoundtripTest::RunTest(const FString& Parameters)
{
    FGenesisSavegameService Service = MakeMigrationService();
    FGenesisSavegameContainer Source;
    Source.ContentHash = TEXT("content-a");
    Source.BuildVersion = TEXT("dev-1");
    Source.SavedTick = 123;
    Source.Seed = 456;
    FGenesisSavegameService::AddOrReplaceBlock(Source, TEXT("World"), 2, {1, 2, 3, 4});
    FGenesisSavegameService::AddOrReplaceBlock(Source, TEXT("Reservations"), 1, {5, 6});

    TArray<uint8> Bytes;
    TestTrue(TEXT("Container serializes"), Service.Serialize(Source, Bytes));

    FGenesisSavegameContainer Loaded;
    const FGenesisSavegameResult Result = Service.Deserialize(Bytes, TEXT("content-a"), Loaded);
    TestTrue(TEXT("Container loads"), Result.bSucceeded);
    TestEqual(TEXT("Tick survives"), Loaded.SavedTick, int64{123});
    TestEqual(TEXT("Seed survives"), Loaded.Seed, int64{456});
    TestEqual(TEXT("All blocks survive"), Loaded.Blocks.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSavegameThreeVersionMigrationTest,
    "Genesis.Core.Savegame.MigrateThreeVersions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSavegameThreeVersionMigrationTest::RunTest(const FString& Parameters)
{
    FGenesisSavegameService Service = MakeMigrationService();
    FGenesisSavegameContainer Container;
    Container.SchemaVersion = 1;
    Container.ContentHash = TEXT("content-a");
    Container.BuildVersion = TEXT("old-build");

    const FGenesisSavegameResult First = Service.MigrateToCurrent(Container);
    TestTrue(TEXT("Version one migrates to current"), First.bSucceeded);
    TestEqual(TEXT("Current version reached"), Container.SchemaVersion, FGenesisSavegameContainer::CurrentSchemaVersion);
    TestNotNull(TEXT("V2 marker exists"), FGenesisSavegameService::FindBlock(Container, TEXT("Migration.V2")));
    TestNotNull(TEXT("V3 marker exists"), FGenesisSavegameService::FindBlock(Container, TEXT("Migration.V3")));
    TestNotNull(TEXT("V4 marker exists"), FGenesisSavegameService::FindBlock(Container, TEXT("Migration.V4")));

    const int32 BlockCount = Container.Blocks.Num();
    const FGenesisSavegameResult Second = Service.MigrateToCurrent(Container);
    TestTrue(TEXT("Migration is idempotent at current version"), Second.bSucceeded);
    TestEqual(TEXT("Second migration adds no blocks"), Container.Blocks.Num(), BlockCount);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisSavegameCorruptionAndContentHashTest,
    "Genesis.Core.Savegame.CorruptionAndContentHash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisSavegameCorruptionAndContentHashTest::RunTest(const FString& Parameters)
{
    FGenesisSavegameService Service = MakeMigrationService();
    FGenesisSavegameContainer Source;
    Source.ContentHash = TEXT("expected");
    Source.BuildVersion = TEXT("dev");
    FGenesisSavegameService::AddOrReplaceBlock(Source, TEXT("World"), 1, {10, 20, 30});

    TArray<uint8> Bytes;
    Service.Serialize(Source, Bytes);

    FGenesisSavegameContainer Loaded;
    const FGenesisSavegameResult HashResult = Service.Deserialize(Bytes, TEXT("different"), Loaded);
    TestEqual(TEXT("Content mismatch has precise code"), HashResult.ErrorCode, EGenesisSavegameErrorCode::ContentHashMismatch);

    Bytes.Last() ^= 0xFF;
    const FGenesisSavegameResult CorruptResult = Service.Deserialize(Bytes, TEXT("expected"), Loaded);
    TestEqual(TEXT("Corruption is rejected"), CorruptResult.ErrorCode, EGenesisSavegameErrorCode::CorruptBlock);
    TestEqual(TEXT("Corrupt block is identified"), CorruptResult.BlockId, FName(TEXT("World")));
    return true;
}

#endif
