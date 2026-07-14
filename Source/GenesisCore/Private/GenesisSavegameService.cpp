#include "GenesisSavegameService.h"

#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
constexpr uint32 SaveMagic = 0x47534E53;
constexpr int32 ContainerFormatVersion = 1;

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

bool FGenesisSavegameService::RegisterMigration(FGenesisSavegameMigration Migration)
{
    if (Migration.FromVersion <= 0 || Migration.ToVersion != Migration.FromVersion + 1 || !Migration.Apply ||
        Migrations.ContainsByPredicate([&Migration](const FGenesisSavegameMigration& Existing)
        {
            return Existing.FromVersion == Migration.FromVersion;
        }))
    {
        return false;
    }

    Migrations.Add(MoveTemp(Migration));
    Migrations.Sort([](const FGenesisSavegameMigration& Left, const FGenesisSavegameMigration& Right)
    {
        return Left.FromVersion < Right.FromVersion;
    });
    return true;
}

void FGenesisSavegameService::AddOrReplaceBlock(
    FGenesisSavegameContainer& Container,
    const FName BlockId,
    const int32 SchemaVersion,
    TArray<uint8> Data)
{
    FGenesisSavegameBlock* Block = Container.Blocks.FindByPredicate([BlockId](const FGenesisSavegameBlock& Existing)
    {
        return Existing.BlockId == BlockId;
    });
    if (Block == nullptr)
    {
        Block = &Container.Blocks.AddDefaulted_GetRef();
        Block->BlockId = BlockId;
    }
    Block->SchemaVersion = FMath::Max(1, SchemaVersion);
    Block->Data = MoveTemp(Data);
    Block->Checksum = ComputeChecksum(Block->Data);
    Container.Blocks.Sort([](const FGenesisSavegameBlock& Left, const FGenesisSavegameBlock& Right)
    {
        return Left.BlockId.LexicalLess(Right.BlockId);
    });
}

const FGenesisSavegameBlock* FGenesisSavegameService::FindBlock(
    const FGenesisSavegameContainer& Container,
    const FName BlockId)
{
    return Container.Blocks.FindByPredicate([BlockId](const FGenesisSavegameBlock& Block)
    {
        return Block.BlockId == BlockId;
    });
}

bool FGenesisSavegameService::Serialize(const FGenesisSavegameContainer& Container, TArray<uint8>& OutBytes) const
{
    if (Container.SchemaVersion <= 0 || Container.ContentHash.IsEmpty() || Container.BuildVersion.IsEmpty() ||
        Container.SavedTick < 0)
    {
        return false;
    }

    OutBytes.Reset();
    FMemoryWriter Writer(OutBytes, true);
    uint32 Magic = SaveMagic;
    int32 FormatVersion = ContainerFormatVersion;
    int32 SchemaVersion = Container.SchemaVersion;
    FString ContentHash = Container.ContentHash;
    FString BuildVersion = Container.BuildVersion;
    int64 SavedTick = Container.SavedTick;
    int64 Seed = Container.Seed;
    TArray<FGenesisSavegameBlock> Blocks = Container.Blocks;
    Blocks.Sort([](const FGenesisSavegameBlock& Left, const FGenesisSavegameBlock& Right)
    {
        return Left.BlockId.LexicalLess(Right.BlockId);
    });

    Writer << Magic;
    Writer << FormatVersion;
    Writer << SchemaVersion;
    Writer << ContentHash;
    Writer << BuildVersion;
    Writer << SavedTick;
    Writer << Seed;

    int32 BlockCount = Blocks.Num();
    Writer << BlockCount;
    for (FGenesisSavegameBlock& Block : Blocks)
    {
        SerializeName(Writer, Block.BlockId);
        Writer << Block.SchemaVersion;
        Block.Checksum = ComputeChecksum(Block.Data);
        Writer << Block.Checksum;
        Writer << Block.Data;
    }
    return !Writer.IsError();
}

FGenesisSavegameResult FGenesisSavegameService::Deserialize(
    const TArray<uint8>& Bytes,
    const FString& ExpectedContentHash,
    FGenesisSavegameContainer& OutContainer) const
{
    if (Bytes.IsEmpty())
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.Empty"));
    }

    FMemoryReader Reader(Bytes, true);
    uint32 Magic = 0;
    int32 FormatVersion = 0;
    FGenesisSavegameContainer Container;
    Reader << Magic;
    Reader << FormatVersion;
    if (Magic != SaveMagic || FormatVersion != ContainerFormatVersion)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.InvalidHeader"));
    }

    Reader << Container.SchemaVersion;
    Reader << Container.ContentHash;
    Reader << Container.BuildVersion;
    Reader << Container.SavedTick;
    Reader << Container.Seed;

    int32 BlockCount = 0;
    Reader << BlockCount;
    if (BlockCount < 0 || BlockCount > 100000)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.InvalidBlockCount"));
    }

    Container.Blocks.Reserve(BlockCount);
    for (int32 Index = 0; Index < BlockCount; ++Index)
    {
        FGenesisSavegameBlock& Block = Container.Blocks.AddDefaulted_GetRef();
        SerializeName(Reader, Block.BlockId);
        Reader << Block.SchemaVersion;
        Reader << Block.Checksum;
        Reader << Block.Data;
        if (Reader.IsError() || Block.BlockId.IsNone() || Block.SchemaVersion <= 0 ||
            Block.Checksum != ComputeChecksum(Block.Data))
        {
            return FGenesisSavegameResult::Failure(
                EGenesisSavegameErrorCode::CorruptBlock,
                TEXT("Genesis.Save.CorruptBlock"),
                Block.BlockId);
        }
    }

    if (!ExpectedContentHash.IsEmpty() && Container.ContentHash != ExpectedContentHash)
    {
        return FGenesisSavegameResult::Failure(
            EGenesisSavegameErrorCode::ContentHashMismatch,
            TEXT("Genesis.Save.ContentHashMismatch"));
    }

    FGenesisSavegameResult MigrationResult = MigrateToCurrent(Container);
    if (!MigrationResult.bSucceeded)
    {
        return MigrationResult;
    }

    OutContainer = MoveTemp(Container);
    return FGenesisSavegameResult::Success();
}

FGenesisSavegameResult FGenesisSavegameService::MigrateToCurrent(FGenesisSavegameContainer& Container) const
{
    if (Container.SchemaVersion <= 0 || Container.SchemaVersion > FGenesisSavegameContainer::CurrentSchemaVersion)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::UnsupportedSchema, TEXT("Genesis.Save.UnsupportedSchema"));
    }

    while (Container.SchemaVersion < FGenesisSavegameContainer::CurrentSchemaVersion)
    {
        const FGenesisSavegameMigration* Migration = Migrations.FindByPredicate([&Container](const FGenesisSavegameMigration& Candidate)
        {
            return Candidate.FromVersion == Container.SchemaVersion;
        });
        if (Migration == nullptr)
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::MigrationMissing, TEXT("Genesis.Save.MigrationMissing"));
        }
        const int32 PreviousVersion = Container.SchemaVersion;
        if (!Migration->Apply(Container))
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::MigrationFailed, TEXT("Genesis.Save.MigrationFailed"));
        }
        Container.SchemaVersion = Migration->ToVersion;
        if (Container.SchemaVersion != PreviousVersion + 1)
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::MigrationFailed, TEXT("Genesis.Save.InvalidMigrationStep"));
        }
    }
    return FGenesisSavegameResult::Success();
}

uint32 FGenesisSavegameService::ComputeChecksum(const TArray<uint8>& Data)
{
    return FCrc::MemCrc32(Data.GetData(), Data.Num());
}
