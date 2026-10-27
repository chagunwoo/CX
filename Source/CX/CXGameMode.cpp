#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "PlayerState/CXPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Subsystem/ChatSubsystem.h"
#include "TimerManager.h"
ACXGameMode::ACXGameMode()
{
    PlayerControllerClass = ACXPlayerController::StaticClass();
    PlayerStateClass = ACXPlayerState::StaticClass();
    GameStateClass = ACXGameState::StaticClass();
}
void ACXGameMode::InitGameState()
{
    Super::InitGameState();
    if (ACXGameState* GS = GetGameState<ACXGameState>())
    {
        GS->MatchPhase = ECXMatchPhase::Waiting;
        GS->RemainingTime = FMath::Max(1, WaitingDuration);
        GS->WinnerPlayerIds.Reset();
        GS->WinnerNames.Reset();
    }
}
void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (ACXGameState* GS = GetGameState<ACXGameState>())
    {
        if (GS->MatchPhase == ECXMatchPhase::Waiting && !GetWorldTimerManager().IsTimerActive(MatchTimer))
            GetWorldTimerManager().SetTimer(MatchTimer, this, &ACXGameMode::AdvanceMatch, 1.f, true);
    }
    if (ACXPlayerController* PC = Cast<ACXPlayerController>(NewPlayer))
        if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
            PC->STCChatLog(Chat->GetChatLog());
}
void ACXGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    // Logout runs before the departing controller is removed from the world.
    bool bHasOtherPlayers = false;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (IsValid(PC) && PC != Exiting && PC->PlayerState && !MustSpectate(PC))
        {
            bHasOtherPlayers = true;
            break;
        }
    }
    if (!bHasOtherPlayers)
    {
        GetWorldTimerManager().ClearTimer(MatchTimer);
        InitGameState();
    }
}
void ACXGameMode::AdvanceMatch()
{
    ACXGameState* GS = GetGameState<ACXGameState>();
    if (!GS || GS->MatchPhase == ECXMatchPhase::Finished) return;
    GS->RemainingTime = FMath::Max(0, GS->RemainingTime - 1);
    if (GS->RemainingTime == 0)
    {
        if (GS->MatchPhase == ECXMatchPhase::Waiting)
        {
            GS->MatchPhase = ECXMatchPhase::Playing;
            GS->RemainingTime = FMath::Max(1, PlayingDuration);
        }
        else FinishMatch();
    }
    GS->ForceNetUpdate();
}
void ACXGameMode::FinishMatch()
{
    ACXGameState* GS = GetGameState<ACXGameState>();
    if (!GS) return;
    GS->MatchPhase = ECXMatchPhase::Finished;
    GS->RemainingTime = 0;
    GS->WinnerPlayerIds.Reset();
    GS->WinnerNames.Reset();
    int32 BestCoins = -1;
    for (APlayerState* State : GS->PlayerArray)
    {
        ACXPlayerState* PS = Cast<ACXPlayerState>(State);
        if (!PS || PS->IsOnlyASpectator() || PS->IsInactive()) continue;
        if (PS->GetCoins() > BestCoins)
        {
            BestCoins = PS->GetCoins();
            GS->WinnerPlayerIds.Reset();
            GS->WinnerNames.Reset();
        }
        if (PS->GetCoins() == BestCoins)
        {
            GS->WinnerPlayerIds.Add(PS->GetPlayerId());
            GS->WinnerNames.Add(PS->GetPlayerName());
        }
    }
    GetWorldTimerManager().ClearTimer(MatchTimer);
    GS->ForceNetUpdate();
}
void ACXGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(MatchTimer);
    Super::EndPlay(Reason);
}
AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    TArray<APlayerStart*> Starts;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        if (IsPlayerStartAvailable(*It)) Starts.Add(*It);
    if (Starts.IsEmpty()) return Super::ChoosePlayerStart_Implementation(Player);
    return Starts[FMath::RandHelper(Starts.Num())];
}
bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* Start) const
{
    if (!IsValid(Start) || !GetWorld()) return false;
    const FTransform Transform = Start->GetActorTransform();
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Vehicle);
    return !GetWorld()->OverlapAnyTestByObjectType(Transform.TransformPosition(VehicleSpawnCheckOffset),
        Transform.GetRotation(), Objects, FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent),
        FCollisionQueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false));
}
