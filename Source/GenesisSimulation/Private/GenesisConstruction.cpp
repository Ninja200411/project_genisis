#include "GenesisConstruction.h"

TArray<FGenesisPlacementViolation> FGenesisPlacementService::Validate(
    const FGenesisPlacementRequest& Request,
    const float MaximumSlopeDegrees)
{
    TArray<FGenesisPlacementViolation> Result;
    const auto Add = [&Result](const FName Code, const FString& Message)
    {
        FGenesisPlacementViolation Violation;
        Violation.Code = Code;
        Violation.Message = Message;
        Result.Add(MoveTemp(Violation));
    };

    if (Request.BuildingId.IsNone()) Add(TEXT("GEN-PLACE-BUILDING"), TEXT("Building ID is required."));
    if (Request.TerrainSlopeDegrees > MaximumSlopeDegrees) Add(TEXT("GEN-PLACE-SLOPE"), TEXT("Terrain slope exceeds the building limit."));
    if (!Request.bHasFoundation) Add(TEXT("GEN-PLACE-FOUNDATION"), TEXT("A compatible foundation is required."));
    if (!Request.bCollisionFree) Add(TEXT("GEN-PLACE-COLLISION"), TEXT("Placement overlaps another object."));
    if (!Request.bHasAccess) Add(TEXT("GEN-PLACE-ACCESS"), TEXT("No valid access route is available."));
    if (!Request.bNetworksAvailable) Add(TEXT("GEN-PLACE-NETWORK"), TEXT("Required network connection is unavailable."));
    if (!Request.bTechnologyUnlocked) Add(TEXT("GEN-PLACE-TECH"), TEXT("Required technology is not unlocked."));
    return Result;
}

void FGenesisConstructionOrder::Initialize(
    const FGuid InOrderId,
    const FName InBuildingId,
    const TArray<FGenesisConstructionPhaseDefinition>& InPhases)
{
    OrderId = InOrderId;
    BuildingId = InBuildingId;
    Phases = InPhases;
    Phases.Sort([](const FGenesisConstructionPhaseDefinition& A, const FGenesisConstructionPhaseDefinition& B)
    {
        return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase);
    });
    PhaseIndex = 0;
    CompletedWork = {};
    Defects.Reset();
    CurrentPhase = Phases.IsEmpty() ? EGenesisConstructionPhase::Completed : Phases[0].Phase;
}

bool FGenesisConstructionOrder::DeliverMaterial(const FName ResourceId, const FGenesisFixedPoint Quantity)
{
    if (!Phases.IsValidIndex(PhaseIndex) || Quantity.Raw <= 0) return false;
    for (FGenesisConstructionRequirement& Requirement : Phases[PhaseIndex].Materials)
    {
        if (Requirement.Id == ResourceId)
        {
            Requirement.Delivered.Raw = FMath::Min(
                Requirement.Required.Raw,
                Requirement.Delivered.Raw + Quantity.Raw);
            return true;
        }
    }
    return false;
}

bool FGenesisConstructionOrder::AddWork(
    const FGenesisFixedPoint Work,
    const int32 ProductivityPermille,
    const int32 QualityPermille)
{
    if (!Phases.IsValidIndex(PhaseIndex) || Work.Raw <= 0 || ProductivityPermille <= 0) return false;
    const int64 Effective = (Work.Raw * static_cast<int64>(ProductivityPermille)) / 1000;
    CompletedWork.Raw = FMath::Min(Phases[PhaseIndex].RequiredWork.Raw, CompletedWork.Raw + Effective);
    LastQualityPermille = FMath::Clamp(QualityPermille, 0, 1000);
    return true;
}

bool FGenesisConstructionOrder::CompleteInspection(
    const int32 MeasuredQualityPermille,
    const FName Cause,
    const FName RepairRecipeId)
{
    if (!Phases.IsValidIndex(PhaseIndex)) return false;
    if (MeasuredQualityPermille >= Phases[PhaseIndex].RequiredQualityPermille) return true;

    FGenesisConstructionDefect Defect;
    Defect.Id = FGuid::NewGuid();
    Defect.Cause = Cause;
    Defect.SeverityPermille = Phases[PhaseIndex].RequiredQualityPermille - MeasuredQualityPermille;
    Defect.RepairRecipeId = RepairRecipeId;
    Defects.Add(MoveTemp(Defect));
    return false;
}

bool FGenesisConstructionOrder::AdvancePhase()
{
    if (!Phases.IsValidIndex(PhaseIndex)) return false;
    for (const FGenesisConstructionRequirement& Requirement : Phases[PhaseIndex].Materials)
    {
        if (Requirement.Delivered.Raw < Requirement.Required.Raw) return false;
    }
    if (CompletedWork.Raw < Phases[PhaseIndex].RequiredWork.Raw) return false;
    if (LastQualityPermille < Phases[PhaseIndex].RequiredQualityPermille) return false;
    for (const FGenesisConstructionDefect& Defect : Defects)
    {
        if (!Defect.bResolved) return false;
    }

    ++PhaseIndex;
    CompletedWork = {};
    CurrentPhase = Phases.IsValidIndex(PhaseIndex) ? Phases[PhaseIndex].Phase : EGenesisConstructionPhase::Completed;
    return true;
}

bool FGenesisConstructionOrder::Cancel()
{
    if (CurrentPhase == EGenesisConstructionPhase::Completed) return false;
    CurrentPhase = EGenesisConstructionPhase::Cancelled;
    return true;
}
