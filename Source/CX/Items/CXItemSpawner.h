#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"
class ACXPickup;
UCLASS()
class CX_API ACXItemSpawner : public AActor
{
    GENERATED_BODY()
public:
    ACXItemSpawner();
    UPROPERTY(EditAnywhere, Category="Spawner") TSubclassOf<ACXPickup> ItemClass;
    UPROPERTY(EditAnywhere, Category="Spawner", meta=(ClampMin="0.1")) float RespawnDelay = 2.f;
    UPROPERTY(VisibleInstanceOnly, Category="Spawner") TObjectPtr<ACXPickup> SpawnedItem;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void SpawnItem();
    UFUNCTION() void OnItemDestroyed(AActor* Item);
    FTimerHandle RespawnTimer;
};
