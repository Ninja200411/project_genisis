#pragma once

#include "CoreMinimal.h"
#include "GenesisFixedPoint.h"
#include "GenesisResourceStack.h"
#include "GenesisInventory.generated.h"

UENUM(BlueprintType)
enum class EGenesisReservationPurpose : uint8
{
    Input,
    Output,
    Transport,
    Construction
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisInventoryCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Mass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Volume;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSet<FName> AllowedStorageClasses;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisReservation
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid ReservationId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid OwnerId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ResourceId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGenesisFixedPoint Quantity;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisReservationPurpose Purpose = EGenesisReservationPurpose::Input;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Priority = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 ExpiryTick = MAX_int64;
};

class GENESISCORE_API FGenesisInventory
{
public:
    explicit FGenesisInventory(FGenesisInventoryCapacity InCapacity = {});

    bool Add(const FGenesisResourceStack& Stack, FGenesisFixedPoint UnitMass, FGenesisFixedPoint UnitVolume, FName StorageClass);
    bool Remove(FName ResourceId, FGenesisFixedPoint Quantity);
    FGenesisFixedPoint GetStored(FName ResourceId) const;
    FGenesisFixedPoint GetReserved(FName ResourceId) const;
    FGenesisFixedPoint GetAvailable(FName ResourceId) const;

    bool Reserve(const FGenesisReservation& Reservation);
    bool CommitReservation(FGuid ReservationId);
    bool RollbackReservation(FGuid ReservationId);
    int32 ReleaseExpired(int64 CurrentTick);

    const TMap<FGuid, FGenesisReservation>& GetReservations() const { return Reservations; }

private:
    FGenesisInventoryCapacity Capacity;
    FGenesisFixedPoint UsedMass;
    FGenesisFixedPoint UsedVolume;
    TMap<FName, FGenesisFixedPoint> Stored;
    TMap<FGuid, FGenesisReservation> Reservations;
};

class GENESISCORE_API FGenesisInventoryTransaction
{
public:
    bool Stage(FGenesisInventory& Inventory, const FGenesisReservation& Reservation);
    bool Commit();
    void Rollback();

private:
    struct FStagedReservation
    {
        FGenesisInventory* Inventory = nullptr;
        FGuid ReservationId;
    };
    TArray<FStagedReservation> Staged;
};
