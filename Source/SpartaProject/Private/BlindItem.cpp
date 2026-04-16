// Fill out your copyright notice in the Description page of Project Settings.

#include "BlindItem.h"
#include "SpartaCharacter.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Camera/PlayerCameraManager.h" 

ABlindItem::ABlindItem()
{
	ItemType = "BlindItem";
}

void ABlindItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);
	if (Activator && Activator->ActorHasTag("Player"))
	{
		if (ASpartaCharacter* PlayerCharacter = Cast<ASpartaCharacter>(Activator))
		{
			// 블라인드 효과를 캐릭터에게 위임
			PlayerCharacter->ApplyBlindEffect();
		}

		DestroyItem();
	}
}