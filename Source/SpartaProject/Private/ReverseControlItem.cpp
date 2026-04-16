#include "ReverseControlItem.h"
#include "SpartaCharacter.h"

AReverseControlItem::AReverseControlItem()
{
	ItemType = "ReverseControlItem";
}

void AReverseControlItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);
	if(Activator && Activator->ActorHasTag("Player"))
	{
		if (ASpartaCharacter* PlayerCharacter = Cast<ASpartaCharacter>(Activator))
		{
			PlayerCharacter->ApplyReverseControlEffect(); // 5초 동안 입력 반전
		}
		DestroyItem();
	}
}
