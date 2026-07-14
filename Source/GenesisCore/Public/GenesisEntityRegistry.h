#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisEntityRegistry.generated.h"

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisEntityGeneratorState
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int64 NextValue = 1;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisEntityRegistrySnapshot
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    FGenesisEntityGeneratorState Generator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    TArray<FGenesisEntityId> ActiveEntities;
};

class GENESISCORE_API FGenesisEntityIdGenerator final
{
public:
    FGenesisEntityId Allocate();
    FGenesisEntityGeneratorState CaptureState() const;
    bool RestoreState(const FGenesisEntityGeneratorState& State);

private:
    int64 NextValue = 1;
};

class GENESISCORE_API IGenesisComponentStore
{
public:
    virtual ~IGenesisComponentStore() = default;
    virtual FName GetStoreId() const = 0;
    virtual int32 GetComponentCount() const = 0;
    virtual SIZE_T GetAllocatedSize() const = 0;
    virtual void RemoveEntity(FGenesisEntityId EntityId) = 0;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisComponentStoreDiagnostics
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    FName StoreId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int32 ComponentCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Genesis|Entity")
    int64 AllocatedBytes = 0;
};

class GENESISCORE_API FGenesisEntityRegistry final
{
public:
    FGenesisEntityId QueueCreate();
    bool QueueDestroy(FGenesisEntityId EntityId);
    void ApplyPendingChanges();
    void Reset();

    bool IsAlive(FGenesisEntityId EntityId) const;
    bool IsPendingDestroy(FGenesisEntityId EntityId) const;
    int32 Num() const { return ActiveEntities.Num(); }
    const TArray<FGenesisEntityId>& GetEntities() const { return ActiveEntities; }

    bool RegisterComponentStore(IGenesisComponentStore& Store);
    bool UnregisterComponentStore(FName StoreId);
    TArray<FGenesisComponentStoreDiagnostics> GetDiagnostics() const;

    FGenesisEntityRegistrySnapshot CaptureSnapshot() const;
    bool RestoreSnapshot(const FGenesisEntityRegistrySnapshot& Snapshot);

private:
    void SortStores();

    FGenesisEntityIdGenerator Generator;
    TArray<FGenesisEntityId> ActiveEntities;
    TArray<FGenesisEntityId> PendingCreates;
    TArray<FGenesisEntityId> PendingDestroys;
    TArray<IGenesisComponentStore*> ComponentStores;
};
