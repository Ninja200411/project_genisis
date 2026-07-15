#pragma once

#include "CoreMinimal.h"
#include "GenesisReadModels.generated.h"

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisInspectorValue
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Key;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Label;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Value;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Unit;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Explanation;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName RelatedObjectId;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisInspectorSection
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Title;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FGenesisInspectorValue> Values;
};

USTRUCT(BlueprintType)
struct GENESISSIMULATION_API FGenesisObjectReadModel
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Version = 1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ObjectId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ObjectType;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Status;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 SnapshotTick = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FGenesisInspectorSection> Sections;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FName> AvailableCommands;
};

class GENESISSIMULATION_API IGenesisReadModelPresenter
{
public:
    virtual ~IGenesisReadModelPresenter() = default;
    virtual FName SupportedType() const = 0;
    virtual void Extend(FGenesisObjectReadModel& Model) const = 0;
};

class GENESISSIMULATION_API FGenesisReadModelRegistry
{
public:
    void Register(TSharedRef<const IGenesisReadModelPresenter> Presenter);
    FGenesisObjectReadModel Build(FGenesisObjectReadModel BaseModel) const;

private:
    TMap<FName, TArray<TSharedRef<const IGenesisReadModelPresenter>>> Presenters;
};
