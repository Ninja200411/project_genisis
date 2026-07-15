#include "GenesisResourceStack.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisResourceStackSplitMergeTest,
    "Genesis.Core.Resources.SplitMergeConservesQuantity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisResourceStackSplitMergeTest::RunTest(const FString& Parameters)
{
    FGenesisResourceStack Original;
    Original.ResourceId = TEXT("resource:iron");
    Original.Quantity = FGenesisFixedPoint::FromWhole(10);
    Original.BatchId = FGuid::NewGuid();
    Original.OriginPermille.Add(TEXT("mine:alpha"), 1000);

    FGenesisResourceStack Split;
    TestTrue(TEXT("Split succeeds"), Original.Split(FGenesisFixedPoint::FromWhole(4), Split));
    TestEqual(TEXT("Remaining quantity"), Original.Quantity.Raw, int64(6000));
    TestEqual(TEXT("Split quantity"), Split.Quantity.Raw, int64(4000));
    TestTrue(TEXT("Merge succeeds"), Original.Merge(Split));
    TestEqual(TEXT("Total quantity conserved"), Original.Quantity.Raw, int64(10000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGenesisResourceStackInvalidQuantityTest,
    "Genesis.Core.Resources.RejectsInvalidQuantity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisResourceStackInvalidQuantityTest::RunTest(const FString& Parameters)
{
    FGenesisResourceStack Stack;
    Stack.ResourceId = TEXT("resource:water");
    Stack.Quantity = FGenesisFixedPoint::FromRaw(-1);
    Stack.BatchId = FGuid::NewGuid();
    TestFalse(TEXT("Negative quantities are invalid"), Stack.IsValid());
    return true;
}

#endif
