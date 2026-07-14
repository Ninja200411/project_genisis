#include "GenesisCommandBus.h"

namespace
{
const FGenesisCommandHandler* FindHandler(
    const TArray<FGenesisCommandHandler>& Handlers,
    const FName CommandType)
{
    return Handlers.FindByPredicate([CommandType](const FGenesisCommandHandler& Handler)
    {
        return Handler.CommandType == CommandType;
    });
}
}

bool FGenesisCommandBus::RegisterHandler(FGenesisCommandHandler Handler)
{
    if (Handler.CommandType.IsNone() || !Handler.Validate || !Handler.Execute ||
        Handlers.ContainsByPredicate([&Handler](const FGenesisCommandHandler& Existing)
        {
            return Existing.CommandType == Handler.CommandType;
        }))
    {
        return false;
    }

    Handlers.Add(MoveTemp(Handler));
    Handlers.Sort([](const FGenesisCommandHandler& Left, const FGenesisCommandHandler& Right)
    {
        return Left.CommandType.LexicalLess(Right.CommandType);
    });
    return true;
}

bool FGenesisCommandBus::UnregisterHandler(const FName CommandType)
{
    return Handlers.RemoveAll([CommandType](const FGenesisCommandHandler& Handler)
    {
        return Handler.CommandType == CommandType;
    }) > 0;
}

void FGenesisCommandBus::Reset()
{
    PendingCommands.Reset();
    KnownCommandIds.Reset();
    ProcessedCommandIds.Reset();
}

FGenesisCommandResult FGenesisCommandBus::Enqueue(FGenesisCommandEnvelope Command)
{
    if (!Command.IsSyntacticallyValid())
    {
        return FGenesisCommandResult::Rejected(
            EGenesisCommandErrorCode::InvalidEnvelope,
            TEXT("Genesis.Command.InvalidEnvelope"));
    }

    if (KnownCommandIds.Contains(Command.CommandId))
    {
        return FGenesisCommandResult::Rejected(
            EGenesisCommandErrorCode::DuplicateCommand,
            TEXT("Genesis.Command.Duplicate"));
    }

    if (FindHandler(Handlers, Command.CommandType) == nullptr)
    {
        return FGenesisCommandResult::Rejected(
            EGenesisCommandErrorCode::UnknownCommandType,
            TEXT("Genesis.Command.UnknownType"));
    }

    KnownCommandIds.Add(Command.CommandId);
    PendingCommands.Add(MoveTemp(Command));
    SortQueue();
    return FGenesisCommandResult::Accepted();
}

TArray<FGenesisCommandResult> FGenesisCommandBus::ProcessTick(
    const int64 Tick,
    FGenesisEventJournal& Journal)
{
    TArray<FGenesisCommandResult> Results;
    int32 ProcessCount = 0;

    while (ProcessCount < PendingCommands.Num() && PendingCommands[ProcessCount].TargetTick <= Tick)
    {
        const FGenesisCommandEnvelope& Command = PendingCommands[ProcessCount];
        const FGenesisCommandHandler* Handler = FindHandler(Handlers, Command.CommandType);

        FGenesisCommandResult Result;
        if (Handler == nullptr)
        {
            Result = FGenesisCommandResult::Rejected(
                EGenesisCommandErrorCode::UnknownCommandType,
                TEXT("Genesis.Command.UnknownType"));
        }
        else if (Handler->Authorize)
        {
            Result = Handler->Authorize(Command);
        }
        else
        {
            Result = FGenesisCommandResult::Accepted();
        }

        if (Result.bAccepted)
        {
            Result = Handler->Validate(Command);
        }

        if (Result.bAccepted)
        {
            TArray<FGenesisDomainEvent> Events;
            Result = Handler->Execute(Command, Events);
            if (Result.bAccepted)
            {
                for (FGenesisDomainEvent& Event : Events)
                {
                    Event.Tick = Tick;
                    Event.CommandId = Command.CommandId;
                    Journal.Append(MoveTemp(Event));
                }
            }
        }

        ProcessedCommandIds.Add(Command.CommandId);
        Results.Add(Result);
        ++ProcessCount;
    }

    if (ProcessCount > 0)
    {
        PendingCommands.RemoveAt(0, ProcessCount, EAllowShrinking::No);
    }

    return Results;
}

void FGenesisCommandBus::SortQueue()
{
    PendingCommands.Sort([](const FGenesisCommandEnvelope& Left, const FGenesisCommandEnvelope& Right)
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
}
