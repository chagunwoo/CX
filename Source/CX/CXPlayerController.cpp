#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "UI/CXMatchHUD.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Subsystem/ChatSubsystem.h"

void ACXPlayerController::STCChatLog_Implementation(const TArray<FChatData>& ChatLog)
{
    PendingChat = ChatLog;
    if (VehicleUI)
    {
        VehicleUI->ClearChat();
        for (const FChatData& Entry : PendingChat) VehicleUI->OnNewChatMessage(Entry.Sender, Entry.Message);
        PendingChat.Reset();
    }
}
void ACXPlayerController::CTSChatMessage_Implementation(const FString& Message)
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (!PlayerState || Now - LastChatTime < .25 || Message.Len() > 256) return;
    FString Clean = Message.TrimStartAndEnd().Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" "));
    if (Clean.IsEmpty()) return;
    LastChatTime = Now;
    const FString Sender = PlayerState->GetPlayerName();
    if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>()) Chat->AddChatMessage(Sender, Clean);
    for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It) It->STCChatMessage(Sender, Clean);
}
void ACXPlayerController::STCChatMessage_Implementation(const FString& Sender, const FString& Message)
{
    if (VehicleUI) VehicleUI->OnNewChatMessage(Sender, Message);
    else
    {
        PendingChat.Emplace(Sender, Message);
        if (PendingChat.Num() > 100) PendingChat.RemoveAt(0);
    }
}
void ACXPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController() || !GetLocalPlayer()) return;
    VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass ? VehicleUIClass.Get() : UCXUI::StaticClass());
    if (VehicleUI)
    {
        VehicleUI->AddToPlayerScreen();
        for (const FChatData& Entry : PendingChat) VehicleUI->OnNewChatMessage(Entry.Sender, Entry.Message);
        PendingChat.Reset();
    }
    MatchHUD = CreateWidget<UCXMatchHUD>(this);
    if (MatchHUD) MatchHUD->AddToPlayerScreen(10);
    SetInputMode(FInputModeGameOnly());
}
void ACXPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (VehicleUI) VehicleUI->RemoveFromParent();
    if (MatchHUD) MatchHUD->RemoveFromParent();
    Super::EndPlay(Reason);
}
void ACXPlayerController::FocusChat() { if (VehicleUI) VehicleUI->FocusChat(); }
void ACXPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACXPlayerController::FocusChat);
    if (UEnhancedInputLocalPlayerSubsystem* Inputs = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InputMappingContext) Inputs->AddMappingContext(InputMappingContext, 0);
        if (bUseSteeringWheelControls && SteeringWheelInputMappingContext) Inputs->AddMappingContext(SteeringWheelInputMappingContext, 1);
    }
}
void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXPlayerController, VehiclePawn);
}
void ACXPlayerController::Tick(float Delta)
{
    Super::Tick(Delta);
    ACXPawn* Vehicle = Cast<ACXPawn>(GetPawn());
    if (IsValid(Vehicle) && VehicleUI)
    {
        VehicleUI->UpdateSpeed(Vehicle->GetChaosVehicleMovement()->GetForwardSpeed());
        VehicleUI->UpdateGear(Vehicle->GetChaosVehicleMovement()->GetCurrentGear());
    }
}
void ACXPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    VehiclePawn = Cast<ACXPawn>(InPawn);
}
