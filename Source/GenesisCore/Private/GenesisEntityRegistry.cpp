#include "GenesisEntityRegistry.h"

FGenesisEntityId FGenesisEntityIdGenerator::Allocate()
{
    check(NextValue > 0 && NextValue < MAX_int64);
    return FGenesisEntityId(NextValue++);
}

FGenesisEntityGeneratorState FGenesisEntityIdGenerator::CaptureState() const
{
    FGenesisEntityGeneratorState State;
    State.NextValue = NextValue;
    return State;
}

bool FGenesisEntityIdGenerator::RestoreState(const FGenesisEntityGeneratorState& State)
{
    if (State.SchemaVersion != FGenesisEntityGeneratorState::CurrentSchemaVersion || State.NextValue <= 0)
    {
        return false;
    }

    NextValue = State.NextValue;
    return true;
}

FGenesisEntityId FGenesisEntityRegistry::QueueCreate()
{
    const FGenesisEntityId EntityId = Generator.Allocate();
    PendingCreates.Add(EntityId);
    return EntityId;
}

bool FGenesisEntityRegistry::QueueDestroy(const FGenesisEntityId EntityId)
{
    if (!IsAlive(EntityId) || PendingDestroys.Contains(EntityId))
    {
        return false;
    }

    PendingDestroys.Add(EntityId);
    return true;
}

void FGenesisEntityRegistry::ApplyPendingChanges()
{
    PendingCreates.Sort();
    PendingDestroys.Sort();

    for (const FGenesisEntityId EntityId : PendingCreates)
    {
        ActiveEntities.Add(EntityId);
    }
    ActiveEntities.Sort();

    for (const FGenesisEntityId EntityId : PendingDestroys)
    {
        for (IGenesisComponentStore* Store : ComponentStores)
        {
            Store->RemoveEntity(EntityId);
        }
        ActiveEntities.RemoveSingle(EntityId);
    }

    PendingCreates.Reset();
    PendingDestroys.Reset();
}

void FGenesisEntityRegistry::Reset()
{
    const TArray<FGenesisEntityId> EntitiesToRemove = ActiveEntities;
    for (IGenesisComponentStore* Store : ComponentStores)
    {
        for (const FGenesisEntityId EntityId : EntitiesToRemove)
        {
            Store->RemoveEntity(EntityId);
        }
    }

    ActiveEntities.Reset();
    PendingCreates.Reset();
    PendingDestroys.Reset();
    Generator = FGenesisEntityIdGenerator{};
}

bool FGenesisEntityRegistry::IsAlive(const FGenesisEntityId EntityId) const
{
    return ActiveEntities.Contains(EntityId) && !PendingDestroys.Contains(EntityId);
}

bool FGenesisEntityRegistry::IsPendingDestroy(const FGenesisEntityId EntityId) const
{
    return PendingDestroys.Contains(EntityId);
}

bool FGenesisEntityRegistry::RegisterComponentStore(IGenesisComponentStore& Store)
{
    if (Store.GetStoreId().IsNone() || ComponentStores.ContainsByPredicate([&Store](const IGenesisComponentStore* Existing)
        {
            return Existing->GetStoreId() == Store.GetStoreId();
        }))
    {
        return false;
    }

    ComponentStores.Add(&Store);
    SortStores();
    return true;
}

bool FGenesisEntityRegistry::UnregisterComponentStore(const FName StoreId)
{
    return ComponentStores.RemoveAll([StoreId](const IGenesisComponentStore* Store)
    {
        return Store->GetStoreId() == StoreId;
    }) > 0;
}

TArray<FGenesisComponentStoreDiagnostics> FGenesisEntityRegistry::GetDiagnostics() const
{
    TArray<FGenesisComponentStoreDiagnostics> Result;
    Result.Reserve(ComponentStores.Num());
    for (const IGenesisComponentStore* Store : ComponentStores)
    {
        FGenesisComponentStoreDiagnostics Diagnostics;
        Diagnostics.StoreId = Store->GetStoreId();
        Diagnostics.ComponentCount = Store->GetComponentCount();
        Diagnostics.AllocatedBytes = static_cast<int64>(Store->GetAllocatedSize());
        Result.Add(Diagnostics);
    }
    return Result;
}

FGenesisEntityRegistrySnapshot FGenesisEntityRegistry::CaptureSnapshot() const
{
    FGenesisEntityRegistrySnapshot Snapshot;
    Snapshot.Generator = Generator.CaptureState();
    Snapshot.ActiveEntities = ActiveEntities;
    return Snapshot;
}

bool FGenesisEntityRegistry::RestoreSnapshot(const FGenesisEntityRegistrySnapshot& Snapshot)
{
    if (Snapshot.SchemaVersion != FGenesisEntityRegistrySnapshot::CurrentSchemaVersion ||
        Snapshot.Generator.SchemaVersion != FGenesisEntityGeneratorState::CurrentSchemaVersion ||
        Snapshot.Generator.NextValue <= 0)
    {
        return false;
    }

    for (int32 Index = 0; Index < Snapshot.ActiveEntities.Num(); ++Index)
    {
        const FGenesisEntityId EntityId = Snapshot.ActiveEntities[Index];
        if (!EntityId.IsValid() ||
            EntityId.Value >= Snapshot.Generator.NextValue ||
            (Index > 0 && !(Snapshot.ActiveEntities[Index - 1] < EntityId)))
        {
            return false;
        }
    }

    FGenesisEntityIdGenerator RestoredGenerator;
    if (!RestoredGenerator.RestoreState(Snapshot.Generator))
    {
        return false;
    }

    Generator = RestoredGenerator;
    ActiveEntities = Snapshot.ActiveEntities;
    PendingCreates.Reset();
    PendingDestroys.Reset();
    return true;
}

void FGenesisEntityRegistry::SortStores()
{
    ComponentStores.Sort([](const IGenesisComponentStore& Left, const IGenesisComponentStore& Right)
    {
        return Left.GetStoreId().LexicalLess(Right.GetStoreId());
    });
}
