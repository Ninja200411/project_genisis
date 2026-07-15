#include "GenesisDiagnostics.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FGenesisDiagnosticReport::FGenesisDiagnosticReport(FGenesisExecutionContext InContext)
    : Context(MoveTemp(InContext))
{
}

void FGenesisDiagnosticReport::Add(FGenesisDiagnostic Diagnostic)
{
    Diagnostics.Add(MoveTemp(Diagnostic));
}

void FGenesisDiagnosticReport::AddTelemetry(FGenesisTelemetrySample Sample)
{
    Telemetry.Add(MoveTemp(Sample));
}

bool FGenesisDiagnosticReport::HasErrors() const
{
    return Diagnostics.ContainsByPredicate([](const FGenesisDiagnostic& Item)
    {
        return Item.Severity == EGenesisDiagnosticSeverity::Error || Item.Severity == EGenesisDiagnosticSeverity::Fatal;
    });
}

TMap<FString, TArray<FGenesisDiagnostic>> FGenesisDiagnosticReport::GroupBySource() const
{
    TMap<FString, TArray<FGenesisDiagnostic>> Groups;
    for (const FGenesisDiagnostic& Item : Diagnostics)
    {
        Groups.FindOrAdd(Item.SourceFile).Add(Item);
    }
    return Groups;
}

FString FGenesisDiagnosticReport::ToConsoleText() const
{
    TArray<FGenesisDiagnostic> Ordered = Diagnostics;
    Ordered.Sort([](const FGenesisDiagnostic& A, const FGenesisDiagnostic& B)
    {
        if (A.SourceFile != B.SourceFile) return A.SourceFile < B.SourceFile;
        if (A.ObjectId != B.ObjectId) return A.ObjectId.LexicalLess(B.ObjectId);
        return A.Code.LexicalLess(B.Code);
    });

    FString Result;
    for (const FGenesisDiagnostic& Item : Ordered)
    {
        Result += FString::Printf(
            TEXT("[%s] %s %s:%s id=%s rule=%s - %s Hint: %s\n"),
            Item.Severity == EGenesisDiagnosticSeverity::Info ? TEXT("INFO") :
            Item.Severity == EGenesisDiagnosticSeverity::Warning ? TEXT("WARN") :
            Item.Severity == EGenesisDiagnosticSeverity::Fatal ? TEXT("FATAL") : TEXT("ERROR"),
            *Item.Code.ToString(), *Item.SourceFile, *Item.FieldPath, *Item.ObjectId.ToString(),
            *Item.Rule.ToString(), *Item.Message, *Item.CorrectionHint);
    }
    return Result;
}

FString FGenesisDiagnosticReport::ToJson() const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("build"), Context.BuildVersion);
    Root->SetStringField(TEXT("contentHash"), Context.ContentHash);
    Root->SetNumberField(TEXT("seed"), static_cast<double>(Context.Seed));
    Root->SetNumberField(TEXT("tick"), static_cast<double>(Context.Tick));

    TArray<TSharedPtr<FJsonValue>> DiagnosticValues;
    for (const FGenesisDiagnostic& Item : Diagnostics)
    {
        TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
        Json->SetNumberField(TEXT("severity"), static_cast<int32>(Item.Severity));
        Json->SetStringField(TEXT("code"), Item.Code.ToString());
        Json->SetStringField(TEXT("file"), Item.SourceFile);
        Json->SetStringField(TEXT("objectId"), Item.ObjectId.ToString());
        Json->SetStringField(TEXT("fieldPath"), Item.FieldPath);
        Json->SetStringField(TEXT("rule"), Item.Rule.ToString());
        Json->SetStringField(TEXT("message"), Item.Message);
        Json->SetStringField(TEXT("correctionHint"), Item.CorrectionHint);
        DiagnosticValues.Add(MakeShared<FJsonValueObject>(Json));
    }
    Root->SetArrayField(TEXT("diagnostics"), DiagnosticValues);

    TArray<TSharedPtr<FJsonValue>> MetricValues;
    for (const FGenesisTelemetrySample& Sample : Telemetry)
    {
        TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("metric"), Sample.Metric.ToString());
        Json->SetNumberField(TEXT("value"), Sample.Value);
        Json->SetNumberField(TEXT("tick"), static_cast<double>(Sample.Tick));
        MetricValues.Add(MakeShared<FJsonValueObject>(Json));
    }
    Root->SetArrayField(TEXT("telemetry"), MetricValues);

    FString Output;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root, Writer);
    return Output;
}

void FGenesisEventRingBuffer::Push(FString Event)
{
    if (Events.Num() == Capacity) Events.RemoveAt(0, 1, EAllowShrinking::No);
    Events.Add(MoveTemp(Event));
}
