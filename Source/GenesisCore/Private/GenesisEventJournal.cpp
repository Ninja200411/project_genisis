#include "GenesisEventJournal.h"

void FGenesisEventJournal::Reset()
{
    Events.Reset();
    NextSequence = 1;
}

void FGenesisEventJournal::Append(FGenesisDomainEvent Event)
{
    Event.Sequence = NextSequence++;
    Event.Entities.Sort();
    Events.Add(MoveTemp(Event));
}

TArray<FGenesisDomainEvent> FGenesisEventJournal::FindByTick(const int64 Tick) const
{
    return Events.FilterByPredicate([Tick](const FGenesisDomainEvent& Event)
    {
        return Event.Tick == Tick;
    });
}

TArray<FGenesisDomainEvent> FGenesisEventJournal::FindByEntity(const FGenesisEntityId EntityId) const
{
    return Events.FilterByPredicate([EntityId](const FGenesisDomainEvent& Event)
    {
        return Event.Entities.Contains(EntityId);
    });
}

TArray<FGenesisDomainEvent> FGenesisEventJournal::FindByCommand(const int64 CommandId) const
{
    return Events.FilterByPredicate([CommandId](const FGenesisDomainEvent& Event)
    {
        return Event.CommandId == CommandId;
    });
}

bool FGenesisEventJournal::ReplayToTick(
    const int64 InclusiveTick,
    const TFunctionRef<bool(const FGenesisDomainEvent&)>& Consumer) const
{
    for (const FGenesisDomainEvent& Event : Events)
    {
        if (Event.Tick > InclusiveTick)
        {
            break;
        }

        if (!Consumer(Event))
        {
            return false;
        }
    }
    return true;
}
