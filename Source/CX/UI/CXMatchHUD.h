#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXMatchHUD.generated.h"
class UTextBlock;
UCLASS()
class CX_API UCXMatchHUD : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
};
