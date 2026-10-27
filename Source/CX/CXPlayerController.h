// CXPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/ChatSubsystem.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class ACXPawn;
class UCXUI;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 */
UCLASS()
class CX_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()
	friend class FCXGameplayTest;
	friend struct FCXNetworkSmokeState;

protected:
	friend class UChatUI;
	friend class ACXGameMode;

	UFUNCTION(Client, Reliable)
	void STCChatLog(const TArray<FChatData>& ChatLog);

	UFUNCTION(Server, Reliable)
	void CTSChatMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void STCChatMessage(const FString& Sender, const FString& Message);

protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	/** If true, the optional steering wheel input mapping context will be registered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	bool bUseSteeringWheelControls = false;

	/** Optional Input Mapping Context to be used for steering wheel input.
	 *  This is added alongside the default Input Mapping Context and does not block other forms of input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(EditCondition="bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	/** Pointer to the controlled vehicle pawn */
	UPROPERTY(Replicated)
	TObjectPtr<ACXPawn> VehiclePawn;

	/** Type of the UI to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXUI> VehicleUIClass;

	/** Pointer to the UI widget */
	UPROPERTY(Transient)
	TObjectPtr<UCXUI> VehicleUI;
	UPROPERTY(Transient) TObjectPtr<class UCXMatchHUD> MatchHUD;
	TArray<FChatData> PendingChat;
	double LastChatTime = -1000.0;
	void FocusChat();

	

	// Begin Actor interface
protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

public:

	virtual void Tick(float Delta) override;

	// End Actor interface

	// Begin PlayerController interface
protected:

	virtual void OnPossess(APawn* InPawn) override;

	// End PlayerController interface
};
