#include "SlowingItem.h"
#include "SpartaCharacter.h"

ASlowingItem::ASlowingItem()
{
	ItemType = "SlowingItem";
}

void ASlowingItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);

	if (Activator && Activator->ActorHasTag("Player"))
	{
		if (ASpartaCharacter* PlayerCharacter = Cast<ASpartaCharacter>(Activator))
		{
			PlayerCharacter->ApplySlowingEffect(); // 5초 동안 이동 속도를 50%로 감소
		}
		DestroyItem();
	}
}
