#include "Items/CXPickup.h"
#include "CXPawn.h"
#include "CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
ACXPickup::ACXPickup()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(1.f);
    SetMinNetUpdateFrequency(1.f);
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .25f;
    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetSphereRadius(100.f);
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACXPickup::OnOverlap);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Trigger);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UParticleSystem> Effect(TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks"));
    static ConstructorHelpers::FObjectFinder<USoundBase> Sound(TEXT("/Game/StarterContent/Audio/Light01_Cue.Light01_Cue"));
    PickupEffect = Effect.Object;
    PickupSound = Sound.Object;
}
ACXCoin::ACXCoin()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Mesh->SetStaticMesh(Shape.Object);
    Mesh->SetRelativeScale3D(FVector(.65f, .65f, .12f));
    Mesh->SetRelativeRotation(FRotator(0, 0, 90));
}
ACXSpeedBuff::ACXSpeedBuff()
{
    bSpeedBuff = true;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Mesh->SetStaticMesh(Shape.Object);
    Mesh->SetRelativeScale3D(FVector(.65f));
}
void ACXPickup::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority()) Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorTickEnabled(HasAuthority());
}
void ACXPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bCollected) return;
    TArray<AActor*> Vehicles;
    Trigger->GetOverlappingActors(Vehicles, ACXPawn::StaticClass());
    for (AActor* Vehicle : Vehicles) OnOverlap(Trigger, Vehicle, nullptr, 0, false, FHitResult());
}
void ACXPickup::OnOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!HasAuthority() || bCollected) return;
    const ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>();
    if (!GS || !GS->IsPlaying()) return;
    ACXPawn* Vehicle = Cast<ACXPawn>(Other);
    ACXPlayerState* PS = Vehicle ? Vehicle->GetPlayerState<ACXPlayerState>() : nullptr;
    if (!PS) return;
    bCollected = true;
    if (bSpeedBuff) PS->AddBonusMaxSpeed(10.f);
    else PS->AddCoins(10);
    MulticastCollected();
    ForceNetUpdate();
    // Allow the collection flag to reach clients before destruction.
    SetLifeSpan(.25f);
}
void ACXPickup::MulticastCollected_Implementation()
{
    bCollected = true;
    OnRep_Collected();
}
void ACXPickup::OnRep_Collected()
{
    if (!bCollected) return;
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    // Cosmetics are played by Destroyed on each machine, not before destruction.
}
void ACXPickup::PlayCollectionEffects()
{
    if (bEffectsPlayed || GetNetMode() == NM_DedicatedServer) return;
    bEffectsPlayed = true;
    if (PickupEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupEffect, GetActorTransform());
    if (PickupSound) UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
}
void ACXPickup::Destroyed()
{
    if (bCollected) PlayCollectionEffects();
    Super::Destroyed();
}
void ACXPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXPickup, bCollected);
}
