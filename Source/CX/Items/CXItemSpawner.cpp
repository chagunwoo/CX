#include "Items/CXItemSpawner.h"
#include "Items/CXPickup.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
ACXItemSpawner::ACXItemSpawner()
{
    // Placed non-replicated actors have authority even on clients. Replicate the
    // spawner and also check net mode so clients never create local pickup copies.
    bReplicates = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    ItemClass = ACXCoin::StaticClass();
}
void ACXItemSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) SpawnItem();
}
void ACXItemSpawner::SpawnItem()
{
    if (!HasAuthority() || GetNetMode() == NM_Client || IsValid(SpawnedItem) || !ItemClass) return;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnedItem = GetWorld()->SpawnActor<ACXPickup>(ItemClass, GetActorTransform(), Params);
    if (SpawnedItem) SpawnedItem->OnDestroyed.AddDynamic(this, &ACXItemSpawner::OnItemDestroyed);
}
void ACXItemSpawner::OnItemDestroyed(AActor*)
{
    SpawnedItem = nullptr;
    if (HasAuthority()) GetWorldTimerManager().SetTimer(RespawnTimer, this, &ACXItemSpawner::SpawnItem, FMath::Max(.1f, RespawnDelay), false);
}
void ACXItemSpawner::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(RespawnTimer);
    if (IsValid(SpawnedItem))
    {
        SpawnedItem->OnDestroyed.RemoveDynamic(this, &ACXItemSpawner::OnItemDestroyed);
        if (HasAuthority() && Reason == EEndPlayReason::Destroyed) SpawnedItem->Destroy();
    }
    Super::EndPlay(Reason);
}
