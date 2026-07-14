#include "GenesisSavegameService.h"

#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
constexpr uint32 SaveMagic = 0x47534E53;
constexpr int32 ContainerFormatVersion = 2;

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

FGenesisSavegameResult FGenesisSavegameService::AddOrReplaceBlock(
    FGenesisSavegameContainer& Container,
    const FName BlockId,
    const int32 SchemaVersion,
    TArray<uint8> Data,
    const EGenesisSavegameCompression Compression)
{
    if (BlockId.IsNone() || SchemaVersion <= 0)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.InvalidBlock"), BlockId);
    }

    FGenesisSavegameBlock* Block = Container.Blocks.FindByPredicate([BlockId](const FGenesisSavegameBlock& Existing)
    {
        return Existing.BlockId == BlockId;
    });
    if (Block == nullptr)
    {
        Block = &Container.Blocks.AddDefaulted_GetRef();
        Block->BlockId = BlockId;
    }

    Block->SchemaVersion = SchemaVersion;
    Block->Compression = Compression;
    Block->UncompressedSize = Data.Num();
    Block->Checksum = ComputeChecksum(Data);

    if (Compression == EGenesisSavegameCompression::Zlib)
    {
        if (!Compress(Data, Block->Data))
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CompressionFailed, TEXT("Genesis.Save.CompressionFailed"), BlockId);
        }
    }
    else
    {
        Block->Data = MoveTemp(Data);
    }

    Container.Blocks.Sort([](const FGenesisSavegameBlock& Left, const FGenesisSavegameBlock& Right)
    {
        return Left.BlockId.LexicalLess(Right.BlockId);
    });
    return FGenesisSavegameResult::Success();
}

const FGenesisSavegameBlock* FGenesisSavegameService::FindBlock(const FGenesisSavegameContainer& Container, const FName BlockId)
{
    return Container.Blocks.FindByPredicate([BlockId](const FGenesisSavegameBlock& Block)
    {
        return Block.BlockId == BlockId;
    });
}

FGenesisSavegameResult FGenesisSavegameService::ReadBlock(
    const FGenesisSavegameContainer& Container,
    const FName BlockId,
    TArray<uint8>& OutData)
{
    const FGenesisSavegameBlock* Block = FindBlock(Container, BlockId);
    if (Block == nullptr)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::MissingBlock, TEXT("Genesis.Save.MissingBlock"), BlockId);
    }

    if (Block->Compression == EGenesisSavegameCompression::Zlib)
    {
        if (!Decompress(Block->Data, Block->UncompressedSize, OutData))
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::DecompressionFailed, TEXT("Genesis.Save.DecompressionFailed"), BlockId);
        }
    }
    else
    {
        OutData = Block->Data;
    }

    if (ComputeChecksum(OutData) != Block->Checksum)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock, TEXT("Genesis.Save.CorruptBlock"), BlockId);
    }
    return FGenesisSavegameResult::Success();
}

bool FGenesisSavegameService::Serialize(const FGenesisSavegameContainer& Container, TArray<uint8>& OutBytes) const
{
    if (Container.SchemaVersion <= 0 || Container.ContentHash.IsEmpty() || Container.BuildVersion.IsEmpty() || Container.SavedTick < 0)
    {
        return false;
    }

    TArray<FGenesisSavegameBlock> Blocks = Container.Blocks;
    Blocks.Sort([](const FGenesisSavegameBlock& Left, const FGenesisSavegameBlock& Right)
    {
        return Left.BlockId.LexicalLess(Right.BlockId);
    });

    OutBytes.Reset();
    FMemoryWriter Writer(OutBytes, true);
    uint32 Magic = SaveMagic;
    int32 FormatVersion = ContainerFormatVersion;
    int32 SchemaVersion = Container.SchemaVersion;
    FString ContentHash = Container.ContentHash;
    FString BuildVersion = Container.BuildVersion;
    int64 SavedTick = Container.SavedTick;
    int64 Seed = Container.Seed;

    Writer << Magic << FormatVersion << SchemaVersion << ContentHash << BuildVersion << SavedTick << Seed;
    int32 BlockCount = Blocks.Num();
    Writer << BlockCount;
    for (FGenesisSavegameBlock& Block : Blocks)
    {
        SerializeName(Writer, Block.BlockId);
        Writer << Block.SchemaVersion;
        uint8 Compression = static_cast<uint8>(Block.Compression);
        Writer << Compression << Block.UncompressedSize << Block.Checksum << Block.Data;
    }
    return !Writer.IsError();
}

