#include "GenesisReadModels.h"

void FGenesisReadModelRegistry::Register(TSharedRef<const IGenesisReadModelPresenter> Presenter)
{
    Presenters.FindOrAdd(Presenter->SupportedType()).Add(MoveTemp(Presenter));
}

FGenesisObjectReadModel FGenesisReadModelRegistry::Build(FGenesisObjectReadModel BaseModel) const
{
    if (const TArray<TSharedRef<const IGenesisReadModelPresenter>>* Extensions = Presenters.Find(BaseModel.ObjectType))
    {
        for (const TSharedRef<const IGenesisReadModelPresenter>& Presenter : *Extensions)
        {
            Presenter->Extend(BaseModel);
        }
    }
    BaseModel.Sections.Sort([](const FGenesisInspectorSection& A, const FGenesisInspectorSection& B)
    {
        return A.Id.LexicalLess(B.Id);
    });
    for (FGenesisInspectorSection& Section : BaseModel.Sections)
    {
        Section.Values.Sort([](const FGenesisInspectorValue& A, const FGenesisInspectorValue& B)
        {
            return A.Key.LexicalLess(B.Key);
        });
    }
    BaseModel.AvailableCommands.Sort(FNameLexicalLess());
    return BaseModel;
}
