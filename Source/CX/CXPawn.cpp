// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	bReplicates = true;
	static ConstructorHelpers::FObjectFinder<UParticleSystem> Sparks(TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks"));
	HitEffect = Sparks.Object;
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::BeginPlay()
{
	Super::BeginPlay();
	// Vehicle template assets may disable overlap generation on the mesh.
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		if (!AttackAction)
		{
			AttackAction = NewObject<UInputAction>(this);
			AttackAction->ValueType = EInputActionValueType::Boolean;
		}
		if (!AttackMapping)
		{
			AttackMapping = NewObject<UInputMappingContext>(this);
			AttackMapping->MapKey(AttackAction, EKeys::LeftMouseButton);
		}
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
			if (UEnhancedInputLocalPlayerSubsystem* Inputs = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
				Inputs->AddMappingContext(AttackMapping, 1);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::Attack);

		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// km/h -> cm/s. Only the server and autonomous owner enforce the cap;
	// simulated proxies follow replicated vehicle physics.
	if (HasAuthority() || IsLocallyControlled())
	{
		const ACXPlayerState* PS = GetPlayerState<ACXPlayerState>();
		const float Limit = (BaseMaxSpeed + (PS ? PS->GetBonusMaxSpeed() : 0.f)) / .036f;
		FVector Velocity = GetMesh()->GetPhysicsLinearVelocity();
		const float HorizontalSpeed = Velocity.Size2D();
		if (HorizontalSpeed > Limit && Limit > 0.f)
		{
			Velocity.X *= Limit / HorizontalSpeed;
			Velocity.Y *= Limit / HorizontalSpeed;
			GetMesh()->SetPhysicsLinearVelocity(Velocity);
		}
	}

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

#undef LOCTEXT_NAMESPACE

void ACXPawn::Attack(const FInputActionValue&)
{
	ServerAttack();
}

void ACXPawn::ServerAttack_Implementation()
{
	const ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>();
	const float Now = GetWorld()->GetTimeSeconds();
	if (!GS || !GS->IsPlaying() || !Controller || Now - LastAttackTime < AttackCooldown) return;
	LastAttackTime = Now;
	const FBox Bounds = GetMesh()->GetLocalBounds().GetBox();
	const FVector Min = Bounds.Min, Max = Bounds.Max;
	const FVector Start = GetMesh()->GetComponentTransform().TransformPosition(FVector(Max.X + 5.f, (Min.Y + Max.Y) * .5f, (Min.Z + Max.Z) * .5f));
	const FVector End = Start + GetActorForwardVector() * AttackRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CXAttack), false, this);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	ACXPawn* Victim = Cast<ACXPawn>(Hit.GetActor());
	if (Victim && Victim != this)
		UGameplayStatics::ApplyDamage(Victim, 5.f, Controller, this, UDamageType::StaticClass());
	MulticastAttackEffects(Start, Hit.bBlockingHit ? Hit.ImpactPoint : End, Victim != nullptr);
}

float ACXPawn::TakeDamage(float DamageAmount, const FDamageEvent&, AController* InstigatorController, AActor* Causer)
{
	const ACXGameState* GS = GetWorld()->GetGameState<ACXGameState>();
	ACXPawn* Attacker = Cast<ACXPawn>(Causer);
	if (!HasAuthority() || !GS || !GS->IsPlaying() || DamageAmount <= 0.f || !Attacker || Attacker == this || !InstigatorController || Attacker->GetController() != InstigatorController) return 0.f;
	ACXPlayerState* VictimState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerState = Attacker->GetPlayerState<ACXPlayerState>();
	if (!VictimState || !AttackerState || VictimState == AttackerState || AttackerState->GetCoins() > MAX_int32 - 5 || !VictimState->TryRemoveCoins(5)) return 0.f;
	AttackerState->AddCoins(5);
	return 5.f;
}

void ACXPawn::MulticastAttackEffects_Implementation(FVector Start, FVector End, bool bHitVehicle)
{
	if (GetNetMode() == NM_DedicatedServer) return;
	DrawDebugLine(GetWorld(), Start, End, bHitVehicle ? FColor::Red : FColor::Green, false, 1.f, 0, 3.f);
	if (bHitVehicle && HitEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, End);
}

void ACXPawn::EndPlay(const EEndPlayReason::Type Reason)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Inputs = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			if (AttackMapping) Inputs->RemoveMappingContext(AttackMapping);
	Super::EndPlay(Reason);
}
