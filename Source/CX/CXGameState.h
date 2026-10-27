#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"
UENUM(BlueprintType)
enum class ECXMatchPhase : uint8 { Waiting, Playing, Finished };
UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Match") ECXMatchPhase MatchPhase = ECXMatchPhase::Waiting;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Match") int32 RemainingTime = 0;
    // IDs and names preserve the result even if a winner disconnects afterwards.
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Match") TArray<int32> WinnerPlayerIds;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Match") TArray<FString> WinnerNames;
    bool IsPlaying() const { return MatchPhase == ECXMatchPhase::Playing; }
};