FGenesisSavegameResult FGenesisSavegameService::Deserialize(
    const TArray<uint8>& Bytes,
    const FString& ExpectedContentHash,
    FGenesisSavegameContainer& OutContainer,
    const FString& ExpectedBuildVersion) const
{
    if (Bytes.IsEmpty())
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.Empty"));
    }

    FMemoryReader Reader(Bytes, true);
    uint32 Magic = 0;
    int32 FormatVersion = 0;
    FGenesisSavegameContainer Container;
    Reader << Magic << FormatVersion;
    if (Magic != SaveMagic || FormatVersion != ContainerFormatVersion)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.InvalidHeader"));
    }

    Reader << Container.SchemaVersion << Container.ContentHash << Container.BuildVersion << Container.SavedTick << Container.Seed;
    int32 BlockCount = 0;
    Reader << BlockCount;
    if (BlockCount < 0 || BlockCount > 100000)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::InvalidContainer, TEXT("Genesis.Save.InvalidBlockCount"));
    }

    TSet<FName> SeenBlocks;
    for (int32 Index = 0; Index < BlockCount; ++Index)
    {
        FGenesisSavegameBlock& Block = Container.Blocks.AddDefaulted_GetRef();
        SerializeName(Reader, Block.BlockId);
        Reader << Block.SchemaVersion;
        uint8 Compression = 0;
        Reader << Compression << Block.UncompressedSize << Block.Checksum << Block.Data;
        Block.Compression = static_cast<EGenesisSavegameCompression>(Compression);
        if (Reader.IsError() || Block.BlockId.IsNone() || Block.SchemaVersion <= 0 || Block.UncompressedSize < 0 ||
            Compression > static_cast<uint8>(EGenesisSavegameCompression::Zlib) || SeenBlocks.Contains(Block.BlockId))
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::CorruptBlock, TEXT("Genesis.Save.CorruptBlock"), Block.BlockId);
        }
        SeenBlocks.Add(Block.BlockId);
        TArray<uint8> VerifiedData;
        const FGenesisSavegameResult VerifyResult = ReadBlock(Container, Block.BlockId, VerifiedData);
        if (!VerifyResult.bSucceeded)
        {
            return VerifyResult;
        }
    }

    if (!ExpectedContentHash.IsEmpty() && Container.ContentHash != ExpectedContentHash)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::ContentHashMismatch, TEXT("Genesis.Save.ContentHashMismatch"));
    }
    if (!ExpectedBuildVersion.IsEmpty() && Container.BuildVersion != ExpectedBuildVersion)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::BuildHashMismatch, TEXT("Genesis.Save.BuildVersionMismatch"));
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

bool FGenesisSavegameService::Compress(const TArray<uint8>& Input, TArray<uint8>& Output)
{
    if (Input.IsEmpty())
    {
        Output.Reset();
        return true;
    }
    int32 Bound = FCompression::CompressMemoryBound(NAME_Zlib, Input.Num());
    Output.SetNumUninitialized(Bound);
    int32 CompressedSize = Bound;
    if (!FCompression::CompressMemory(NAME_Zlib, Output.GetData(), CompressedSize, Input.GetData(), Input.Num(), COMPRESS_BiasMemory))
    {
        Output.Reset();
        return false;
    }
    Output.SetNum(CompressedSize, EAllowShrinking::Yes);
    return true;
}

bool FGenesisSavegameService::Decompress(const TArray<uint8>& Input, const int32 UncompressedSize, TArray<uint8>& Output)
{
    if (UncompressedSize == 0)
    {
        Output.Reset();
        return Input.IsEmpty();
    }
    Output.SetNumUninitialized(UncompressedSize);
    if (!FCompression::UncompressMemory(NAME_Zlib, Output.GetData(), UncompressedSize, Input.GetData(), Input.Num()))
    {
        Output.Reset();
        return false;
    }
    return true;
}
