// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseItem.h"
#include "ReverseControlItem.generated.h"

/**
 * 
 */
UCLASS()
class SPARTAPROJECT_API AReverseControlItem : public ABaseItem
{
	GENERATED_BODY()
public:
	AReverseControlItem();
	virtual void ActivateItem(AActor* Activator) override;
};
