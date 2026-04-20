// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseItem.h"
#include "BombItem.generated.h"

/**
 * 
 */
UCLASS()
class SPARTAPROJECT_API ABombItem : public ABaseItem
{
	GENERATED_BODY()
public:
	ABombItem();
	virtual void ActivateItem(AActor* Activator) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 ExplosionDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float ExplosionDelay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effects")
	UParticleSystem* ExplosionParticle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effects")
	USoundBase* ExplosionSound;

	FTimerHandle ExplosionTimerHandle;
	void Explode();
};
