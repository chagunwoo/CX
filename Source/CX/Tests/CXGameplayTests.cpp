#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "CXPawn.h"
#include "PlayerState/CXPlayerState.h"
#include "Items/CXPickup.h"
#include "Items/CXItemSpawner.h"
#include "Subsystem/ChatSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCXGameplayTest, "CX.Gameplay.ServerRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCXGameplayTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::SM5, &Init);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    Context.OwningGameInstance = Instance;
    World->SetGameInstance(Instance);
    World->GetWorldSettings()->DefaultGameMode = ACXGameMode::StaticClass();
    World->SetGameMode(FURL());
    World->InitializeActorsForPlay(FURL());
    ACXGameMode* GM = World->GetAuthGameMode<ACXGameMode>();
    ACXGameState* GS = World->GetGameState<ACXGameState>();
    if (!TestNotNull(TEXT("Native game mode"), GM) || !TestNotNull(TEXT("Replicated game state"), GS))
    {
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
    }
    World->BeginPlay();
    TestTrue(TEXT("Starts waiting"), GS->MatchPhase == ECXMatchPhase::Waiting);
    TestFalse(TEXT("No countdown before login"), World->GetTimerManager().IsTimerActive(GM->MatchTimer));
    ACXPlayerController* PC1 = World->SpawnActor<ACXPlayerController>();
    ACXPlayerController* PC2 = World->SpawnActor<ACXPlayerController>();
    ACXPlayerState* PS1 = PC1->GetPlayerState<ACXPlayerState>();
    ACXPlayerState* PS2 = PC2->GetPlayerState<ACXPlayerState>();
    if (!TestNotNull(TEXT("Player one state"), PS1) || !TestNotNull(TEXT("Player two state"), PS2))
    {
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
    }
    PS1->SetPlayerId(1); PS2->SetPlayerId(2);
    PS1->SetPlayerName(TEXT("One")); PS2->SetPlayerName(TEXT("Two"));
    PS1->AddCoins(10);
    TestEqual(TEXT("Coin increment"), PS1->GetCoins(), 10);
    TestFalse(TEXT("Cannot remove more than balance"), PS1->TryRemoveCoins(11));
    TestFalse(TEXT("Cannot remove negative coins"), PS1->TryRemoveCoins(-5));
    TestTrue(TEXT("Exact balance withdrawal"), PS1->TryRemoveCoins(10));
    PS1->SetRole(ROLE_SimulatedProxy);
    PS1->AddCoins(10); PS1->AddBonusMaxSpeed(10);
    TestEqual(TEXT("Client cannot mint coins"), PS1->GetCoins(), 0);
    TestEqual(TEXT("Client cannot change speed"), PS1->GetBonusMaxSpeed(), 0.f);
    PS1->SetRole(ROLE_Authority);
    TArray<FLifetimeProperty> Props;
    ACXPlayerState::StaticClass()->SetUpRuntimeReplicationData();
    PS1->GetLifetimeReplicatedProps(Props);
    for (const FName Name : {FName(TEXT("Coins")), FName(TEXT("BonusMaxSpeed"))})
    {
        const FProperty* Property = FindFProperty<FProperty>(ACXPlayerState::StaticClass(), Name);
        const FLifetimeProperty* Rep = Props.FindByPredicate([Property](const FLifetimeProperty& P) { return P.RepIndex == Property->RepIndex; });
        TestTrue(*FString::Printf(TEXT("%s owner only"), *Name.ToString()), Rep && Rep->Condition == COND_OwnerOnly);
    }
    // This isolated world has no network connections. The separate network
    // smoke test exercises PostLogin; here we drive the real countdown callback.
    World->GetTimerManager().SetTimer(GM->MatchTimer, GM, &ACXGameMode::AdvanceMatch, 1.f, true);
    GS->RemainingTime = 1;
    GM->AdvanceMatch();
    TestTrue(TEXT("Waiting transitions to Playing"), GS->IsPlaying());
    TestEqual(TEXT("Play countdown initialized"), GS->RemainingTime, 120);

    UClass* CarClass = LoadClass<ACXPawn>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn.BP_SportsCar_Pawn_C"));
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACXPawn* Car1 = World->SpawnActor<ACXPawn>(CarClass, FVector(0,0,500), FRotator::ZeroRotator, Spawn);
    ACXPawn* Car2 = World->SpawnActor<ACXPawn>(CarClass, FVector(900,0,500), FRotator::ZeroRotator, Spawn);
    if (TestNotNull(TEXT("Car one"), Car1) && TestNotNull(TEXT("Car two"), Car2))
    {
        PC1->Possess(Car1); PC2->Possess(Car2);
        PS2->AddCoins(4);
        TestEqual(TEXT("Victim under five is protected"), Car2->TakeDamage(5, FDamageEvent(), PC1, Car1), 0.f);
        TestEqual(TEXT("No award when victim has four"), PS1->GetCoins(), 0);
        PS2->AddCoins(1);
        TestEqual(TEXT("Five coin transfer"), Car2->TakeDamage(5, FDamageEvent(), PC1, Car1), 5.f);
        TestEqual(TEXT("Victim loses five"), PS2->GetCoins(), 0);
        TestEqual(TEXT("Attacker gains five"), PS1->GetCoins(), 5);
        PS2->AddCoins(10);
        Car1->ServerAttack_Implementation();
        TestEqual(TEXT("Bumper line trace damages other car"), PS2->GetCoins(), 5);
        Car1->ServerAttack_Implementation();
        TestEqual(TEXT("Server attack cooldown"), PS2->GetCoins(), 5);

        ACXCoin* Coin = World->SpawnActor<ACXCoin>(ACXCoin::StaticClass(), FVector(3000,0,500), FRotator::ZeroRotator, Spawn);
        const int32 Before = PS1->GetCoins();
        Coin->OnOverlap(nullptr, Car1, nullptr, 0, false, FHitResult());
        Coin->OnOverlap(nullptr, Car1, nullptr, 0, false, FHitResult());
        TestEqual(TEXT("Coin pays ten exactly once"), PS1->GetCoins(), Before + 10);
        TestEqual(TEXT("Coin update frequency"), Coin->GetNetUpdateFrequency(), 1.f);
        ACXSpeedBuff* Buff = World->SpawnActor<ACXSpeedBuff>(ACXSpeedBuff::StaticClass(), FVector(4000,0,500), FRotator::ZeroRotator, Spawn);
        Buff->OnOverlap(nullptr, Car1, nullptr, 0, false, FHitResult());
        Buff->OnOverlap(nullptr, Car1, nullptr, 0, false, FHitResult());
        TestEqual(TEXT("Buff pays ten exactly once"), PS1->GetBonusMaxSpeed(), 10.f);
        TestEqual(TEXT("Buff update frequency"), Buff->GetNetUpdateFrequency(), 1.f);
        TestFalse(TEXT("Coin effect waits for destruction"), Coin->bEffectsPlayed);
        TestFalse(TEXT("Buff effect waits for destruction"), Buff->bEffectsPlayed);
        TestNotNull(TEXT("Coin particle assigned"), Coin->PickupEffect.Get());
        TestNotNull(TEXT("Coin sound assigned"), Coin->PickupSound.Get());
        TestNotNull(TEXT("Buff particle assigned"), Buff->PickupEffect.Get());
        TestNotNull(TEXT("Buff sound assigned"), Buff->PickupSound.Get());
        Coin->Destroy();
        Buff->Destroy();
        TestTrue(TEXT("Coin destruction plays cosmetics"), Coin->bEffectsPlayed);
        TestTrue(TEXT("Buff destruction plays cosmetics"), Buff->bEffectsPlayed);

        const int32 BeforeOverlap = PS1->GetCoins();
        ACXCoin* PhysicalCoin = World->SpawnActor<ACXCoin>(ACXCoin::StaticClass(), Car1->GetActorLocation(), FRotator::ZeroRotator, Spawn);
        PhysicalCoin->Trigger->UpdateOverlaps();
        TestEqual(TEXT("Real vehicle mesh overlap collects coin"), PS1->GetCoins(), BeforeOverlap + 10);

        ACXItemSpawner* Spawner = World->SpawnActor<ACXItemSpawner>(ACXItemSpawner::StaticClass(), FVector(5000,0,500), FRotator::ZeroRotator, Spawn);
        TestTrue(TEXT("Spawner replicated to avoid client authority copies"), Spawner->GetIsReplicated());
        ACXPickup* First = Spawner->SpawnedItem;
        if (TestNotNull(TEXT("Spawner creates pickup"), First))
        {
            First->Destroy();
            TestNull(TEXT("No instant respawn"), Spawner->SpawnedItem.Get());
            // TimerManager runs at most once per engine frame.
            const uint64 OriginalFrame = GFrameCounter;
            // Timers set before the first world tick become active at the end
            // of that tick. Prime the timer manager without advancing time.
            ++GFrameCounter; World->GetTimerManager().Tick(0.f);
            ++GFrameCounter; World->GetTimerManager().Tick(1.f);
            TestNull(TEXT("Still absent after one second"), Spawner->SpawnedItem.Get());
            ++GFrameCounter; World->GetTimerManager().Tick(1.1f);
            TestTrue(TEXT("Respawns same kind after two seconds"), IsValid(Spawner->SpawnedItem) && Spawner->SpawnedItem->IsA<ACXCoin>());
            GFrameCounter = OriginalFrame;
        }
        PS2->AddCoins(PS1->GetCoins() - PS2->GetCoins());
        GS->RemainingTime = 1;
        GM->AdvanceMatch();
        TestTrue(TEXT("Playing transitions to Finished"), GS->MatchPhase == ECXMatchPhase::Finished);
        TestEqual(TEXT("Both tied players win"), GS->WinnerPlayerIds.Num(), 2);
        TestTrue(TEXT("Winner identity one"), GS->WinnerPlayerIds.Contains(1));
        TestTrue(TEXT("Winner identity two"), GS->WinnerPlayerIds.Contains(2));
        TestEqual(TEXT("No damage after finish"), Car2->TakeDamage(5, FDamageEvent(), PC1, Car1), 0.f);
        TestFalse(TEXT("Timer stopped after finish"), World->GetTimerManager().IsTimerActive(GM->MatchTimer));
        PS2->AddCoins(1);
        GM->FinishMatch();
        TestEqual(TEXT("Unequal scores have one winner"), GS->WinnerPlayerIds.Num(), 1);
        TestTrue(TEXT("Higher score wins"), GS->WinnerPlayerIds.Contains(2));
    }
    UChatSubsystem* Chat = World->GetSubsystem<UChatSubsystem>();
    for (int32 I=0; I<15; ++I) Chat->AddChatMessage(TEXT("One"), FString::FromInt(I));
    TestEqual(TEXT("History bounded to ten"), Chat->GetChatLog().Num(), 10);
    TestEqual(TEXT("Most recent history retained"), Chat->GetChatLog()[0].Message, FString(TEXT("5")));
    PC1->STCChatLog_Implementation(Chat->GetChatLog());
    TestEqual(TEXT("History buffered before UI creation"), PC1->PendingChat.Num(), 10);
    PC1->CTSChatMessage_Implementation(TEXT(" hello\nworld "));
    TestEqual(TEXT("Chat server sanitizes content"), Chat->GetChatLog().Last().Message, FString(TEXT("hello world")));
    PC1->CTSChatMessage_Implementation(TEXT("spam"));
    TestEqual(TEXT("Chat rate limited"), Chat->GetChatLog().Last().Message, FString(TEXT("hello world")));
    PC1->Destroy();
    TestTrue(TEXT("One disconnect preserves the match"), GS->MatchPhase == ECXMatchPhase::Finished);
    PC2->Destroy();
    TestTrue(TEXT("Empty server returns to waiting"), GS->MatchPhase == ECXMatchPhase::Waiting);
    TestEqual(TEXT("Empty server resets countdown"), GS->RemainingTime, 10);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
