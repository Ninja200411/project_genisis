#pragma once

#include "CoreMinimal.h"
#include "GenesisEntityRegistry.h"

template <typename TComponent>
class TGenesisComponentStore final : public IGenesisComponentStore
{
public:
    explicit TGenesisComponentStore(const FName InStoreId)
        : StoreId(InStoreId)
    {
        check(!StoreId.IsNone());
    }

    virtual FName GetStoreId() const override { return StoreId; }
    virtual int32 GetComponentCount() const override { return SortedEntities.Num(); }
    virtual SIZE_T GetAllocatedSize() const override
    {
        return Components.GetAllocatedSize() + SortedEntities.GetAllocatedSize();
    }

    virtual void RemoveEntity(const FGenesisEntityId EntityId) override
    {
        if (Components.Remove(EntityId) > 0)
        {
            SortedEntities.RemoveSingle(EntityId);
        }
    }

    bool Add(const FGenesisEntityId EntityId, const TComponent& Component)
    {
        if (!EntityId.IsValid() || Components.Contains(EntityId))
        {
            return false;
        }

        Components.Add(EntityId, Component);
        SortedEntities.Add(EntityId);
        SortedEntities.Sort();
        return true;
    }

    bool Add(FGenesisEntityId EntityId, TComponent&& Component)
    {
        if (!EntityId.IsValid() || Components.Contains(EntityId))
        {
            return false;
        }

        Components.Add(EntityId, MoveTemp(Component));
        SortedEntities.Add(EntityId);
        SortedEntities.Sort();
        return true;
    }

    TComponent* Find(const FGenesisEntityId EntityId) { return Components.Find(EntityId); }
    const TComponent* Find(const FGenesisEntityId EntityId) const { return Components.Find(EntityId); }
    bool Contains(const FGenesisEntityId EntityId) const { return Components.Contains(EntityId); }
    const TArray<FGenesisEntityId>& GetEntities() const { return SortedEntities; }

    template <typename TFunction>
    void ForEach(TFunction&& Function)
    {
        for (const FGenesisEntityId EntityId : SortedEntities)
        {
            Function(EntityId, Components.FindChecked(EntityId));
        }
    }

private:
    FName StoreId;
    TMap<FGenesisEntityId, TComponent> Components;
    TArray<FGenesisEntityId> SortedEntities;
};
