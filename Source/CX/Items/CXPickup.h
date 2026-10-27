#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXPickup.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;
UCLASS(Abstract)
class CX_API ACXPickup : public AActor
{
    GENERATED_BODY()
    friend class FCXGameplayTest;
public:
    ACXPickup();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Destroyed() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Trigger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditDefaultsOnly, Category="Pickup") bool bSpeedBuff = false;
    UPROPERTY(EditDefaultsOnly, Category="Pickup") TObjectPtr<UParticleSystem> PickupEffect;
    UPROPERTY(EditDefaultsOnly, Category="Pickup") TObjectPtr<USoundBase> PickupSound;
    UPROPERTY(ReplicatedUsing=OnRep_Collected) bool bCollected = false;
    bool bEffectsPlayed = false;
    UFUNCTION() void OnRep_Collected();
    UFUNCTION(NetMulticast, Reliable) void MulticastCollected();
    UFUNCTION() void OnOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
    void PlayCollectionEffects();
};
UCLASS()
class CX_API ACXCoin : public ACXPickup
{
    GENERATED_BODY()
public: ACXCoin();
};
UCLASS()
class CX_API ACXSpeedBuff : public ACXPickup
{
    GENERATED_BODY()
public: ACXSpeedBuff();
};
