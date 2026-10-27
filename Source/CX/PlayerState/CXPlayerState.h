#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    int32 GetCoins() const { return Coins; }
    float GetBonusMaxSpeed() const { return BonusMaxSpeed; }
    float GetBounusMaxSpeed() const { return GetBonusMaxSpeed(); }
    void AddCoins(int32 Amount);
    bool TryRemoveCoins(int32 Amount);
    void AddBonusMaxSpeed(float Amount);
    void AddBounusMaxSpeed(float Amount) { AddBonusMaxSpeed(Amount); }
    UFUNCTION(BlueprintCallable) FString GetPlayerStateInfo() const;
protected:
    UPROPERTY(Replicated) int32 Coins = 0;
    UPROPERTY(Replicated) float BonusMaxSpeed = 0.f;
};
