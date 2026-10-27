#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"
UCLASS()
class CX_API ACXGameMode : public AGameModeBase
{
    GENERATED_BODY()
    friend class FCXGameplayTest;
public:
    ACXGameMode();
    virtual void InitGameState() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
protected:
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="1")) int32 WaitingDuration = 10;
    UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="1")) int32 PlayingDuration = 120;
private:
    void AdvanceMatch();
    void FinishMatch();
    bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;
    FTimerHandle MatchTimer;
    UPROPERTY(EditDefaultsOnly) FVector VehicleSpawnCheckHalfExtent = FVector(250, 120, 80);
    UPROPERTY(EditDefaultsOnly) FVector VehicleSpawnCheckOffset = FVector(0, 0, 80);
};
