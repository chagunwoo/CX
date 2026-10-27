// Copyright Epic Games, Inc. All Rights Reserved.


#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	if (UI_Chat) UI_Chat->OnNewChatMessage(InSender, InMessage);
}

TSharedRef<SWidget> UCXUI::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Root;
		UI_Chat = WidgetTree->ConstructWidget<UChatUI>(UChatUI::StaticClass(), TEXT("UI_Chat"));
		UCanvasPanelSlot* ChatSlot = Root->AddChildToCanvas(UI_Chat);
		ChatSlot->SetAnchors(FAnchors(0.f, 1.f));
		ChatSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChatSlot->SetPosition(FVector2D(24.f, -24.f));
		ChatSlot->SetSize(FVector2D(480.f, 230.f));
	}
	return Super::RebuildWidget();
}
void UCXUI::FocusChat() { if (UI_Chat) UI_Chat->FocusInput(); }
void UCXUI::ClearChat() { if (UI_Chat) UI_Chat->ClearMessages(); }

void UCXUI::NativeConstruct()
{
	Super::NativeConstruct();
	// Existing HUD assets name the child WBP_Chat rather than UI_Chat.
	// Resolve by native widget type so the existing designer layout is preserved.
	if (!UI_Chat && WidgetTree)
	{
		TArray<UWidget*> Widgets;
		WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UChatUI* Chat = Cast<UChatUI>(Widget))
			{
				UI_Chat = Chat;
				break;
			}
		}
	}
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UCXUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}
