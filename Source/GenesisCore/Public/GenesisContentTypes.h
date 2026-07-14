#pragma once

#include "CoreMinimal.h"
#include "GenesisContentTypes.generated.h"

UENUM(BlueprintType)
enum class EGenesisPhysicalUnit : uint8
{
    Kilogram,
    CubicMeter,
    Joule,
    Watt,
    Second,
    Credit
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisQuantity
{
    GENERATED_BODY()

    FGenesisQuantity() = default;
    FGenesisQuantity(const double InValue, const EGenesisPhysicalUnit InUnit) : Value(InValue), Unit(InUnit) {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content")
    double Value = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content")
    EGenesisPhysicalUnit Unit = EGenesisPhysicalUnit::Kilogram;

    bool IsNonNegative() const { return FMath::IsFinite(Value) && Value >= 0.0; }
    bool IsPositive() const { return FMath::IsFinite(Value) && Value > 0.0; }
};

#define GENESIS_DECLARE_CONTENT_ID(TypeName) \
USTRUCT(BlueprintType) \
struct GENESISCORE_API TypeName \
{ \
    GENERATED_BODY() \
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") \
    FName Value; \
    bool IsValid() const { return !Value.IsNone(); } \
    friend bool operator==(const TypeName& A, const TypeName& B) { return A.Value == B.Value; } \
};

GENESIS_DECLARE_CONTENT_ID(FGenesisBuildingId)
GENESIS_DECLARE_CONTENT_ID(FGenesisResourceId)
GENESIS_DECLARE_CONTENT_ID(FGenesisPersonRoleId)
GENESIS_DECLARE_CONTENT_ID(FGenesisVehicleId)
GENESIS_DECLARE_CONTENT_ID(FGenesisRecipeId)
GENESIS_DECLARE_CONTENT_ID(FGenesisTechnologyId)

#undef GENESIS_DECLARE_CONTENT_ID

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisDefinitionMetadata
{
    GENERATED_BODY()

    static constexpr int32 CurrentSchemaMajor = 1;
    static constexpr int32 CurrentSchemaMinor = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") int32 SchemaMajor = CurrentSchemaMajor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") int32 SchemaMinor = CurrentSchemaMinor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FName> Tags;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisResourceAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisResourceId Resource;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity Amount;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisBuildingDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisBuildingId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisResourceAmount> ConstructionCost;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity ConstructionTime = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Second);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity PowerDemand = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Watt);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisTechnologyId> RequiredTechnologies;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisResourceDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisResourceId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") EGenesisPhysicalUnit StorageUnit = EGenesisPhysicalUnit::Kilogram;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity UnitMass = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Kilogram);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity UnitVolume = FGenesisQuantity(0.0, EGenesisPhysicalUnit::CubicMeter);
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisPersonRoleDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisPersonRoleId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity WagePerSecond = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Credit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisTechnologyId> RequiredTechnologies;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisVehicleDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisVehicleId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity CargoMass = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Kilogram);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity CargoVolume = FGenesisQuantity(0.0, EGenesisPhysicalUnit::CubicMeter);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity PowerDemand = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Watt);
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisRecipeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisRecipeId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisResourceAmount> Inputs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisResourceAmount> Outputs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity Duration = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Second);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisBuildingId> SupportedBuildings;
};

USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisTechnologyDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisTechnologyId Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisDefinitionMetadata Metadata;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity ResearchCost = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Credit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") FGenesisQuantity ResearchTime = FGenesisQuantity(0.0, EGenesisPhysicalUnit::Second);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Content") TArray<FGenesisTechnologyId> Prerequisites;
};
