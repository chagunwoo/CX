// Opt-in development-only two-client smoke test. Normal games never execute this.
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "CXUI.h"
#include "UI/ChatUI.h"
#include "PlayerState/CXPlayerState.h"
#include "Subsystem/ChatSubsystem.h"
#include "Items/CXPickup.h"

struct FCXNetworkSmokeState
{
    FDelegateHandle Handle;
    double Start = 0;
    double FinishedAt = 0;
    double LastDiagnostic = 0;
    bool bPrepared = false;
    bool bSent = false;
    bool bSawPrivateStats = false;
    bool bDone = false;
    bool bServerPassed = false;
    FCXNetworkSmokeState()
    {
        Handle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FCXNetworkSmokeState::Tick);
    }
    ~FCXNetworkSmokeState() { FWorldDelegates::OnWorldTickStart.Remove(Handle); }
    void Complete(bool bSuccess, const TCHAR* Reason)
    {
        if (bDone) return;
        bDone = true;
        UE_LOG(LogTemp, Display, TEXT("CX_NETWORK_SMOKE %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Reason);
        FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
    }
    void Tick(UWorld* World, ELevelTick, float)
    {
        if (bDone || !FParse::Param(FCommandLine::Get(), TEXT("CXNetworkSmoke")) || !World->IsGameWorld()) return;
        const double Now = FPlatformTime::Seconds();
        if (Start == 0) Start = Now;
        if (Now - Start > 240) { Complete(false, TEXT("Timeout waiting for two players / replication")); return; }
        ACXGameState* GS = World->GetGameState<ACXGameState>();
        if (!GS || !World->HasBegunPlay()) return;
        if (Now - LastDiagnostic > 5)
        {
            LastDiagnostic = Now;
            ACXPlayerController* DebugPC = Cast<ACXPlayerController>(World->GetFirstPlayerController());
            UChatUI* DebugChat = DebugPC && DebugPC->VehicleUI ? DebugPC->VehicleUI->UI_Chat : nullptr;
            const ACXPlayerState* DebugPS = DebugPC ? DebugPC->GetPlayerState<ACXPlayerState>() : nullptr;
            UE_LOG(LogTemp, Display, TEXT("CX_NETWORK_PROGRESS mode=%d phase=%d time=%d players=%d PC=%d HUD=%d UI=%d Chat=%d input=%d log=%d scroll=%d ownCoins=%d bonus=%.0f"),
                int32(World->GetNetMode()), int32(GS->MatchPhase), GS->RemainingTime, GS->PlayerArray.Num(), DebugPC != nullptr, DebugPC && DebugPC->MatchHUD,
                DebugPC && DebugPC->VehicleUI, DebugChat != nullptr, DebugChat && DebugChat->EditableTextBox_ChatInput, DebugChat && DebugChat->MultiLineEditableTextBox_ChatBox,
                DebugChat && DebugChat->ChatScrollBox, DebugPS ? DebugPS->GetCoins() : -1, DebugPS ? DebugPS->GetBonusMaxSpeed() : -1.f);
        }
        if (World->GetNetMode() != NM_Client)
        {
            if (FinishedAt != 0)
            {
                if (Now - FinishedAt > 8) Complete(bServerPassed, TEXT("Server chat RPCs, timer and tied winners"));
                return;
            }
            TArray<ACXPlayerState*> Players;
            for (APlayerState* PS : GS->PlayerArray)
                if (ACXPlayerState* CX = Cast<ACXPlayerState>(PS)) Players.Add(CX);
            if (Players.Num() != 2) return;
            Players.Sort([](const ACXPlayerState& A, const ACXPlayerState& B) { return A.GetPlayerId() < B.GetPlayerId(); });
            if (!bPrepared)
            {
                if (!Players[0]->GetPawn() || !Players[1]->GetPawn()) return;
                Players[0]->AddCoins(25); Players[0]->AddBonusMaxSpeed(10);
                Players[1]->AddCoins(10); Players[1]->AddBonusMaxSpeed(20);
                GS->MatchPhase = ECXMatchPhase::Playing;
                GS->RemainingTime = 30;
                GS->ForceNetUpdate();
                bPrepared = true;
            }
            UChatSubsystem* Chat = World->GetSubsystem<UChatSubsystem>();
            int32 Messages = 0;
            for (const FChatData& Entry : Chat->GetChatLog()) if (Entry.Message.StartsWith(TEXT("CX_NET_"))) ++Messages;
            if (Messages >= 2 && Players[1]->GetCoins() == 10)
            {
                Players[1]->AddCoins(15);
                GS->RemainingTime = 2;
                GS->ForceNetUpdate();
            }
            if (GS->MatchPhase == ECXMatchPhase::Finished)
            {
                FinishedAt = Now;
                bServerPassed = Messages >= 2 && GS->WinnerPlayerIds.Num() == 2;
            }
            return;
        }
        ACXPlayerController* PC = Cast<ACXPlayerController>(World->GetFirstPlayerController());
        ACXPlayerState* Own = PC ? PC->GetPlayerState<ACXPlayerState>() : nullptr;
        if (!Own || !PC->VehicleUI || !PC->MatchHUD) return;
        UChatUI* ChatUI = PC->VehicleUI->UI_Chat;
        if (!ChatUI || !ChatUI->EditableTextBox_ChatInput || !ChatUI->MultiLineEditableTextBox_ChatBox || !ChatUI->ChatScrollBox) return;
        bool bPrivacy = true;
        for (APlayerState* PS : GS->PlayerArray)
            if (ACXPlayerState* Other = Cast<ACXPlayerState>(PS); Other && Other != Own)
                bPrivacy &= Other->GetCoins() == 0 && Other->GetBonusMaxSpeed() == 0.f;
        if (Own->GetCoins() > 0 && Own->GetBonusMaxSpeed() > 0 && bPrivacy) bSawPrivateStats = true;
        if (!bSent && bSawPrivateStats && GS->IsPlaying())
        {
            PC->CTSChatMessage(FString::Printf(TEXT("CX_NET_%d"), Own->GetPlayerId()));
            bSent = true;
        }
        if (GS->MatchPhase == ECXMatchPhase::Finished && GS->WinnerPlayerIds.Num() == 2)
        {
            int32 Messages = 0;
            for (const FString& Line : ChatUI->Messages) if (Line.Contains(TEXT("CX_NET_"))) ++Messages;
            int32 Pickups = 0;
            for (TActorIterator<ACXPickup> It(World); It; ++It) ++Pickups;
            const bool bSuccess = bSawPrivateStats && bPrivacy && Own->GetCoins() == 25 && Messages == 2 && GS->RemainingTime == 0 && GS->WinnerPlayerIds.Contains(Own->GetPlayerId()) && Pickups == 12;
            UE_LOG(LogTemp, Display, TEXT("CX_NETWORK_DETAILS coins=%d bonus=%.0f privacy=%d messages=%d pickups=%d winners=%d"), Own->GetCoins(), Own->GetBonusMaxSpeed(), bPrivacy, Messages, Pickups, GS->WinnerPlayerIds.Num());
            Complete(bSuccess, TEXT("Client owner-only stats, real widget bindings, chat broadcast, replicated pickups and results"));
        }
    }
};
static FCXNetworkSmokeState GCXNetworkSmoke;
#endif
