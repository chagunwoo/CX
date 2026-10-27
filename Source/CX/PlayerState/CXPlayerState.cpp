#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"
void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}
void ACXPlayerState::AddCoins(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0) return;
    Coins = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Coins) + Amount, MAX_int32));
    ForceNetUpdate();
}
bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0 || Coins < Amount) return false;
    Coins -= Amount;
    ForceNetUpdate();
    return true;
}
void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
    if (!HasAuthority() || !FMath::IsFinite(Amount) || Amount <= 0.f || !FMath::IsFinite(BonusMaxSpeed + Amount)) return;
    BonusMaxSpeed += Amount;
    ForceNetUpdate();
}
FString ACXPlayerState::GetPlayerStateInfo() const
{
    return FString::Printf(TEXT("Coins: %d | Bonus max speed: +%.0f km/h"), Coins, BonusMaxSpeed);
}
