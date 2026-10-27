#include "CXGameState.h"
#include "Net/UnrealNetwork.h"
void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXGameState, MatchPhase);
    DOREPLIFETIME(ACXGameState, RemainingTime);
    DOREPLIFETIME(ACXGameState, WinnerPlayerIds);
    DOREPLIFETIME(ACXGameState, WinnerNames);
}
