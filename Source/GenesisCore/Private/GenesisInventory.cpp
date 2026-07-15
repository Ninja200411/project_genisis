#include "GenesisInventory.h"

FGenesisInventory::FGenesisInventory(FGenesisInventoryCapacity InCapacity)
    : Capacity(MoveTemp(InCapacity))
{
}

bool FGenesisInventory::Add(
    const FGenesisResourceStack& Stack,
    const FGenesisFixedPoint UnitMass,
    const FGenesisFixedPoint UnitVolume,
    const FName StorageClass)
{
    if (!Stack.IsValid() || UnitMass.Raw < 0 || UnitVolume.Raw < 0 ||
        (!Capacity.AllowedStorageClasses.IsEmpty() && !Capacity.AllowedStorageClasses.Contains(StorageClass)))
    {
        return false;
    }

    const int64 AddedMass = (Stack.Quantity.Raw * UnitMass.Raw) / FGenesisFixedPoint::Scale;
    const int64 AddedVolume = (Stack.Quantity.Raw * UnitVolume.Raw) / FGenesisFixedPoint::Scale;
    if (UsedMass.Raw + AddedMass > Capacity.Mass.Raw || UsedVolume.Raw + AddedVolume > Capacity.Volume.Raw)
    {
        return false;
    }

    Stored.FindOrAdd(Stack.ResourceId).Raw += Stack.Quantity.Raw;
    UsedMass.Raw += AddedMass;
    UsedVolume.Raw += AddedVolume;
    return true;
}

bool FGenesisInventory::Remove(const FName ResourceId, const FGenesisFixedPoint Quantity)
{
    if (Quantity.Raw <= 0 || GetAvailable(ResourceId).Raw < Quantity.Raw)
    {
        return false;
    }

    FGenesisFixedPoint* Current = Stored.Find(ResourceId);
    Current->Raw -= Quantity.Raw;
    if (Current->Raw == 0)
    {
        Stored.Remove(ResourceId);
    }
    return true;
}

FGenesisFixedPoint FGenesisInventory::GetStored(const FName ResourceId) const
{
    return Stored.FindRef(ResourceId);
}

FGenesisFixedPoint FGenesisInventory::GetReserved(const FName ResourceId) const
{
    int64 Raw = 0;
    for (const TPair<FGuid, FGenesisReservation>& Pair : Reservations)
    {
        if (Pair.Value.ResourceId == ResourceId)
        {
            Raw += Pair.Value.Quantity.Raw;
        }
    }
    return FGenesisFixedPoint::FromRaw(Raw);
}

FGenesisFixedPoint FGenesisInventory::GetAvailable(const FName ResourceId) const
{
    return GetStored(ResourceId) - GetReserved(ResourceId);
}

bool FGenesisInventory::Reserve(const FGenesisReservation& Reservation)
{
    if (!Reservation.ReservationId.IsValid() || !Reservation.OwnerId.IsValid() || Reservation.ResourceId.IsNone() ||
        Reservation.Quantity.Raw <= 0 || Reservations.Contains(Reservation.ReservationId) ||
        GetAvailable(Reservation.ResourceId).Raw < Reservation.Quantity.Raw)
    {
        return false;
    }

    Reservations.Add(Reservation.ReservationId, Reservation);
    return true;
}

bool FGenesisInventory::CommitReservation(const FGuid ReservationId)
{
    const FGenesisReservation* Reservation = Reservations.Find(ReservationId);
    if (Reservation == nullptr)
    {
        return false;
    }

    const FGenesisReservation Copy = *Reservation;
    Reservations.Remove(ReservationId);
    if (!Remove(Copy.ResourceId, Copy.Quantity))
    {
        Reservations.Add(ReservationId, Copy);
        return false;
    }
    return true;
}

bool FGenesisInventory::RollbackReservation(const FGuid ReservationId)
{
    return Reservations.Remove(ReservationId) == 1;
}

int32 FGenesisInventory::ReleaseExpired(const int64 CurrentTick)
{
    TArray<FGuid> Expired;
    for (const TPair<FGuid, FGenesisReservation>& Pair : Reservations)
    {
        if (Pair.Value.ExpiryTick <= CurrentTick)
        {
            Expired.Add(Pair.Key);
        }
    }
    Expired.Sort([](const FGuid& Left, const FGuid& Right) { return Left < Right; });
    for (const FGuid& Id : Expired)
    {
        Reservations.Remove(Id);
    }
    return Expired.Num();
}

bool FGenesisInventoryTransaction::Stage(FGenesisInventory& Inventory, const FGenesisReservation& Reservation)
{
    if (!Inventory.Reserve(Reservation))
    {
        Rollback();
        return false;
    }
    Staged.Add({&Inventory, Reservation.ReservationId});
    return true;
}

bool FGenesisInventoryTransaction::Commit()
{
    for (const FStagedReservation& Item : Staged)
    {
        if (Item.Inventory == nullptr || !Item.Inventory->CommitReservation(Item.ReservationId))
        {
            Rollback();
            return false;
        }
    }
    Staged.Reset();
    return true;
}

void FGenesisInventoryTransaction::Rollback()
{
    for (const FStagedReservation& Item : Staged)
    {
        if (Item.Inventory != nullptr)
        {
            Item.Inventory->RollbackReservation(Item.ReservationId);
        }
    }
    Staged.Reset();
}
