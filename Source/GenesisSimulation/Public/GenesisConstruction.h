#pragma once

#include "CoreMinimal.h"
#include "GenesisFixedPoint.h"
#include "GenesisConstruction.generated.h"

UENUM(BlueprintType)
enum class EGenesisConstructionPhase : uint8
{
    Planning,
    Groundwork,
    Structure,
    Envelope,
    Systems,
    Commissioning,
    Completed,
    Cancelled
};

UENUM(BlueprintType)
enum class EGenesisPlacementSeverity : uint8 { Info, Warning, Error };

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisPlacementRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BuildingId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Variant;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TerrainSlopeDegrees = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHasFoundation = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCollisionFree = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHasAccess = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bNetworksAvailable = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bTechnologyUnlocked = true;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisPlacementViolation
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Code;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGenesisPlacementSeverity Severity = EGenesisPlacementSeverity::Error;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Message;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FIntPoint> Cells;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisConstructionRequirement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint Required;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGenesisFixedPoint Delivered;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisConstructionPhaseDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EGenesisConstructionPhase Phase = EGenesisConstructionPhase::Planning;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGenesisConstructionRequirement> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGenesisFixedPoint RequiredWork;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RequiredQualityPermille = 800;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisConstructionDefect
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid Id;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Cause;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SeverityPermille = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName RepairRecipeId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bResolved = false;
};

class GENESISSIMULATION_API FGenesisPlacementService
{
public:
    static TArray<FGenesisPlacementViolation> Validate(const FGenesisPlacementRequest& Request, float MaximumSlopeDegrees);
};

class GENESISSIMULATION_API FGenesisConstructionOrder
{
public:
    void Initialize(FGuid InOrderId, FName InBuildingId, const TArray<FGenesisConstructionPhaseDefinition>& InPhases);
    bool DeliverMaterial(FName ResourceId, FGenesisFixedPoint Quantity);
    bool AddWork(FGenesisFixedPoint Work, int32 ProductivityPermille, int32 QualityPermille);
    bool CompleteInspection(int32 MeasuredQualityPermille, FName Cause, FName RepairRecipeId);
    bool AdvancePhase();
    bool Cancel();
    bool IsComplete() const { return CurrentPhase == EGenesisConstructionPhase::Completed; }
    EGenesisConstructionPhase GetCurrentPhase() const { return CurrentPhase; }
    const TArray<FGenesisConstructionDefect>& GetDefects() const { return Defects; }

private:
    FGuid OrderId;
    FName BuildingId;
    TArray<FGenesisConstructionPhaseDefinition> Phases;
    int32 PhaseIndex = 0;
    EGenesisConstructionPhase CurrentPhase = EGenesisConstructionPhase::Planning;
    FGenesisFixedPoint CompletedWork;
    int32 LastQualityPermille = 1000;
    TArray<FGenesisConstructionDefect> Defects;
};
