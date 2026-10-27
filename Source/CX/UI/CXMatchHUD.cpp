#include "UI/CXMatchHUD.h"
#include "CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
TSharedRef<SWidget> UCXMatchHUD::RebuildWidget()
{
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(.015f, .025f, .04f, .85f));
    Panel->SetPadding(FMargin(16.f));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>();
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo Font = StatusText->GetFont();
    Font.Size = 20;
    StatusText->SetFont(Font);
    Panel->SetContent(StatusText);
    UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
    PanelSlot->SetPosition(FVector2D(24.f, 24.f));
    PanelSlot->SetAutoSize(true);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return Super::RebuildWidget();
}
void UCXMatchHUD::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    APlayerController* PC = GetOwningPlayer();
    const ACXPlayerState* PS = PC ? PC->GetPlayerState<ACXPlayerState>() : nullptr;
    const ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>();
    if (!StatusText || !PS || !GS) return;
    const TCHAR* Phase = GS->MatchPhase == ECXMatchPhase::Waiting ? TEXT("Waiting") : GS->IsPlaying() ? TEXT("Playing") : TEXT("Finished");
    FString Text = FString::Printf(TEXT("%s  |  %02d:%02d\nCoins: %d won  |  Bonus max speed: +%.0f km/h\nLMB: attack  |  Enter: chat"), Phase, GS->RemainingTime / 60, GS->RemainingTime % 60, PS->GetCoins(), PS->GetBonusMaxSpeed());
    if (GS->MatchPhase == ECXMatchPhase::Finished)
    {
        const bool bWinner = GS->WinnerPlayerIds.Contains(PS->GetPlayerId());
        Text += FString::Printf(TEXT("\n%s\nWinners: %s"), bWinner ? (GS->WinnerPlayerIds.Num() > 1 ? TEXT("DRAW - SHARED VICTORY") : TEXT("VICTORY")) : TEXT("DEFEAT"), *FString::Join(GS->WinnerNames, TEXT(", ")));
    }
    StatusText->SetText(FText::FromString(Text));
}
