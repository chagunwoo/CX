#include "ChatUI.h"
#include "CXPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UChatUI::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
        WidgetTree->RootWidget = Root;
        ChatScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatScrollBox"));
        Root->AddChildToVerticalBox(ChatScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        MultiLineEditableTextBox_ChatBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("MultiLineEditableTextBox_ChatBox"));
        ChatScrollBox->AddChild(MultiLineEditableTextBox_ChatBox);
        EditableTextBox_ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("EditableTextBox_ChatInput"));
        EditableTextBox_ChatInput->SetHintText(FText::FromString(TEXT("Enter: chat / send")));
        Root->AddChildToVerticalBox(EditableTextBox_ChatInput);
    }
    return Super::RebuildWidget();
}
void UChatUI::NativeConstruct()
{
    Super::NativeConstruct();
    if (EditableTextBox_ChatInput) EditableTextBox_ChatInput->OnTextCommitted.AddUniqueDynamic(this, &UChatUI::OnChatInputTextCommitted);
    if (MultiLineEditableTextBox_ChatBox) MultiLineEditableTextBox_ChatBox->SetIsReadOnly(true);
}
void UChatUI::NativeDestruct()
{
    if (EditableTextBox_ChatInput) EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UChatUI::OnChatInputTextCommitted);
    Super::NativeDestruct();
}
void UChatUI::FocusInput()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !EditableTextBox_ChatInput) return;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(EditableTextBox_ChatInput->TakeWidget());
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
    EditableTextBox_ChatInput->SetUserFocus(PC);
}
void UChatUI::ClearMessages()
{
    Messages.Reset();
    if (MultiLineEditableTextBox_ChatBox) MultiLineEditableTextBox_ChatBox->SetText(FText::GetEmpty());
}
void UChatUI::OnNewChatMessage(const FString& Sender, const FString& Message)
{
    Messages.Add(Sender + TEXT(": ") + Message);
    if (Messages.Num() > 100) Messages.RemoveAt(0);
    if (MultiLineEditableTextBox_ChatBox) MultiLineEditableTextBox_ChatBox->SetText(FText::FromString(FString::Join(Messages, TEXT("\n"))));
    if (ChatScrollBox) ChatScrollBox->ScrollToEnd();
}
void UChatUI::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod != ETextCommit::OnEnter && CommitMethod != ETextCommit::OnCleared) return;
    ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>();
    if (PC && CommitMethod == ETextCommit::OnEnter)
    {
        const FString Message = Text.ToString().TrimStartAndEnd().Left(256);
        if (!Message.IsEmpty()) PC->CTSChatMessage(Message);
    }
    if (EditableTextBox_ChatInput) EditableTextBox_ChatInput->SetText(FText::GetEmpty());
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

