#include "GenesisReplayService.h"

FGenesisSavegameResult FGenesisReplayService::Replay(
    const FGenesisReplayRequest& Request,
    const FGenesisReplayCallbacks& Callbacks,
    uint64& OutStateHash)
{
    OutStateHash = 0;
    if (Request.TargetTick < 0 || !Callbacks.LoadInitialSave || !Callbacks.EnqueueCommand ||
        !Callbacks.AdvanceToTick || !Callbacks.ComputeStateHash)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::ReplayFailed, TEXT("Genesis.Replay.InvalidRequest"));
    }

    const FGenesisSavegameResult LoadResult = Callbacks.LoadInitialSave(Request.InitialSave, Request.ExpectedContentHash);
    if (!LoadResult.bSucceeded)
    {
        return LoadResult;
    }

    TArray<FGenesisCommandEnvelope> Commands = Request.Commands;
    Commands.Sort([](const FGenesisCommandEnvelope& Left, const FGenesisCommandEnvelope& Right)
    {
        if (Left.TargetTick != Right.TargetTick)
        {
            return Left.TargetTick < Right.TargetTick;
        }
        if (Left.Priority != Right.Priority)
        {
            return Left.Priority < Right.Priority;
        }
        return Left.CommandId < Right.CommandId;
    });

    for (const FGenesisCommandEnvelope& Command : Commands)
    {
        if (Command.TargetTick > Request.TargetTick)
        {
            break;
        }
        const FGenesisCommandResult Result = Callbacks.EnqueueCommand(Command);
        if (!Result.bAccepted && Result.ErrorCode != EGenesisCommandErrorCode::DuplicateCommand)
        {
            return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::ReplayFailed, TEXT("Genesis.Replay.CommandRejected"));
        }
    }

    if (!Callbacks.AdvanceToTick(Request.TargetTick))
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::ReplayFailed, TEXT("Genesis.Replay.AdvanceFailed"));
    }

    OutStateHash = Callbacks.ComputeStateHash();
    if (Request.ExpectedStateHash != 0 && OutStateHash != Request.ExpectedStateHash)
    {
        return FGenesisSavegameResult::Failure(EGenesisSavegameErrorCode::StateHashMismatch, TEXT("Genesis.Replay.StateHashMismatch"));
    }
    return FGenesisSavegameResult::Success();
}
