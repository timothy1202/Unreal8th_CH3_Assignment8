// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawnRow.h"
#include "SpawnVolume.generated.h"

class UBoxComponent;
class USceneComponent;
class UDataTable;
class ASpartaGameState;

UCLASS()
class SPARTAPROJECT_API ASpawnVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnVolume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	USceneComponent* Scene;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	UBoxComponent* SpawningBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	UDataTable* ItemDataTable;

	// 기존 아이템 스폰
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	AActor* SpawnRandomItem();

	FItemSpawnRow* GetRandomItem() const;
	AActor* SpawnItem(TSubclassOf<AActor> ItemClass);
	FVector GetRandomPointInVolume() const;

	// --- 폭발 스폰을 위한 설정 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	TSubclassOf<AActor> ExplosionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionSpawnInterval = 5.0f; // 기본 5초 간격

	// --- 웨이브 조건 (사용자 요청: "3웨이브일 때만")
	// TargetWaveNumber는 1 기반. 예: 3 => CurrentWaveIndex == 2일 때 동작
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Wave")
	int32 TargetWaveNumber = 3;

	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void StartSpawningExplosions();

	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void StopSpawningExplosions();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle ExplosionSpawnTimerHandle;
	void SpawnRandomExplosion();
};
