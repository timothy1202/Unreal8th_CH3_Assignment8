#pragma once

#include "CoreMinimal.h"
#include "BaseItem.h"
#include "Components/BoxComponent.h"


#include "SpikeItem.generated.h"

UCLASS()
class SPARTAPROJECT_API ASpikeItem : public ABaseItem
{
	GENERATED_BODY()
public:
	ASpikeItem();
	virtual void ActivateItem(AActor* Activator) override;
	virtual void BeginPlay() override;

	// 토글 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float ToggleInterval = 3.0f;

protected:
	// 현재 활성 상태(보이는 상태일 때 true)
	bool bIsActive = false;

	FTimerHandle ToggleTimerHandle;

	// 정기적으로 호출되어 보임/콜리전 토글
	UFUNCTION()
	void ToggleActiveState();

};