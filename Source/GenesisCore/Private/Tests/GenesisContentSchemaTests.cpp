#if WITH_DEV_AUTOMATION_TESTS

#include "GenesisContentTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisContentSchemaContractsTest,
    "Genesis.Core.Content.SchemaContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisContentSchemaContractsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Current schema major is stable"), FGenesisDefinitionMetadata::CurrentSchemaMajor, 1);
    TestEqual(TEXT("Current schema minor starts at zero"), FGenesisDefinitionMetadata::CurrentSchemaMinor, 0);

    const FGenesisQuantity Mass(5.0, EGenesisPhysicalUnit::Kilogram);
    const FGenesisQuantity Invalid(-1.0, EGenesisPhysicalUnit::Second);
    TestTrue(TEXT("Positive quantity is valid"), Mass.IsPositive());
    TestFalse(TEXT("Negative quantity is rejected"), Invalid.IsNonNegative());

    FGenesisBuildingDefinition Building;
    TestEqual(TEXT("Building time defaults to seconds"), Building.ConstructionTime.Unit, EGenesisPhysicalUnit::Second);
    TestEqual(TEXT("Building power defaults to watts"), Building.PowerDemand.Unit, EGenesisPhysicalUnit::Watt);

    FGenesisVehicleDefinition Vehicle;
    TestEqual(TEXT("Vehicle mass defaults to kilograms"), Vehicle.CargoMass.Unit, EGenesisPhysicalUnit::Kilogram);
    TestEqual(TEXT("Vehicle volume defaults to cubic meters"), Vehicle.CargoVolume.Unit, EGenesisPhysicalUnit::CubicMeter);

    FGenesisTechnologyDefinition Technology;
    TestEqual(TEXT("Research cost defaults to credits"), Technology.ResearchCost.Unit, EGenesisPhysicalUnit::Credit);
    TestEqual(TEXT("Research time defaults to seconds"), Technology.ResearchTime.Unit, EGenesisPhysicalUnit::Second);

    FGenesisBuildingId BuildingId;
    BuildingId.Value = TEXT("building:smelter_mk1");
    FGenesisResourceId ResourceId;
    ResourceId.Value = TEXT("resource:iron_ore");
    TestTrue(TEXT("Building ID is strongly typed"), BuildingId.IsValid());
    TestTrue(TEXT("Resource ID is strongly typed"), ResourceId.IsValid());
    return true;
}

#endif
